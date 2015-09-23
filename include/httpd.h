/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		httpd.h
*	purpose:	HTTP server header
************************************************/

#ifndef	__HTTPD_H__
#define	__HTTPD_H__

#include "shared.h"

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "mem.h"
#include "osapi.h"

#include "io.h"
#include "espfs.h"

#define HTTPD_VERSION_STRING			"0.3"
#define HTTPD_MAX_HEADER_LENGTH			1024	//Max length of request head
#define HTTPD_MAX_POST_LENGTH			1024	//Max post buffer len
#define HTTPD_MAX_SEND_BUFFER_LENGTH 	2048	//Max send buffer len

/* httpd messages */
#define HTTPD_HEADER_404_ERROR_STRING	"%s not found. 404!\n"
#define HTTPD_HEADER_NOT_FOUND_STRING	"HTTP/1.0 404 Not Found\r\nServer: esp8266-httpd/0.1\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nNot Found.\r\n"

/* CGI status enum */
typedef enum 
{
	HTTPD_CGI_FAILED	= -1,
	HTTPD_CGI_MORE 		= 0,
	HTTPD_CGI_DONE,
	HTTPD_CGI_NOTFOUND,
	HTTPD_CGI_AUTHENTICATED
} HTTPD_CGI_STATUS;

/* HTTPD methods */
typedef enum
{
	HTTPD_METHOD_GET = 1,
	HTTPD_METHOD_POST
} HTTPD_METHOD;

typedef struct _HttpdPrivate 	HttpdPrivate;
typedef struct _HttpdConnection HttpdConnection;

typedef int (* pfnCgiCallback)(HttpdConnection *	ptConnection);

/* Holds extension->mime data */
typedef struct _HttpdMimeMap
{
	const char *	pszExtension;
	const char *	pszMimeType;
} HttpdMimeMap;

/* POST data sent inside the http connection, used by the CGI functions */
typedef struct _HttpdPost
{
	int 	cbPostLength;			// POST Content-Length
	int 	cbMaxBufferSize;		// maximum length of post buffer
	int 	cbBufferLength; 		// amount of bytes in current post buffer
	int 	cbReceived; 			// total amount of bytes received
	int 	cbProcessed;			// total amount of bytes processed
	char *	pbBuffer; 				// POST data buffer
	char *	pbMultipartBoundary;
} HttpdPost;

typedef struct _HttpdConnection
{
	struct espconn	*ptEspConnection;
	char			bRequestType;
	char *			pszUrl;
	char *			pbGetArguments;
	const void *	pbCgiArguments;
	void *			pbCgiData;
	void *			pbCgiPreviousData;	// used for streaming handlers storing state between requests
	HttpdPrivate *	ptPrivate;
	pfnCgiCallback	pfnCgi;
	HttpdPost *		ptPost;
} HttpdConnection;

/* URL description,  used to send different URL requests to different routines */
typedef struct _HttpdUrlDescriptor
{
	const char *	pszUrl;
	pfnCgiCallback	pfnCgi;
	const void *	pbCgiArguments;
} HttpdUrlDescriptor;


/************************************************
*	name:			cgiRedirect
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		wrap httpdRedirect
************************************************/
int ICACHE_FLASH_ATTR	cgiRedirect(HttpdConnection *	ptConnection);

/************************************************
*	name:			httpdRedirect
*	parameters:		ptConnection - HTTP connection
*					pszNewURL		- redirect url
*	return value:	CGI status
*	purpose:		http redirect
************************************************/
void ICACHE_FLASH_ATTR	httpdRedirect(HttpdConnection *	ptConnection, 
									  char *			pszNewURL);

/************************************************
*	name:			httpdUrlDecode
*	parameters:		pbEncodedBuffer 		- input buffer
*					encodedBufferLength		- pbEncodedBuffer length
*					pbDecodedBuffer 		- output buffer
*					decodedBufferLength		- pbDecodedBuffer length
*	return value:	amount of bytes used in ret
*	purpose:		decode a percent-encoded value.
************************************************/
int 					httpdUrlDecode(char *	pbEncodedBuffer,
									   int 		cbEncodedBufferLength,
									   char *	pbDecodedBuffer,
									   int 		cbDecodedBufferLength);

/************************************************
*	name:			httpdFindArg
*	parameters:		pszLine 		- string of post/get-data
*					pszArgument		- name of value to look for
*					pszBuffer		- output buffer
*					cbBufferLength 	- pszBuffer length
*	return value:	length of result, -1 if not found
*	purpose:		find argument in post/get data
************************************************/
int ICACHE_FLASH_ATTR	httpdFindArg(char *	pszLine,
									 char *	pszArgument,
									 char *	pszBuffer,
									 int 	cbBufferLength);

/************************************************
*	name:			httpdInit
*	parameters:		ptHttpdFixedUrls	- array of urls
*					iPort				- server port
*	return value:	none
*	purpose:		initialize server
************************************************/
void ICACHE_FLASH_ATTR	httpdInit(HttpdUrlDescriptor *	ptHttpdFixedUrls,
								  int 					iPort);

/************************************************
*	name:			httpdGetMimetype
*	parameters:		pszUrl - url to get mime type for
*	return value:	mime type string
*	purpose:		retrieve mime type
************************************************/
const char *			httpdGetMimetype(char *	pszUrl);

/************************************************
*	name:			httpdStartResponse
*	parameters:		ptConnection - HTTP connection
*					iCode			- response code
*	return value:	none
*	purpose:		send response headers
************************************************/
void ICACHE_FLASH_ATTR 	httpdStartResponse(HttpdConnection *	ptConnection,
										   int 					iCode);

/************************************************
*	name:			httpdHeader
*	parameters:		ptConnection 	- HTTP connection
*					pszField		- http header field
*					pszValue		- http header value
*	return value:	none
*	purpose:		send http header
************************************************/
void ICACHE_FLASH_ATTR 	httpdHeader(HttpdConnection *	ptConnection,
									const char *		pszField,
									const char *		pszValue);

/************************************************
*	name:			httpdEndHeaders
*	parameters:		ptConnection - HTTP connection
*	return value:	none
*	purpose:		send end of headers
************************************************/
void ICACHE_FLASH_ATTR 	httpdEndHeaders(HttpdConnection *	ptConnection);

/************************************************
*	name:			httpdGetHeader
*	parameters:		ptConnection 			- HTTP connection
*					pszHeader				- client header
*					pszOutputBuffer			- output buffer
*					cbOutputBufferLength	- pszOutputBuffer length
*	return value:	1 on success, 0 on failure
*	purpose:		retrieve certain client header value
************************************************/
int ICACHE_FLASH_ATTR 	httpdGetHeader(HttpdConnection *	ptConnection,
									   char *				pszHeader,
									   char *				pszOutputBuffer,
									   int 					cbOutputBufferLength);

/************************************************
*	name:			httpdSend
*	parameters:		ptConnection - HTTP connection
*					pbData			- data to send
*					cbDataLength	- pszDataLength
*	return value:	1 on success, 0 for buffer too large
*	purpose:		send data to client
************************************************/
int ICACHE_FLASH_ATTR 	httpdSend(HttpdConnection *	ptConnection,
								  const char *		pbData,
								  int 				cbDataLength);

#endif	//__HTTPD_H__
