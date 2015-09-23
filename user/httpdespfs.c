/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		httpdespfs.c
*	purpose:	interface for httpd to use the espfs
*				filesystem to access files
************************************************/

#include "httpdespfs.h"

/*	cgiEspFsHook	*/
int ICACHE_FLASH_ATTR	cgiEspFsHook(HttpdConnection *	ptConnection)
{
	/* initialization */
	EspFsFile *	ptFile 					= ptConnection->pbCgiData;
	int 		iRet					= 0;
	int 		cbReadBytesCount		= 0;
	char 		pbFileReadBuffer[1024]	= { 0 };
	
	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		espFsClose(ptFile);
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	if (NULL == ptFile)
	{
		/* first call to cgi, open file for read */
		ptFile = espFsOpen(ptConnection->pszUrl);
		if (NULL == ptFile)
		{
			iRet = HTTPD_CGI_NOTFOUND;
			goto lblCleanup;
		}

		ptConnection->pbCgiData = ptFile;
		httpdStartResponse(ptConnection, 200);
		httpdHeader(ptConnection, "Content-Type", httpdGetMimetype(ptConnection->pszUrl));
		httpdHeader(ptConnection, "Cache-Control", "max-age=3600, must-revalidate");
		httpdEndHeaders(ptConnection);
		
		iRet = HTTPD_CGI_MORE;
		goto lblCleanup;
	}

	/* read 1024 bytes chunk from file */
	cbReadBytesCount = espFsRead(ptFile, pbFileReadBuffer, 1024);
	if (0 < cbReadBytesCount)
	{
		espconn_sent(ptConnection->ptEspConnection, (uint8 *)pbFileReadBuffer, cbReadBytesCount);
	}
	
	/* completed serving file */
	if (1024 != cbReadBytesCount)
	{
		espFsClose(ptFile);
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}
	else
	{
		/* continue for more */
		iRet = HTTPD_CGI_MORE;
		goto lblCleanup;
	}

lblCleanup:
	return iRet;
}

/*	cgiEspFsTemplate	*/
int ICACHE_FLASH_ATTR	cgiEspFsTemplate(HttpdConnection *	ptConnection)
{
	/* initialization */
	TplData *	tTplData 				= NULL;
	int 		cbReadBytesCount		= 0;
	int 		iIndex					= 0;
	int 		iEscapeIndex 			= 0;
	int 		iRet	 				= 0;
	char *		pbTmpReadBuffer			= NULL;
	char 		pbFileReadBuffer[1025]	= { 0 };

	tTplData = ptConnection->pbCgiData;

	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		((TplCallback)(ptConnection->pbCgiArguments))(ptConnection, NULL, &tTplData->tplArg);
		
		espFsClose(tTplData->file);
		os_free(tTplData);
		
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	if (NULL == tTplData)
	{
		/* first call to this cgi - open file for read */
		tTplData = (TplData *)os_malloc(sizeof(TplData));
		tTplData->file = espFsOpen(ptConnection->pszUrl);
		tTplData->tplArg = NULL;
		tTplData->tokenPos = -1;

		if (NULL == tTplData->file)
		{
			iRet = HTTPD_CGI_NOTFOUND;
			goto lblCleanup;
		}

		ptConnection->pbCgiData = tTplData;
		httpdStartResponse(ptConnection, 200);
		httpdHeader(ptConnection, "Content-Type", httpdGetMimetype(ptConnection->pszUrl));
		httpdEndHeaders(ptConnection);
		
		iRet = HTTPD_CGI_MORE;
		goto lblCleanup;
	}

	/* read 1024 bytes chunk from file */
	cbReadBytesCount = espFsRead(tTplData->file, pbFileReadBuffer, 1024);

	if (0 < cbReadBytesCount)
	{
		iEscapeIndex = 0;
		pbTmpReadBuffer = pbFileReadBuffer;

		/* traverse through file data */
		for (iIndex = 0; iIndex < cbReadBytesCount; ++iIndex)
		{
			/* find first % character */
			if (-1 == tTplData->tokenPos)
			{
				if ((*SINGLE_PRECENTAGE_SYMBOL_STRING) == pbFileReadBuffer[iIndex])
				{
					/* send raw data up to first % */
					if (0 != iEscapeIndex)
					{
						httpdSend(ptConnection, pbTmpReadBuffer, iEscapeIndex);
					}

					iEscapeIndex = 0;

					/* mark to begin token chars collecting */
					tTplData->tokenPos = 0;
				}
				else
				{
					/* skip, % character not found yet */
					iEscapeIndex++;
				}
			}
			/* % character already found */ 
			else
			{
				if ((*SINGLE_PRECENTAGE_SYMBOL_STRING) == pbFileReadBuffer[iIndex])
				{
					if (0 == tTplData->tokenPos)
					{
						/* %% escape string - send a single % and resume flow */
						httpdSend(ptConnection, SINGLE_PRECENTAGE_SYMBOL_STRING, 1);
					} 
					else
					{
						/* zero-terminate token */
						tTplData->token[tTplData->tokenPos++] = 0;
						/* dispatch callback with token */
						((TplCallback)(ptConnection->pbCgiArguments))(ptConnection, tTplData->token, &tTplData->tplArg);
					}

					/* update content buffer index */
					pbTmpReadBuffer = &pbFileReadBuffer[iIndex + 1];

					/* reset % counter */
					tTplData->tokenPos = -1;
				}
				else
				{
					/* fill token with content */
					if ((sizeof(tTplData->token) - 1) > tTplData->tokenPos)
					{
						tTplData->token[tTplData->tokenPos++] = pbFileReadBuffer[iIndex];
					}
				}
			}
		}
	}
	
	/* send remaining data */
	if (0 != iEscapeIndex)
	{
		httpdSend(ptConnection, pbTmpReadBuffer, iEscapeIndex);
	}
	if (1024 != cbReadBytesCount)
	{
		/* complete callback call */
		((TplCallback)(ptConnection->pbCgiArguments))(ptConnection, NULL, &tTplData->tplArg);
		espFsClose(tTplData->file);
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}
	else
	{
		/* continue next time */
		iRet = HTTPD_CGI_MORE;
		goto lblCleanup;
	}

lblCleanup:
	return iRet;
}
