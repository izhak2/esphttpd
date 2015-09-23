/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		espfs.h
*	purpose:	espfs support header
************************************************/

#ifndef __ESPFS_H__
#define __ESPFS_H__

#include "shared.h"

/* the following routines can also be tested by comping them in with the espfstest tool to simplify debugging */

#ifdef 	ESPFS_TEST
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define os_malloc	malloc
#define os_free		free
#define os_memcpy	memcpy
#define os_strncmp	strncmp
#define os_strcmp	strcmp
#define os_strcpy	strcpy
#define os_printf	printf
#define ICACHE_FLASH_ATTR
#else
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "flash.h"
#endif

#include "../mkespfsimage/espfsformat.h"

#ifdef EFS_HEATSHRINK
#include "../lib/heatshrink/heatshrink_decoder.h"
#endif //EFS_HEATSHRINK

/* flash is mapped to 0x40200000 and required aligned access, otherwise crashed */
#define	ESP_FLASH_OFFSET	(0x40200000)
#define	ESPFS_MAGIC_HEADER	(0x73665345)
#define	SLASH_SYMBOL_STRING	'/'

typedef struct _EspFsFile 
{
	EspFsHeader *	header;
	char 			decompressor;
	int32_t 		posDecomp;
	char *			posStart;
	char *			posComp;
	void *			decompData;
} EspFsFile;

typedef enum 
{
	ESPFS_READ_STATUS_FAILED = 0,
	ESPFS_READ_HSD_POLL_FAILED,
	ESPFS_READ_HSD_SINK_FAILED,
	ESPFS_READ_HSD_FINISH_FAILED
} ESPFS_READ_STATUS;

/************************************************
*	name:			espFsOpen
*	parameters:		pszFileName - file name
*	return value:	pointer to the file descriptor
*	purpose:		open file
************************************************/
EspFsFile *	espFsOpen(char *	pszFileName);

/************************************************
*	name:			espFsRead
*	parameters:		ptFile 			- file to read from
*					pbBuffer 		- buffer to fill with file content
*					cbBufferSize	- amount of bytes to read into buffer
*	return value:	count of bytes read
*	purpose:		read file
************************************************/
int 		espFsRead(EspFsFile *	ptFile, 
					  char *		pbBuffer, 
					  int 			cbBufferSize);

/************************************************
*	name:			espFsClose
*	parameters:		ptFile - file to close
*	return value:	none
*	purpose:		close file
************************************************/
void 		espFsClose(EspFsFile *	ptFile);

#ifdef EFS_HEATSHRINK
#define HSD_SINK_FAILED(_STATUS)	(_STATUS <= HSDR_SINK_ERROR_NULL)
#define HSD_POLL_FAILED(_STATUS)	(_STATUS <= HSDR_POLL_ERROR_NULL)
#define HSD_FINISH_FAILED(_STATUS)	(_STATUS <= HSDR_FINISH_ERROR_NULL)
#endif //EFS_HEATSHRINK

#endif //__ESPFS_H__
