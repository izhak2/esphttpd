/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		httpd.c
*	purpose:	HTTP server core routines
************************************************/

#include "httpd.h"

/* http connection struct passed to cgi functions */
typedef struct _HttpdPrivate
{
	char	pbHeader[HTTPD_MAX_HEADER_LENGTH];
	int 	cbHeaderLength;
	char *	pbSendBuffer;
	int 	cbSendBufferLength;
} HttpdPrivate;


static HttpdUrlDescriptor *	g_ptSupportedUrls	= NULL;

static HttpdPrivate			g_ptConnectionPrivateData[HTTPD_MAX_CONNECTIONS]	= { 0 };
static HttpdConnection 		g_ptConnectionData[HTTPD_MAX_CONNECTIONS]			= { 0 };
static HttpdPost 			g_ptConnectionPost[HTTPD_MAX_CONNECTIONS]			= { 0 };

/* Listening connection data */
static struct espconn 		g_tConnection 	= { 0 };
static esp_tcp 				g_tTcp			= { 0 };

/* File extensions to mime types mapping */
static const HttpdMimeMap g_ptMimeTypes[] = {
	{"htm",		"text/htm"},
	{"html",	"text/html"},
	{"css",		"text/css"},
	{"js",		"text/javascript"},
	{"txt",		"text/plain"},
	{"jpg",		"image/jpeg"},
	{"jpeg",	"image/jpeg"},
	{"png",		"image/png"},
	{NULL,		"text/html"}, 	// Default
};

/*	httpdGetMimetype	*/
const char ICACHE_FLASH_ATTR *	httpdGetMimetype(char *	pszUrl) 
{
	/* initialization */
	int 	iIndex 		 = 0;
	char *	pszExtension = pszUrl + (strlen(pszUrl) - 1);

	/* advance buffer to extension */
	for(; (pszExtension != pszUrl) && (*EXTENSION_SYMBOL_STRING != *pszExtension); --pszExtension);

	if (*EXTENSION_SYMBOL_STRING == *pszExtension)
	{
		pszExtension++;
	}

	/* match url to mime type - case sensitive - to fix */
	while ((NULL != g_ptMimeTypes[iIndex].pszExtension) && (0 != os_strcmp(pszExtension, g_ptMimeTypes[iIndex].pszExtension)))
	{
		iIndex++;
	}

	return g_ptMimeTypes[iIndex].pszMimeType;
}

/************************************************
*	name:			httpdFindConnData
*	parameters:		ptEspConnection - connection looking for
*	return value:	HttpdConnection for connection argument
*	purpose:		search g_ptConnectionData for a specific connection
************************************************/
static HttpdConnection ICACHE_FLASH_ATTR *	httpdFindConnData(void * ptEspConnection)
{
	/* initialization */
	int 				iIndex				= 0;
	HttpdConnection * 	ptHttpdConnection	= NULL;

	for (iIndex = 0; HTTPD_MAX_CONNECTIONS > iIndex; ++iIndex) 
	{
		if (g_ptConnectionData[iIndex].ptEspConnection == (struct espconn *)ptEspConnection)
		{
			ptHttpdConnection = &g_ptConnectionData[iIndex];
			break;
		}
	}

	if (NULL == ptHttpdConnection)
	{

#ifdef HTTPD_DEBUG
		os_printf("httpdFindConnData: couldn't find connection %p\n", ptEspConnection);
#endif		
	}

	return ptHttpdConnection;
}

/************************************************
*	name:			httpdRetireConn
*	parameters:		ptHttpdConnection - conection to retire
*	return value:	none
*	purpose:		retires a connection for re-use
************************************************/
static void ICACHE_FLASH_ATTR	httpdRetireConn(HttpdConnection *	ptHttpdConnection)
{
	if (NULL != ptHttpdConnection->ptPost->pbBuffer)
	{
		os_free(ptHttpdConnection->ptPost->pbBuffer);
	}

	ptHttpdConnection->ptPost->pbBuffer = NULL;
	ptHttpdConnection->pfnCgi = NULL;
	ptHttpdConnection->ptEspConnection = NULL;
}

/************************************************
*	name:			httpdHexVal
*	parameters:		bChar - character to convert
*	return value:	hex value
*	purpose:		hex of char
************************************************/
static int httpdHexVal(char bChar)
{
	if (bChar >= '0' && bChar <= '9') return bChar-'0';
	if (bChar >= 'A' && bChar <= 'F') return bChar-'A' + 10;
	if (bChar >= 'a' && bChar <= 'f') return bChar-'a' + 10;
	return 0;
}

/*	httpdUrlDecode	*/
int httpdUrlDecode(char *	pbEncodedBuffer,
				   int 		cbEncodedBufferLength,
				   char *	pbDecodedBuffer,
				   int 		cbDecodedBufferLength)
{
	/* initialization */
	int iSourceIndex 		= 0;
	int iDestinationIndex 	= 0;
	int iEscapeFlag 		= 0;
	int iEscapedValue 		= 0;

	while ((iSourceIndex < cbEncodedBufferLength) && (iDestinationIndex < cbDecodedBufferLength))
	{
		if (1 == iEscapeFlag)
		{
			iEscapedValue = httpdHexVal(pbEncodedBuffer[iSourceIndex]) << 4;
			iEscapeFlag = 2;
		}
		else if (2 == iEscapeFlag)
		{
			iEscapedValue += httpdHexVal(pbEncodedBuffer[iSourceIndex]);
			pbDecodedBuffer[iDestinationIndex++] = iEscapedValue;
			iEscapeFlag = 0;
		}
		else if ((*PERCENTAGE_SYMBOL_STRING) == pbEncodedBuffer[iSourceIndex])
		{
			iEscapeFlag = 1;
		}
		else if ((*PLUS_SYMBOL_STRING) == pbEncodedBuffer[iSourceIndex])
		{
			pbDecodedBuffer[iDestinationIndex++] = *SPACE_SYMBOL_STRING;
		}
		else
		{
			pbDecodedBuffer[iDestinationIndex++] = pbEncodedBuffer[iSourceIndex];
		}

		iSourceIndex++;
	}
	if (iDestinationIndex < cbDecodedBufferLength)
	{
		pbDecodedBuffer[iDestinationIndex] = 0;
	}
	return iDestinationIndex;
}

/*	httpdFindArg	*/
int ICACHE_FLASH_ATTR	httpdFindArg(char *	pszLine,
									 char *	pszArgument,
									 char *	pszBuffer,
									 int 	cbBufferLength)
{
	/* initialization */
	int 	iRet 	 			= -1;
	char *	pbBeginningOfArg	= NULL;
	char *	pbEndOfArg 			= NULL;

	if (NULL == pszLine)
	{
		iRet = 0;
		goto lblCleanup;
	}

	pbBeginningOfArg = pszLine;

	while ((NULL_STRING != pbBeginningOfArg) 				&& 
		  ((*NEWLINE_SYMBOL_STRING) != *pbBeginningOfArg) 	&& 
		  ((*NEWLINE_R_SYMBOL_STRING) != *pbBeginningOfArg)	&& 
		  (NULL_STRING != *pbBeginningOfArg))
	{

#ifdef HTTPD_DEBUG
		os_printf("httpdFindArg: %s\n", pbBeginningOfArg);
#endif
		if ((0 == os_strncmp(pbBeginningOfArg, pszArgument, os_strlen(pszArgument))) && ((*EQUAL_SYMBOL_STRING) == pbBeginningOfArg[strlen(pszArgument)]))
		{
			pbBeginningOfArg += os_strlen(pszArgument) + 1; //move pbBeginningOfArg to start of value
			pbEndOfArg = (char *)os_strstr(pbBeginningOfArg, AMPERSAND_SYMBOL_STRING);
			
			if (NULL == pbEndOfArg)
			{
				pbEndOfArg = pbBeginningOfArg + os_strlen(pbBeginningOfArg);
			}

#ifdef HTTPD_DEBUG
			os_printf("httpdFindArg: value: %s length: %d\n", pbBeginningOfArg, (pbEndOfArg - pbBeginningOfArg));
#endif
			
			iRet = httpdUrlDecode(pbBeginningOfArg, (pbEndOfArg - pbBeginningOfArg), pszBuffer, cbBufferLength);
			goto lblCleanup;
		}

		pbBeginningOfArg = (char *)os_strstr(pbBeginningOfArg, AMPERSAND_SYMBOL_STRING);
		if (NULL != pbBeginningOfArg)
		{
			pbBeginningOfArg += 1;
		}
	}

	/* not found */	

#ifdef HTTPD_DEBUG
	os_printf("httpdFindArg: didn't find %s in %s\n", pszArgument, pszLine);
#endif

lblCleanup:
	return iRet;
}

/* httpdGetHeader */
int ICACHE_FLASH_ATTR	httpdGetHeader(HttpdConnection *	ptConnection,
									   char *				pszHeader,
									   char *				pszOutputBuffer,
									   int 					cbOutputBufferLength)
{
	/* initialization */
	int 	iRet			= 0;
	char *	pszTmpHeader 	= ptConnection->ptPrivate->pbHeader;

	/* skip GET/POST part */
	pszTmpHeader = pszTmpHeader + strlen(pszTmpHeader) + 1;

	/* skip HTTP part */
	pszTmpHeader = pszTmpHeader + strlen(pszTmpHeader) + 1;

	while (pszTmpHeader < (ptConnection->ptPrivate->pbHeader + ptConnection->ptPrivate->cbHeaderLength))
	{
		while ((32 >= *pszTmpHeader) && (0 != *pszTmpHeader))
		{
			/* skip start */
			pszTmpHeader++;
		}

		/* check if header */
		if ((0 == os_strncmp(pszTmpHeader, pszHeader, strlen(pszHeader))) && ((*COLON_SYMBOL_STRING) == pszTmpHeader[strlen(pszHeader)]))
		{
			/* skip 'key:' bit of header line */
			pszTmpHeader = pszTmpHeader + strlen(pszHeader) + 1;
			
			/* skip past spaces after the colon */
			while ((*SPACE_SYMBOL_STRING) == *pszTmpHeader)
			{
				pszTmpHeader++;
			}

			/* copy */
			while ((NULL_STRING != *pszTmpHeader) && ((*NEWLINE_R_SYMBOL_STRING) != *pszTmpHeader) && ((*NEWLINE_SYMBOL_STRING) != *pszTmpHeader) && (1 < cbOutputBufferLength))
			{
				*pszOutputBuffer++ = *pszTmpHeader++;
				cbOutputBufferLength--;
			}

			/* zero-terminate string */
			*pszOutputBuffer = 0;
			
			/* complete */
			iRet = 1;
			goto lblCleanup;
		}

		/* skip past end of string and \0 terminator */
		pszTmpHeader += strlen(pszTmpHeader) + 1; 
	}

lblCleanup:
	return iRet;
}

/*	httpdStartResponse	*/
void ICACHE_FLASH_ATTR	httpdStartResponse(HttpdConnection *	ptConnection,
										   int 					iCode)
{
	/* initialization */
	char 	pszResponseBuffer[128]	= { 0 };
	int 	cbResponseBufferLength	= 0;

	cbResponseBufferLength = os_sprintf(pszResponseBuffer, "HTTP/1.0 %d OK\r\nServer: esp8266-httpd/"HTTPD_VERSION_STRING"\r\n", iCode);

	httpdSend(ptConnection, pszResponseBuffer, cbResponseBufferLength);
}

/*	httpdHeader		*/
void ICACHE_FLASH_ATTR	httpdHeader(HttpdConnection *	ptConnection,
									const char *		pszField,
									const char *		pszValue)
{
	/* initialization */
	char 	pszHeaderBuffer[256]	= { 0 };
	int 	cbHeaderBufferLength 	= 0;

	cbHeaderBufferLength = os_sprintf(pszHeaderBuffer, "%s: %s\r\n", pszField, pszValue);

	httpdSend(ptConnection, pszHeaderBuffer, cbHeaderBufferLength);
}

/*	httpdEndHeaders	*/
void ICACHE_FLASH_ATTR httpdEndHeaders(HttpdConnection *	ptConnection)
{
	httpdSend(ptConnection, "\r\n", -1);
}

/*	httpdRedirect	*/
void ICACHE_FLASH_ATTR	httpdRedirect(HttpdConnection *	ptConnection,
									  char *			pszNewURL)
{
	/* initialization */
	char 	pszRedirectMessageBuffer[1024]	= { 0 };
	int 	cbRedirectMessageBufferLength 	= 0;

	cbRedirectMessageBufferLength = os_sprintf(pszRedirectMessageBuffer, "HTTP/1.1 302 Found\r\nLocation: %s\r\n\r\nMoved to %s\r\n", pszNewURL, pszNewURL);

	httpdSend(ptConnection, pszRedirectMessageBuffer, cbRedirectMessageBufferLength);
}

/*	cgiRedirect 	*/
int ICACHE_FLASH_ATTR	cgiRedirect(HttpdConnection *	ptConnection)
{
	/* initialization */
	int 	iRet = HTTPD_CGI_DONE;
	
	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		goto lblCleanup;
	}

	httpdRedirect(ptConnection, (char *)ptConnection->pbCgiArguments);

lblCleanup:
	return iRet;
}

/*	httpdSend	*/
int ICACHE_FLASH_ATTR	httpdSend(HttpdConnection *	ptConnection,
								  const char *		pbData,
								  int 				cbDataLength)
{
	/* initialization */
	int 	iRet = 0;

	if (0 > cbDataLength)
	{
		cbDataLength = strlen(pbData);
	}

	if (ptConnection->ptPrivate->cbSendBufferLength+cbDataLength>HTTPD_MAX_SEND_BUFFER_LENGTH)
	{
		goto lblCleanup;
	}
	
	os_memcpy(ptConnection->ptPrivate->pbSendBuffer + ptConnection->ptPrivate->cbSendBufferLength, pbData, cbDataLength);

	ptConnection->ptPrivate->cbSendBufferLength += cbDataLength;
	iRet = 1;

lblCleanup:
	return iRet;
}

/************************************************
*	name:			xmitSendBuff
*	parameters:		ptConnection 	- HTTP connection
*	return value:	none
*	purpose:		send data inside ptEspConnection->ptPrivate->pbSendBuffer
************************************************/
static void ICACHE_FLASH_ATTR	xmitSendBuff(HttpdConnection *	ptConnection)
{
	if (0 != ptConnection->ptPrivate->cbSendBufferLength)
	{
		espconn_sent(ptConnection->ptEspConnection, (uint8_t *)ptConnection->ptPrivate->pbSendBuffer, ptConnection->ptPrivate->cbSendBufferLength);
		ptConnection->ptPrivate->cbSendBufferLength = 0;
	}
}

/************************************************
*	name:			httpdSentCb
*	parameters:		pArgument - connection data
*	return value:	none
*	purpose:		callback when data on socket has been successfully sent
************************************************/
static void ICACHE_FLASH_ATTR	httpdSentCb(void *	pArgument)
{
	/* initialization */
	int 				cgiResult 									= HTTPD_CGI_FAILED;
	char 				pbSendBuffer[HTTPD_MAX_SEND_BUFFER_LENGTH]	= { 0 };
	HttpdConnection *	ptConnection 								= NULL;

	ptConnection = httpdFindConnData(pArgument);

	if (NULL == ptConnection)
	{
		goto lblCleanup;
	}

	ptConnection->ptPrivate->pbSendBuffer = pbSendBuffer;
	ptConnection->ptPrivate->cbSendBufferLength = 0;

	if (NULL == ptConnection->pfnCgi)
	{
		/* marked for destruction - no need to call xmitSendBuff */

#ifdef HTTPD_DEBUG
		os_printf("httpdSendCb: connection %p is done and closing.\n", ptConnection->ptEspConnection);
#endif

		espconn_disconnect(ptConnection->ptEspConnection);
		httpdRetireConn(ptConnection);
		goto lblCleanup;
	}

	/* Execute cgi function */
	cgiResult = ptConnection->pfnCgi(ptConnection);
	if (HTTPD_CGI_DONE == cgiResult)
	{
		/* mark for destruction */
		ptConnection->pfnCgi = NULL;
	}

	xmitSendBuff(ptConnection);

lblCleanup:
	return;
}

/************************************************
*	name:			httpdProcessRequest
*	parameters:		ptConnection - connection data
*	return value:	none
*	purpose:		called when headers have been received and 
*					the connection is ready to send the result headers and data
************************************************/
static void ICACHE_FLASH_ATTR	httpdProcessRequest(HttpdConnection *	ptConnection)
{
	/* initialization */
	int 	iCgiResult = HTTPD_CGI_FAILED;

	/* execute cgi */
	iCgiResult = ptConnection->pfnCgi(ptConnection);
	if (HTTPD_CGI_NOTFOUND != iCgiResult)
	{
		if (HTTPD_CGI_DONE == iCgiResult)
		{
			/* cgi finishes immediately then mark ptConnection for destruction */
			ptConnection->pfnCgi = NULL;
		}
		xmitSendBuff(ptConnection);
	}
	else
	{
		/* didn't find */

#ifdef HTTPD_DEBUG
		os_printf(HTTPD_HEADER_404_ERROR_STRING, ptConnection->pszUrl);
#endif

		httpdSend(ptConnection, HTTPD_HEADER_NOT_FOUND_STRING, -1);
		xmitSendBuff(ptConnection);
		/* mark for destruction */
		ptConnection->pfnCgi = NULL;
	}
}

/************************************************
*	name:			httpdParseHeader
*	parameters:		pszHeader 			- header data
*					ptConnection 	- connection data
*	return value:	none
*	purpose:		parse a line of header data and modify connection data accordingly
************************************************/
static void ICACHE_FLASH_ATTR	httpdParseHeader(char *				pszHeader,
												 HttpdConnection *	ptConnection)
{
	/* initialization */
	int 	iIndex 			= 0;
	char 	bIsFirstLine	= false;

	if (0 == os_strncmp(pszHeader, "GET ", 4))
	{
		ptConnection->bRequestType = HTTPD_METHOD_GET;
		bIsFirstLine = true;
	}
	else if (0 == os_strncmp(pszHeader, "POST ", 5))
	{
		ptConnection->bRequestType = HTTPD_METHOD_POST;
		bIsFirstLine = true;
	}

	if (bIsFirstLine)
	{
		char *	pbEndOfURL = NULL;
		
		/* skip past the space after POST/GET */
		for (iIndex = 0; (*SPACE_SYMBOL_STRING) != pszHeader[iIndex]; ++iIndex);

		ptConnection->pszUrl = pszHeader + iIndex + 1;

		/* figure out end of url */
		pbEndOfURL = (char *)os_strstr(ptConnection->pszUrl, SPACE_SYMBOL_STRING);
		if (NULL == pbEndOfURL)
		{
			return;
		}
		/* terminate url part */
		*pbEndOfURL = 0;

#ifdef HTTPD_DEBUG
		os_printf("httpdParseHeader: URL is %s\n", ptConnection->pszUrl);
#endif
		
		/* parse URL part before the GET parameters */
		ptConnection->pbGetArguments=(char *)os_strstr(ptConnection->pszUrl, QUERY_SYMBOL_STRING);
		if (0 != ptConnection->pbGetArguments)
		{
			*ptConnection->pbGetArguments = 0;
			ptConnection->pbGetArguments++;

#ifdef HTTPD_DEBUG
			os_printf("httpdParseHeader: GET arguments are %s\n", ptConnection->pbGetArguments);
#endif

		}
		else
		{
			ptConnection->pbGetArguments = NULL;
		}

		/* check if url is in url table */
		for (iIndex = 0; (NULL != g_ptSupportedUrls[iIndex].pszUrl) && (NULL != ptConnection->pszUrl); ++iIndex)
		{
			char 	bIsMatch = 0;

			if (0 == os_strcmp(g_ptSupportedUrls[iIndex].pszUrl, ptConnection->pszUrl))
			{
				bIsMatch = 1;
			}
			if (((*ASTERISK_SYMBOL_STRING) == g_ptSupportedUrls[iIndex].pszUrl[os_strlen(g_ptSupportedUrls[iIndex].pszUrl) - 1]) &&
					( 0 == os_strncmp(g_ptSupportedUrls[iIndex].pszUrl, ptConnection->pszUrl, os_strlen(g_ptSupportedUrls[iIndex].pszUrl) - 1)))
			{
				bIsMatch = 1;
			}
			if (1 == bIsMatch)
			{

#ifdef HTTPD_DEBUG
				os_printf("httpdParseHeader: URL index is %d\n", iIndex);
#endif

				ptConnection->pbCgiData = NULL;
				ptConnection->pfnCgi = g_ptSupportedUrls[iIndex].pfnCgi;
				ptConnection->pbCgiArguments = g_ptSupportedUrls[iIndex].pbCgiArguments;
				return;
			}
		}
	}
	else if (0 == os_strncmp(pszHeader, "Content-Length: ", 16))
	{
		/* skip trailing spaces */
		for (iIndex = 0; (*SPACE_SYMBOL_STRING) != pszHeader[iIndex]; ++iIndex);

		/* get POST data length */
		ptConnection->ptPost->cbPostLength = atoi(pszHeader + iIndex + 1);

		/* allocate the buffer */
		if(HTTPD_MAX_POST_LENGTH < ptConnection->ptPost->cbPostLength)
		{
			/* stream in chunks */
			ptConnection->ptPost->cbMaxBufferSize = HTTPD_MAX_POST_LENGTH;
		}
		else
		{
			ptConnection->ptPost->cbMaxBufferSize = ptConnection->ptPost->cbPostLength;
		}

#ifdef HTTPD_DEBUG
		os_printf("httpdParseHeader: allocated buffer for %d + 1 bytes of post data\n", ptConnection->ptPost->cbMaxBufferSize);
#endif

		ptConnection->ptPost->pbBuffer=(char *)os_malloc(ptConnection->ptPost->cbMaxBufferSize + 1);
		ptConnection->ptPost->cbBufferLength = 0;
	}
	else if (0 == os_strncmp(pszHeader, "Content-Type: ", 14))
	{
		if (0 != os_strstr(pszHeader, "multipart/form-data"))
		{
			/* multipart form data so let's pull out the boundary for future use */
			char *	pbBoundaryOffset = NULL;

			pbBoundaryOffset = os_strstr(pszHeader, "boundary=");
			if (NULL != pbBoundaryOffset)
			{
				/* move pointer 2 chars before boundary then fill with dashes */
				ptConnection->ptPost->pbMultipartBoundary = pbBoundaryOffset + 7;
				ptConnection->ptPost->pbMultipartBoundary[0] = *DASH_SYMBOL_STRING;
				ptConnection->ptPost->pbMultipartBoundary[1] = *DASH_SYMBOL_STRING;

#ifdef HTTPD_DEBUG				
				os_printf("httpdParseHeader: boundary %s\n", ptConnection->ptPost->pbMultipartBoundary);
#endif
			}
		}
	}
}

/*  */
/************************************************
*	name:			httpdRecvCb
*	parameters:		ptArgument 		- connection data
*					pbData 	  		- data available
*					cbDataLength	- data length
*	return value:	none
*	purpose:		callback when data available on a socket
************************************************/
static void ICACHE_FLASH_ATTR	httpdRecvCb(void *			ptArgument,
											char *			pbData,
											unsigned short	cbDataLength)
{
	/* initialization */
	int 				iIndex 							= 0;
	char *				pbTmpHeader 					= NULL;
	char *				pbEndOfHeader 					= NULL;
	char 				pbSendBuffer[HTTPD_MAX_SEND_BUFFER_LENGTH]	= { 0 };
	HttpdConnection *	ptConnection 					= NULL;

	ptConnection = httpdFindConnData(ptArgument);

	if (NULL == ptConnection)
	{
		return;
	}

	ptConnection->ptPrivate->pbSendBuffer = pbSendBuffer;
	ptConnection->ptPrivate->cbSendBufferLength = 0;

	for (iIndex = 0; iIndex < cbDataLength; iIndex++)
	{
		if (0 > ptConnection->ptPost->cbPostLength)
		{
			/* header byte */
			if (HTTPD_MAX_HEADER_LENGTH != ptConnection->ptPrivate->cbHeaderLength)
			{
				ptConnection->ptPrivate->pbHeader[ptConnection->ptPrivate->cbHeaderLength++]=pbData[iIndex];
			}

			ptConnection->ptPrivate->pbHeader[ptConnection->ptPrivate->cbHeaderLength] = 0;

			/* scan for /r/n/r/n */
			if (((*NEWLINE_SYMBOL_STRING) == pbData[iIndex]) && (NULL != (char *)os_strstr(ptConnection->ptPrivate->pbHeader, "\r\n\r\n")))
			{
				/* indicate we're done with the headers */
				ptConnection->ptPost->cbPostLength = 0;

				/* reset url data */
				ptConnection->pszUrl = NULL;

				/* find end of next header line */
				pbTmpHeader = ptConnection->ptPrivate->pbHeader;

				while (pbTmpHeader < (&ptConnection->ptPrivate->pbHeader[ptConnection->ptPrivate->cbHeaderLength-4]))
				{
					pbEndOfHeader = (char *)os_strstr(pbTmpHeader, "\r\n");
					if (NULL == pbEndOfHeader)
					{
						break;
					}

					pbEndOfHeader[0] = 0;
					httpdParseHeader(pbTmpHeader, ptConnection);
					pbTmpHeader = pbEndOfHeader + 2;
				}

				/* if receive post data unncessary send the response */
				if (0 == ptConnection->ptPost->cbPostLength)
				{
					httpdProcessRequest(ptConnection);
				}
			}
		}
		else if (0 != ptConnection->ptPost->cbPostLength)
		{
			/* this is a POST byte */
			ptConnection->ptPost->pbBuffer[ptConnection->ptPost->cbBufferLength++] = pbData[iIndex];
			ptConnection->ptPost->cbReceived++;
			if ((ptConnection->ptPost->cbBufferLength >= ptConnection->ptPost->cbMaxBufferSize) || (ptConnection->ptPost->cbReceived == ptConnection->ptPost->cbPostLength))
			{
				/* received a chunk of post data */
				/* zero-terminate, in case the cgi handler knows it can use strings */
				ptConnection->ptPost->pbBuffer[ptConnection->ptPost->cbBufferLength] = 0;
				/* send the response */
				httpdProcessRequest(ptConnection);
				ptConnection->ptPost->cbBufferLength = 0;
			}
		}
	}
}

/************************************************
*	name:			httpdReconCb
*	parameters:		ptArgument 	- connection data
*					iError		- error code
*	return value:	none
*	purpose:		callback on reconnect
************************************************/
static void ICACHE_FLASH_ATTR	httpdReconCb(void *	ptArgument,
											 sint8	iError)
{
	/* initialization */
	HttpdConnection *	ptConnection = NULL;

	ptConnection = httpdFindConnData(ptArgument);

#ifdef HTTPD_DEBUG
	os_printf("httpdReconCb\n");
#endif

	UNUSED(ptConnection);
}

/************************************************
*	name:			httpdDisconCb
*	parameters:		ptArgument - argument
*	return value:	none
*	purpose:		callback for disconnect to check all sockets and kill if needed
************************************************/
static void ICACHE_FLASH_ATTR	httpdDisconCb(void *	ptArgument)
{
	/*	initialization	*/
	int 	iIndex = 0;

	for (iIndex = 0; HTTPD_MAX_CONNECTIONS > iIndex; iIndex++)
	{
		if (NULL != g_ptConnectionData[iIndex].ptEspConnection)
		{
			/* Why the >=ESPCONN_CLOSE and not ==? Well, seems the stack sometimes de-allocates
			espconns under our noses, especially when connections are interrupted. The memory
			is then used for something else, and we can use that to capture *most* of the
			disconnect cases. */
			if ((ESPCONN_NONE == g_ptConnectionData[iIndex].ptEspConnection->state) || (ESPCONN_CLOSE <= g_ptConnectionData[iIndex].ptEspConnection->state))
			{
				g_ptConnectionData[iIndex].ptEspConnection = NULL;
				if (NULL != g_ptConnectionData[iIndex].pfnCgi)
				{
					/* flush cgi data */
					g_ptConnectionData[iIndex].pfnCgi(&g_ptConnectionData[iIndex]);
				}
				httpdRetireConn(&g_ptConnectionData[iIndex]);
			}
		}
	}
}

/************************************************
*	name:			httpdConnectCb
*	parameters:		ptArgument - esp connection
*	return value:	none
*	purpose:		assign connection to request
************************************************/
static void ICACHE_FLASH_ATTR	httpdConnectCb(void *	ptArgument)
{
	/* initialization */
	struct espconn *	ptEspConnection = ptArgument;
	int 				iIndex 	 			= 0;

	/* find empty g_ptConnectionData in pool */
	for (iIndex = 0; HTTPD_MAX_CONNECTIONS > iIndex; iIndex++)
	{
		if (NULL == g_ptConnectionData[iIndex].ptEspConnection)
		{
			break;
		}
		
#ifdef HTTPD_DEBUG
		os_printf("httpdConnectCb: connection request: connection %p, pool slot %d\n", ptEspConnection, iIndex);
#endif

		if (HTTPD_MAX_CONNECTIONS == iIndex)
		{

#ifdef HTTPD_DEBUG
			os_printf("httpdConnectCb: connection pool overflow!\n");
#endif

			espconn_disconnect(ptEspConnection);
			goto lblCleanup;
		}
	}

	g_ptConnectionData[iIndex].ptPrivate = &g_ptConnectionPrivateData[iIndex];
	g_ptConnectionData[iIndex].ptEspConnection = ptEspConnection;
	g_ptConnectionData[iIndex].ptPrivate->cbHeaderLength = 0;
	g_ptConnectionData[iIndex].ptPost = &g_ptConnectionPost[iIndex];
	g_ptConnectionData[iIndex].ptPost->pbBuffer = NULL;
	g_ptConnectionData[iIndex].ptPost->cbBufferLength = 0;
	g_ptConnectionData[iIndex].ptPost->cbReceived = 0;
	g_ptConnectionData[iIndex].ptPost->cbProcessed = 0;
	g_ptConnectionData[iIndex].ptPost->cbPostLength = -1;

	espconn_regist_recvcb(ptEspConnection, httpdRecvCb);
	espconn_regist_reconcb(ptEspConnection, httpdReconCb);
	espconn_regist_disconcb(ptEspConnection, httpdDisconCb);
	espconn_regist_sentcb(ptEspConnection, httpdSentCb);

lblCleanup:
	return;
}

/*	httpdInit	*/
void ICACHE_FLASH_ATTR	httpdInit(HttpdUrlDescriptor *	ptHttpdFixedUrls,
								  int 					iPort)
{
	/* initialization */
	int 	iIndex = 0;

	for (iIndex = 0; HTTPD_MAX_CONNECTIONS > iIndex; iIndex++)
	{
		g_ptConnectionData[iIndex].ptEspConnection = NULL;
	}

	g_tConnection.type = ESPCONN_TCP;
	g_tConnection.state = ESPCONN_NONE;
	g_tTcp.local_port = iPort;
	g_tConnection.proto.tcp = &g_tTcp;
	g_ptSupportedUrls = ptHttpdFixedUrls;

#ifdef HTTPD_DEBUG
	os_printf("httpdInit: connection %p\n", &g_tConnection);
#endif
	
	espconn_regist_connectcb(&g_tConnection, httpdConnectCb);
	espconn_accept(&g_tConnection);
}
