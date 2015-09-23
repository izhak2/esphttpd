/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		auth.h
*	purpose:	server authorization functions header
************************************************/

#ifndef __AUTH_H__
#define __AUTH_H__

#include "shared.h"
#include "string.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include "base64.h"
#include "ip_addr.h"

#define	HTTPD_CONTENT_TYPE_STRING			"Content-Type"
#define	HTTPD_TEXT_PLAIN_STRING				"text/plain"
#define	HTTPD_AUTH_WWW_AUTHENTICATE_STRING	"WWW-Authenticate"
#define	HTTPD_AUTH_BASIC_STRING				"Basic"	
#define	HTTPD_AUTH_AUTHORIZATION_STRING		"Authorization"
#define	HTTPD_401_ERROR_STRING				"401 Forbidden."

#define HTTPD_AUTH_SINGLE					0
#define HTTPD_AUTH_CALLBACK					1
	
#define AUTH_MAX_USER_LEN					32
#define AUTH_MAX_PASS_LEN					32

/* typedef for authXXX functions - callback which returns credentials the device has */
typedef int (* pfnAuthGetCredentials)(HttpdConnection *	ptConnection, 
							  		  int 				iCrednetialsCounter, 
							  		  char *			pszUsername, 
							  		  int 				cbUsernameLength, 
							  		  char *			pszPassword, 
							  		  int 				cbPasswordLength);

/************************************************
*	name:			authBasic
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		implement server authentication routine
************************************************/
int ICACHE_FLASH_ATTR authBasic(HttpdConnection *	ptConnection);

#endif //__AUTH_H__
