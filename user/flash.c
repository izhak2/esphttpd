/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		flash.c
*	purpose:	flash memory helper functions implementation
************************************************/

#include "flash.h"

#ifdef OTA
Partition partition[6]	= {
	{PARTITION_0, PARTITION_SIZE_0},
	{PARTITION_1, PARTITION_SIZE_1},
	{PARTITION_2, PARTITION_SIZE_2},
	{PARTITION_3, PARTITION_SIZE_3},
	{PARTITION_4, PARTITION_SIZE_4},
	{PARTITION_5, PARTITION_SIZE_5}
};

#else

Partition partition[4]	= {
	{PARTITION_0, PARTITION_SIZE_0},
	{PARTITION_1, PARTITION_SIZE_1},
	{PARTITION_2, PARTITION_SIZE_2},
	{PARTITION_3, PARTITION_SIZE_3}
};
#endif

int cbFlashedSize = 0;

/* reset_flash */
void reset_flash()
{
	cbFlashedSize = 0;
}

/* erase_partition */
void ICACHE_FLASH_ATTR erase_partition(int iPart)
{
	/* initialization */
	int iPartitionOffset	= partition[iPart].iOffset;
	int cbPartitionSize 	= partition[iPart].cbSize;
	int iIndex				= 0;

#ifdef FLASH_DEBUG
	os_printf("erase_partition: erasing partition %d at offset 0x%x\n", iPart, iPartitionOffset);
#endif

	// Which segment are we flashing?	
	for (iIndex = 0; iIndex < cbPartitionSize; iIndex += SPI_FLASH_SEC_SIZE)
	{
		spi_flash_erase_sector((iPartitionOffset + iIndex) / SPI_FLASH_SEC_SIZE);
	}

#ifdef FLASH_DEBUG
	os_printf("erase_partition: completed erasing\n");
#endif
}

/* erase_block */
void ICACHE_FLASH_ATTR erase_block(int iAddress)
{
	if (0 == (iAddress % SPI_FLASH_SEC_SIZE))
	{
		/* erase block */

#ifdef FLASH_DEBUG
		os_printf("erase_block: erasing flash at 0x%x\n", iAddress / SPI_FLASH_SEC_SIZE);
#endif

		spi_flash_erase_sector(iAddress / SPI_FLASH_SEC_SIZE);
	}
}

/* flash_binary */
int ICACHE_FLASH_ATTR flash_binary(char *	pbData, 
								   int 		cbSize, 
								   int 		iPartition)
{
	/* initialization */
	SpiFlashOpResult 	eFlashOperationStatus 	= SPI_FLASH_RESULT_ERR;
	int 				iRet 					= 0;
	
	erase_block(partition[iPartition].iOffset + cbFlashedSize);
	
	/* source assumed to be 4 byte aligned */
	eFlashOperationStatus = spi_flash_write((partition[iPartition].iOffset + cbFlashedSize), 
											(uint32 *)pbData, 
											cbSize);
	if (SPI_FLASH_RESULT_OK == eFlashOperationStatus)
	{
		cbFlashedSize += cbSize;
		iRet = cbSize;
		goto lblCleanup;
	}
	else
	{

#ifdef FLASH_DEBUG
		os_printf("flash_binary: SpiFlashOpResult %d\n", iRet);
#endif

		iRet = 0;
		goto lblCleanup;
	}

lblCleanup:
	return iRet;
}

/* get_updatable_partition */
int get_updatable_partition()
{
	/* initialization */
	int iRet = 0;

#ifndef OTA
	iRet = 1;
#endif

	if (0 == system_upgrade_userbin_check())
	{
		iRet = 4;
	}
	else
	{
		iRet = 1;
	}

	return iRet;
}
