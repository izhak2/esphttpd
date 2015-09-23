/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		auth.c
*	purpose:	HTTP auth implementation
************************************************/

#include "auth.h"

/*	authBasic 	*/
int ICACHE_FLASH_ATTR authBasic(HttpdConnection *	ptConnection)
{
	/* initialization */
	int 	iRet 																	= 0;
	int 	iCredentialsCounter 													= 0;
	int 	iSuccess																= 0;
	char 	pszAuthorizationHeader[(AUTH_MAX_USER_LEN+AUTH_MAX_PASS_LEN + 2) * 10]	= { 0 };
	char 	pszCredentials[AUTH_MAX_USER_LEN+AUTH_MAX_PASS_LEN + 2]					= { 0 };
	char 	pszUser[AUTH_MAX_USER_LEN]												= { 0 };
	char 	pszPassword[AUTH_MAX_PASS_LEN]											= { 0 };

	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	/* retrieve authrization header */
	iSuccess = httpdGetHeader(ptConnection, HTTPD_AUTH_AUTHORIZATION_STRING, pszAuthorizationHeader, sizeof(pszAuthorizationHeader));
	if (iSuccess && (0 == strncmp(pszAuthorizationHeader, HTTPD_AUTH_BASIC_STRING, sizeof(HTTPD_AUTH_BASIC_STRING))))
	{
		/* decode header */
		iSuccess = base64_decode(strlen(pszAuthorizationHeader) - 6, pszAuthorizationHeader + 6, sizeof(pszCredentials), (unsigned char *)pszCredentials);
		
		if (0 > iSuccess)
		{
			/* clean string on decode error */
			iSuccess = 0;
		}

		/* zero-terminate credentials string */
		pszCredentials[iSuccess] = 0;

		while (((pfnAuthGetCredentials)(ptConnection->pbCgiArguments))(ptConnection, iCredentialsCounter, pszUser, AUTH_MAX_USER_LEN, pszPassword, AUTH_MAX_PASS_LEN))
		{
			/* verify credentials against auth header */
			if ((strlen(pszCredentials) == strlen(pszUser) + strlen(pszPassword) + 1) 	&& 
				(0 == os_strncmp(pszCredentials, pszUser, strlen(pszUser)))				&&
				((*COLON_SYMBOL_STRING) == pszCredentials[strlen(pszUser)])				&&
				(0 == os_strcmp(pszCredentials + strlen(pszUser) + 1, pszPassword)))
			{
				/* authenticated */
				iRet = HTTPD_CGI_AUTHENTICATED;
				goto lblCleanup;
			}

			/* try next supplied credentials */
			iCredentialsCounter++;
		}
	}

	/* authentication failed - show login screen */
	httpdStartResponse(ptConnection, 401);
	httpdHeader(ptConnection, HTTPD_CONTENT_TYPE_STRING, HTTPD_TEXT_PLAIN_STRING);
	httpdHeader(ptConnection, HTTPD_AUTH_WWW_AUTHENTICATE_STRING, "Basic realm=\""HTTP_AUTH_REALM"\"");
	httpdEndHeaders(ptConnection);
	httpdSend(ptConnection, HTTPD_401_ERROR_STRING, -1);
	
	/* completed */
	iRet = HTTPD_CGI_DONE;

lblCleanup:
	return iRet;
}
