/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		flash.h
*	purpose:	flash memory helper functions header
************************************************/

#ifndef __FLASH_H__
#define __FLASH_H__

#include "shared.h"
#include "osapi.h"

#define	CONTENT_LENGTH_STRING		"Content-Length"

#ifdef OTA
#define PARTITION_0			0x00000 // BOOTLOADER
#define PARTITION_SIZE_0	0x01000 //4KB
#define PARTITION_1			0x01000 //USER APP 1
#define PARTITION_SIZE_1	0x3B000 // 236KB
#define PARTITION_2			0x3C000 //USER PARAMS
#define PARTITION_SIZE_2	0x04000 // 16KB
#define PARTITION_3			0x40000 //RESERVED
#define PARTITION_SIZE_3	0x01000 //4KB
#define PARTITION_4			0x41000 //USER APP2
#define PARTITION_SIZE_4	0x3B000 // 236KB
#define PARTITION_5			0x7C000 //SYSTEM PARAMS
#define PARTITION_SIZE_5	0x04000 // 16KB

#else

#define PARTITION_0			0x00000 // ICACHE - ROM part 1
#define PARTITION_SIZE_0	0x12000 //72KB
#define PARTITION_1			0x12000 // Website storage
#define PARTITION_SIZE_1	0x20000 // 128KB - Hangs if more than that
#define PARTITION_2			0x40000 // IROM
#define PARTITION_SIZE_2	0x3C000 // 240K 
#define PARTITION_3			0x7C000 //SYSTEM PARAMS AND CONFIGS
#define PARTITION_SIZE_3	0x04000 // 16KB
#endif //OTA

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <spi_flash.h>

typedef struct _Partition 
{
	int iOffset;
	int cbSize;
} Partition;

extern Partition partition[];

/************************************************
*	name:			get_updatable_partition
*	parameters:		none
*	return value:	pointer to the file descriptor
*	purpose:		return the updatable partition
************************************************/
int get_updatable_partition();

/************************************************
*	name:			erase_partition
*	parameters:		iPart - partition index
*	return value:	none
*	purpose:		erase partition
************************************************/
void ICACHE_FLASH_ATTR erase_partition(int iPart);

/************************************************
*	name:			erase_block
*	parameters:		iAddress - address which its sector will be deleted
*	return value:	none
*	purpose:		erase sector which given address belongs to
************************************************/
void ICACHE_FLASH_ATTR erase_block(int iAddress);

/************************************************
*	name:			flash_binary
*	parameters:		pbData		- data to flash
*					cbSize		- data size
*					iPartition	- partition index
*	return value:	count of bytes flashed
*	purpose:		flash data
************************************************/
int ICACHE_FLASH_ATTR flash_binary(char *	pbData, 
								   int		cbSize, 
								   int		iPartition);

/************************************************
*	name:			reset_flash
*	parameters:		none
*	return value:	none
*	purpose:		reset counter of how much flashed
************************************************/
void reset_flash();

#endif //__FLASH_H__
