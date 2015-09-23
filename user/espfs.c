/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		espfs.c
*	purpose:	espfs support implementation
************************************************/

#include "espfs.h"

#ifdef ESPFS_TEST
extern char *	espFsData 	= NULL;
#endif

/************************************************
*	name:			memcpyAligned
*	parameters:		pbDest 		- destination buffer
*					pbSrc		- source buffer
*					cbLength	- amount of bytes to copy
*	return value:	none
*	purpose:		memcpy using only aligned 32-bit reads
************************************************/
void ICACHE_FLASH_ATTR memcpyAligned(char *	pbDest, 
									 char *	pbSrc, 
									 int 	cbLength)
{
	/* initialization */
	int iIndex;
	int w;
	int b;

	for (iIndex = 0; iIndex < cbLength; ++iIndex, ++pbSrc, ++pbDest)
	{
		b = ((int)pbSrc & 3);
		w = *((int *)(pbSrc - b));

		if (b==0) *pbDest = (w >> 0);
		if (b==1) *pbDest = (w >> 8);
		if (b==2) *pbDest = (w >> 16);
		if (b==3) *pbDest = (w >> 24);
	}
}

/* espFsOpen */
EspFsFile ICACHE_FLASH_ATTR *	espFsOpen(char *	pszFilename)
{
	/* initialization */
	char *		pbHeaderOffset			= NULL;
	char *		pbTmpFile				= NULL;
	EspFsFile *	ptEspFile				= NULL;
	char 		pszFileNameBuffer[256]	= { 0 };
	EspFsHeader tEspFileHeader			= { 0 };

	/* acquire file system beginning */
#ifndef ESPFS_TEST
	int 		iEspFsOffset = 0;
	
	if (0 == system_upgrade_userbin_check())
	{
		iEspFsOffset = partition[ESPFS_PART].iOffset;
	}
	else
	{
		iEspFsOffset = partition[ESPFS_PART2].iOffset;
	}

	pbTmpFile = (char *)(iEspFsOffset + ESP_FLASH_OFFSET);
#else	
	pbTmpFile = espFsData;
#endif
	
	/* strip file name slashes */
	while (SLASH_SYMBOL_STRING == pszFilename[0])
	{
		++pszFilename;
	}

	/* locate file */
	while (1)
	{
		pbHeaderOffset = pbTmpFile;
		
		/* retrieve file header */
		os_memcpy(&tEspFileHeader, pbTmpFile, sizeof(EspFsHeader));

		if (ESPFS_MAGIC_HEADER != tEspFileHeader.magic)
		{
#ifdef ESPFS_DEBUG
			os_printf("espFsOpen: magic mismatch - file system image broken. found 0x%x instead\n", tEspFileHeader.magic);
#endif
			ptEspFile = NULL;
			goto lblCleanup;
		}

		if (tEspFileHeader.flags & FLAG_LASTFILE)
		{
#ifdef ESPFS_DEBUG
			os_printf("espFsOpen: end of image reached\n");
#endif
			ptEspFile = NULL;
			goto lblCleanup;
		}
		
		/* acquire file name */
		pbTmpFile += sizeof(EspFsHeader);
		os_memcpy(pszFileNameBuffer, pbTmpFile, sizeof(pszFileNameBuffer));

#ifdef ESPFS_DEBUG
		os_printf("espFsOpen: found file '%s'\nname length = %x, file length compressed = %x, compression = %d flags = %d\n",
				  pszFileNameBuffer, 
				  (unsigned int)tEspFileHeader.nameLen, 
				  (unsigned int)tEspFileHeader.fileLenComp, 
				  tEspFileHeader.compression, 
				  tEspFileHeader.flags);
#endif
		if (0 == os_strcmp(pszFileNameBuffer, pszFilename))
		{
			/* desired file */

			/* skip to content */
			pbTmpFile += tEspFileHeader.nameLen;

			/* allocate file descriptor */
			ptEspFile = (EspFsFile *)os_malloc(sizeof(EspFsFile));

#ifdef ESPFS_DEBUG
			os_printf("espFsOpen: file descriptor allocated at %p\n", ptEspFile);
#endif
			if (NULL == ptEspFile)
			{
				goto lblCleanup;
			}

			/* fill file descriptor */
			ptEspFile->header = (EspFsHeader *)pbHeaderOffset;
			ptEspFile->decompressor = tEspFileHeader.compression;
			ptEspFile->posComp = pbTmpFile;
			ptEspFile->posStart = pbTmpFile;
			ptEspFile->posDecomp = 0;
			
			if (COMPRESS_NONE == tEspFileHeader.compression)
			{
				ptEspFile->decompData = NULL;

#ifdef EFS_HEATSHRINK
			}
			else if (COMPRESS_HEATSHRINK == tEspFileHeader.compression)
			{
				/* compression used */
				char 					bDecoderParameter 	= { 0 };
				heatshrink_decoder *	ptDecoder 			= NULL;
				
				/* acquire decoder parameters - 1st byte */
				memcpyAligned(&bDecoderParameter, ptEspFile->posComp, 1);

				++ptEspFile->posComp;

#ifdef HEATSHRINK_DEBUG
				os_printf("espFsOpen: heatshrink compressed file, decoder parameters = %x\n", bDecoderParameter);
#endif
				ptDecoder = heatshrink_decoder_alloc(16, (bDecoderParameter >> 4) & 0xf, bDecoderParameter & 0xf);
				ptEspFile->decompData = ptDecoder;
#endif
			}
			else
			{
#ifdef HEATSHRINK_DEBUG
				os_printf("espFsOpen: invalid compression %d\n", tEspFileHeader.compression);
#endif
				ptEspFile = NULL;
				goto lblCleanup;
			}
			
			goto lblCleanup;
		}
		
		/* skip file */
		pbTmpFile += tEspFileHeader.nameLen + tEspFileHeader.fileLenComp;
		
		if ((int)pbTmpFile & 3)
		{
			/* align to 32 */
			pbTmpFile += 4 - ((int)pbTmpFile & 3);
		}
	}

lblCleanup:
	return ptEspFile;
}

/* espFsRead */
int ICACHE_FLASH_ATTR espFsRead(EspFsFile *	ptFile, char *	pbBuffer, int 	cbBufferSize)
{
	/* initialization */
	int 	cbFileLength 	= 0;
	int 	iRet 			= ESPFS_READ_STATUS_FAILED;

	if (NULL == ptFile)
	{
		goto lblCleanup;
	}

	/* cache file length */
	memcpyAligned((char *)&cbFileLength, (char *)&ptFile->header->fileLenComp, 4);
	
	/* act according to compression type used */
	if (COMPRESS_NONE == ptFile->decompressor)
	{
		int toRead = 0;
		
		toRead = cbFileLength - (ptFile->posComp-ptFile->posStart);
		
		/* limit read */
		if (cbBufferSize > toRead)
		{
			cbBufferSize = toRead;
		}

#ifdef ESPFS_DEBUG
		os_printf("espFsRead: reading %d bytes from %x\n", cbBufferSize, (unsigned int)ptFile->posComp);
#endif
		
		memcpyAligned(pbBuffer, ptFile->posComp, cbBufferSize);
		ptFile->posDecomp += cbBufferSize;
		ptFile->posComp += cbBufferSize;

#ifdef ESPFS_DEBUG
		os_printf("espFsRead: done reading %d bytes; position %x\n", cbBufferSize, ptFile->posComp);
#endif

		iRet = cbBufferSize;
		goto lblCleanup;

#ifdef EFS_HEATSHRINK
	}
	else if (COMPRESS_HEATSHRINK == ptFile->decompressor)
	{
		int 					cbDecoded 			= 0;
		unsigned int 			uiRemainderLength	= 0;
		unsigned int 			cbReadBytesLength	= 0;
		char 					pbEncodedBuffer[16]	= { 0 };
		heatshrink_decoder *	ptDecoder 			= (heatshrink_decoder *)ptFile->decompData;

#ifdef ESPFS_DEBUG
		os_printf("espFsRead: allocating %p\n", ptDecoder);
#endif

		/* decompress data */
		while (cbDecoded < cbBufferSize)
		{
			HSD_poll_res	iHSDPollStatus = 0;

			uiRemainderLength = cbFileLength - (ptFile->posComp - ptFile->posStart);

			if (0 == uiRemainderLength)
			{
				/* compelted reading file */
				iRet = cbDecoded;
				goto lblCleanup;
			}
			if (0 < uiRemainderLength)
			{
				HSD_sink_res	iHSDSinkStatus = 0;
				/* copy data to decompress */
				memcpyAligned(pbEncodedBuffer, ptFile->posComp, 16);

				/* decompress */
				iHSDSinkStatus = heatshrink_decoder_sink(ptDecoder, (uint8_t *)pbEncodedBuffer, (uiRemainderLength > 16) ? 16 : uiRemainderLength, &cbReadBytesLength);
				if (HSD_SINK_FAILED(iHSDSinkStatus))
				{
					/* heatshrink decoder failed */
					iRet = ESPFS_READ_HSD_SINK_FAILED;
					goto lblCleanup;
				}

				/* update count of bytes read */
				ptFile->posComp += cbReadBytesLength;

				/* check if completed amount of bytes requested */
				if (cbReadBytesLength == uiRemainderLength)
				{
					HSD_finish_res	iHSDFinishStatus = 0;

					iHSDFinishStatus = heatshrink_decoder_finish(ptDecoder);
					if (HSD_FINISH_FAILED(iHSDFinishStatus))
					{
						/* heatshrink decoder failed */
						iRet = ESPFS_READ_HSD_FINISH_FAILED;
						goto lblCleanup;
					}
				}
			}
			
			/* move decompressed data to pbBuffer */
			iHSDPollStatus = heatshrink_decoder_poll(ptDecoder, (uint8_t *)pbBuffer, cbBufferSize - cbDecoded, &cbReadBytesLength);
			if (HSD_POLL_FAILED(iHSDPollStatus))
			{
				/* heatshrink decoder failed */
				iRet = ESPFS_READ_HSD_POLL_FAILED;
				goto lblCleanup;
			}

			/* update status */
			ptFile->posDecomp += cbReadBytesLength;
			pbBuffer += cbReadBytesLength;
			cbDecoded += cbReadBytesLength;
		}

		iRet = cbBufferSize;
		goto lblCleanup;
#endif
	}

	iRet = ESPFS_READ_STATUS_FAILED;

lblCleanup:
	return iRet;
}

/* espFsClose */
void ICACHE_FLASH_ATTR espFsClose(EspFsFile *	ptFile)
{
	/* validate parameters */
	if (NULL == ptFile)
	{
		goto lblCleanup;
	}

#ifdef EFS_HEATSHRINK
	if (COMPRESS_HEATSHRINK == ptFile->decompressor)
	{
		heatshrink_decoder *	ptDecoded = (heatshrink_decoder *)ptFile->decompData;
		heatshrink_decoder_free(ptDecoded);
#ifdef ESPFS_DEBUG
		os_printf("espFsClose: freed %p\n", ptDecoded);
#endif
	}
#endif

	os_free(ptFile);
	
#ifdef ESPFS_DEBUG
	os_printf("espFsClose: freed %p\n", ptFile);
#endif

lblCleanup:
	return;
}
