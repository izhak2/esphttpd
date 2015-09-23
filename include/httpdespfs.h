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
*	purpose:	server espfs related functions header
************************************************/

#ifndef	__HTTPDESPFS_H__
#define	__HTTPDESPFS_H__

#include "shared.h"

#include "httpd.h"
#include "espfs.h"
#include "string.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

typedef struct _TplData
{
	EspFsFile *	file;
	void *		tplArg;
	char 		token[64];
	int 		tokenPos;
} TplData;


/* callback function prototype */
typedef void (* TplCallback)(HttpdConnection *	ptConnection, 
							 char *				pszToken, 
							 void **			ppArgument);

/************************************************
*	name:			cgiEspFsHook
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		generic catch cgi functions for FS
************************************************/
int 					cgiEspFsHook(HttpdConnection *	ptConnection);

/************************************************
*	name:			cgiEspFsTemplate
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		catch tpl cgi functions
************************************************/
int ICACHE_FLASH_ATTR	cgiEspFsTemplate(HttpdConnection *	ptConnection);

#endif //__HTTPDESPFS_H__
