/*
This is a simple read-only implementation of a file system. It uses a block of data coming from the
mkespfsimg tool, and can use that block to do abstracted operations on the files that are in there.
It's written for use with httpd, but doesn't need to be used as such.
*/

#ifdef __ets__
//esp build
#include "driver/uart.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#else
//Test build
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define os_malloc malloc
#define os_free free
#define os_memcpy memcpy
#define os_strncmp strncmp
#define os_strcmp strcmp
#define os_strcpy strcpy
#define os_printf printf
#define ICACHE_FLASH_ATTR
extern char* espFsData;
#endif

#include "../mkespfsimage/espfsformat.h"
#include "espfs.h"
#include "httpdconfig.h"
#ifdef EFS_HEATSHRINK
#include "heatshrink_decoder.h"
#endif

struct EspFsFile {
	EspFsHeader *header;
	char decompressor;
	int32_t posDecomp;
	char *posStart;
	char *posComp;
	void *decompData;
};

/*
Available locations, at least in my flash, with boundaries partially guessed:
0x00000 (0x10000): Code/data (RAM data?)
0x10000 (0x02000): Gets erased by something?
0x12000 (0x2E000): Free (filled with zeroes) (parts used by ESPCloud and maybe SSL)
0x40000 (0x20000): Code/data (ROM data?)
0x60000 (0x1C000): Free
0x7c000 (0x04000): Param store
0x80000 - end of flash

Accessing the flash through the mem emulation at 0x40200000 is a bit hairy: All accesses
*must* be aligned 32-bit accesses. Reading a short, byte or unaligned word will result in
a memory exception, crashing the program.
*/


//Copies len bytes over from dst to src, but does it using *only*
//aligned 32-bit reads.
void ICACHE_FLASH_ATTR memcpyAligned(char *dst, char *src, int len) {
	int x;
	int w, b;
	for (x=0; x<len; x++) {
		b=((int)src&3);
		w=*((int *)(src-b));
		if (b==0) *dst=(w>>0);
		if (b==1) *dst=(w>>8);
		if (b==2) *dst=(w>>16);
		if (b==3) *dst=(w>>24);
		dst++; src++;
	}
}



EspFsFile ICACHE_FLASH_ATTR *espFsOpen(char *fileName) {
#ifdef __ets__
	char *p=(char *)(ESPFS_POS+0x40200000);
#else
	char *p=espFsData;
#endif
	char *hpos;
	char namebuf[256];
	EspFsHeader h;
	EspFsFile *r;
	//Skip initial slashes
	while(fileName[0]=='/') fileName++;
	//Go find that file!
	while(1) {
		hpos=p;
		os_memcpy(&h, p, sizeof(EspFsHeader));
		if (h.magic!=0x73665345) {
			os_printf("Magic mismatch. EspFS image broken.\n");
			return NULL;
		}
		if (h.flags&FLAG_LASTFILE) {
			os_printf("End of image.\n");
			return NULL;
		}
		p+=sizeof(EspFsHeader);
		os_memcpy(namebuf, p, sizeof(namebuf));
		os_printf("Found file '%s'. Namelen=%x fileLenComp=%x, compr=%d flags=%d\n", namebuf, h.nameLen,h.fileLenComp,h.compression,h.flags);
		if (os_strcmp(namebuf, fileName)==0) {
			p+=h.nameLen;
			r=(EspFsFile *)os_malloc(sizeof(EspFsFile));
			if (r==NULL) return NULL;
			r->header=(EspFsHeader *)hpos;
			r->decompressor=h.compression;
			r->posComp=p;
			r->posStart=p;
			r->posDecomp=0;
			if (h.compression==COMPRESS_NONE) {
				r->decompData=NULL;
#ifdef EFS_HEATSHRINK
			} else if (h.compression==COMPRESS_HEATSHRINK) {
				char parm;
				//Decoder params are stored in 1st byte.
				memcpyAligned(&parm, r->posComp, 1);
				r->posComp++;
				os_printf("Heatshrink compressed file; decode parms = %x\n", parm);
				r->decompData=(heatshrink_decoder *)heatshrink_decoder_alloc(16, (parm>>4)&0xf, parm&0xf);
				os_printf("Decompressor allocated.\n");
#endif
			} else {
				os_printf("Invalid compression: %d\n", h.compression);
				return NULL;
			}
			return r;
		}
		//Skip name and file
		p+=h.nameLen+h.fileLenComp;
		if ((int)p&3) p+=4-((int)p&3); //align to next 32bit val
//		os_printf("Next addr = %x\n", (int)p);
	}
}


int ICACHE_FLASH_ATTR espFsRead(EspFsFile *fh, char *buff, int len) {
	int flen;
	if (fh==NULL) return 0;
		memcpyAligned((char*)&flen, (char*)&fh->header->fileLenComp, 4);
	if (fh->decompressor==COMPRESS_NONE) {
		int toRead;
		toRead=flen-(fh->posComp-fh->posStart);
		if (len>toRead) len=toRead;
		os_printf("Reading %d bytes from %x\n", len, fh->posComp);
		memcpyAligned(buff, fh->posComp, len);
		fh->posDecomp+=len;
		fh->posComp+=len;
//		os_printf("Done reading %d bytes, pos=%x\n", len, fh->posComp);
		return len;
#ifdef EFS_HEATSHRINK
	} else if (fh->decompressor==COMPRESS_HEATSHRINK) {
		int decoded=0;
		int elen, rlen, r;
		char ebuff[16];
		os_printf("heatshrink: reading\n");
		heatshrink_decoder *dec=(heatshrink_decoder *)fh->decompData;
		while(decoded<len) {
			//Feed data into the decompressor
			elen=flen-(fh->posComp - fh->posStart);
			if (elen==0) return decoded; //file is read
			if (elen>0) {
				os_printf("heatshrink: feeding decoder (%d comp bytes left)\n", elen);
				memcpyAligned(ebuff, fh->posComp, 16);
				for (r=0; r<16; r++) os_printf("%02hhx ", ebuff[r]);
				os_printf("\n");
				r=heatshrink_decoder_sink(dec, ebuff, (elen>16)?16:elen, &rlen);
				os_printf("heatshrink: decoder ate %d bytes (code %d)\n", rlen, r);
				fh->posComp+=rlen;
				if (rlen==elen) {
					os_printf("heatshrink: finish\n");
					heatshrink_decoder_finish(dec);
				}
			}
			//Grab decompressed data and put into buff
			r=heatshrink_decoder_poll(dec, buff, len-decoded, &rlen);
			os_printf("heatshrink: decoder emitted %d bytes (code %d)\n", rlen, r);
			fh->posDecomp+=rlen;
			buff+=rlen;
			decoded+=rlen;
		}
		return len;
#endif
	}
	return 0;
}

void ICACHE_FLASH_ATTR espFsClose(EspFsFile *fh) {
	if (fh==NULL) return;
#ifdef EFS_HEATSHRINK
	if (fh->decompressor==COMPRESS_HEATSHRINK) {
		heatshrink_decoder_free((heatshrink_decoder*)fh->decompData);
	}
#endif
	os_free(fh);
}


