/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		cgiflash.h
*	purpose:	flash CGI routines implementation - rw flash and update ESPFS image
************************************************/

#include "cgiflash.h"

LOCAL os_timer_t flash_reboot_timer	= { 0 };

/*	cgiReadFlash	*/
int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnection *	ptConnection)
{
	/* initialization */
	int 	iRet 			= 0;
	int *	iFlashOffset 	= NULL;

	iFlashOffset = (int *)&ptConnection->pbCgiData;

	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	if (0 == *iFlashOffset)
	{

#ifdef CGIFLASH_DEBUG
		os_printf("cgiReadFlash: begin flash download\n");
#endif

		httpdStartResponse(ptConnection, 200);
		httpdHeader(ptConnection, "Content-Type", "application/bin");
		httpdEndHeaders(ptConnection);

		/* update offset */
		*iFlashOffset = 0x40200000;

		iRet = HTTPD_CGI_MORE;
		goto lblCleanup;
	}
	
	/* send 1K of flash per call, will be called again if haven't sent 512K yet */
	espconn_sent(ptConnection->ptEspConnection, (uint8 *)(*iFlashOffset), 1024);
	*iFlashOffset += 1024;
	if (*iFlashOffset >= 0x40200000 + (512 * 1024))
	{
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}
	else
	{
		iRet = HTTPD_CGI_MORE;
		goto lblCleanup;
	}

lblCleanup:
	return iRet;
}

/*	cgiUploadRaw	*/
int ICACHE_FLASH_ATTR cgiUploadRaw(HttpdConnection *	ptConnection)
{
	int 	iRet	= 0;

	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	int iPartition = get_updatable_partition();
	if ((0 == ptConnection->ptPost->cbProcessed) && (0 != ptConnection->pbGetArguments))
	{
		/* first call - initialize partition and prepare if necessary */
		/* convert to index */
		iPartition = ptConnection->pbGetArguments[0] - '0';

#ifdef CGIFLASH_DEBUG
		os_printf("cgiUploadRaw: flashing partition %d\n", iPartition);
#endif
	}
	
	SpiFlashOpResult cbFlashedBytes;

	/* source should be 4 byte aligned */
	cbFlashedBytes = flash_binary(ptConnection->ptPost->pbBuffer, ptConnection->ptPost->cbBufferLength, iPartition);
	if (cbFlashedBytes != ptConnection->ptPost->cbBufferLength)
	{

#ifdef CGIFLASH_DEBUG
		os_printf("cgiUploadRaw: error - wrote less bytes than required\n");
#endif

		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	/* update flashed bytes count */
	ptConnection->ptPost->cbProcessed += cbFlashedBytes;

#ifdef CGIFLASH_DEBUG
	os_printf("cgiUploadRaw: wrote %d bytes (%d of %d)\n", ptConnection->ptPost->cbMaxBufferSize, ptConnection->ptPost->cbProcessed, ptConnection->ptPost->cbPostLength);
#endif

	/* check if reached write target */
	if (ptConnection->ptPost->cbProcessed == ptConnection->ptPost->cbPostLength)
	{
		httpdSend(ptConnection, FINISHED_UPLOADING_STRING, -1);
		reset_flash();
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}
	else
	{
		iRet = HTTPD_CGI_MORE;
		goto lblCleanup;
	}

lblCleanup:
	return iRet;
}

/*	cgiUpgradeRaw	*/
int ICACHE_FLASH_ATTR cgiUpgradeRaw(HttpdConnection *	ptConnection)
{
	/* initialization */
	int iRet = 0;

	iRet = cgiUploadRaw(ptConnection);

	if (HTTPD_CGI_DONE == iRet)
	{
		/* once completed flashing */
		system_upgrade_flag_set(UPGRADE_FLAG_FINISH);

		/* schedule reboot */
		os_timer_disarm(&flash_reboot_timer);
		os_timer_setfn(&flash_reboot_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
		os_timer_arm(&flash_reboot_timer, 2000, 1);
		
	}
	return iRet;
}

/************************************************
*	name:			bin_strstr
*	parameters:		pbHaystack 			-	haystack
*					pbNeedle 			-	needle
*					cbHaystackLength	-	pbHaystack length
*					cbNeedleLength		-	pbNeedle length
*	return value:	position, otherwise NULL
*	purpose:		strstr for binary
************************************************/
char * bin_strstr(char *	pbHaystack, 
				  char *	pbNeedle, 
				  int 		cbHaystackLength, 
				  int 		cbNeedleLength)
{
	/* initialization */
	char *	pbReturn		= NULL;
	char * 	pbHaystackEnd 	= pbHaystack + cbHaystackLength;
	int 	iIndex			= 1;
	int 	iMatch			= false;

	/* validate length */
	if(0 > cbNeedleLength)
	{
		cbNeedleLength = strlen(pbNeedle);
	}
	
	for(; pbHaystack < pbHaystackEnd; ++pbHaystack)
	{
		/* check that needle is still in boundaries - I added check for safety*/
		if (pbNeedle + cbNeedleLength < pbHaystackEnd)
		{
			break;
		}

		/* check for first match */
		if (*pbHaystack == *pbNeedle)
		{
			/* first match */
			iMatch = true;

			/* make sure whole needle matches */
			for(iIndex = 1; iIndex < cbNeedleLength; ++iIndex)
			{
				if(*(pbNeedle + iIndex) != *(pbHaystack + iIndex))
				{
					/* update match status on fail */
					iMatch = false;
					break;
				}
			}

			if(true == iMatch)
			{
				pbReturn = pbHaystack;
				goto lblCleanup;
			}
		}
	}

lblCleanup:
	return pbReturn;
}

/*	cgiGetAppVer	*/
int ICACHE_FLASH_ATTR cgiGetAppVer(HttpdConnection *	ptConnection)
{
	/* initialization */
	int iRet = 0;

	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}
	
	char bVersion[1] = { 0 };

	/* set and send app version */
	os_sprintf(bVersion, "%d", system_upgrade_userbin_check() + 1);
	httpdStartResponse(ptConnection, 200);
	httpdHeader(ptConnection, CONTENT_LENGTH_STRING, "1");
	httpdEndHeaders(ptConnection);
	httpdSend(ptConnection, bVersion, 1);

	iRet = HTTPD_CGI_DONE;

lblCleanup:
	return iRet;
}
