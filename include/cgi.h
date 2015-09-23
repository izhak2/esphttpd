/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		cgi.h
*	purpose:	general CGI routines header
************************************************/

#ifndef __CGI_H__
#define __CGI_H__

#include "shared.h"

#include "httpd.h"
#include "string.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "io.h"
#include "ip_addr.h"

#define	CGI_TOKEN_WIFIMODE_STRING			"WiFiMode"
#define CGI_TOKEN_CURRSSID_STRING			"currSsid"
#define CGI_TOKEN_WIFIPASS_STRING			"WiFiPasswd"
#define CGI_TOKEN_WIFIWARN_STRING			"WiFiapwarn"
#define CGI_TOKEN_BOOTMODE_STRING			"boot"
#define	CGI_TOKEN_CHECKBOX_STATUS_STRING	"ledStatus"
#define	CGI_TOKEN_CHECKBOX_CHECKED_STRING	"checked"

#define	CGI_TOKEN_LED_NAME_STRING			"switch"
#define	CGI_TOKEN_LED_SWITCH_STATUS_STRING	1

/************************************************
*	name:			cgiLed
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		led cgi handler
************************************************/
int cgiLed(HttpdConnection *	ptConnection);

/************************************************
*	name:			tplMain
*	parameters:		ptConnection - HTTP connection
*					pszToken		- token string
*					ppArgument		- argument
*	return value:	CGI status
*	purpose:		tpl handler
************************************************/
int tplMain(HttpdConnection *	ptConnection, 
			char *				pszToken, 
			void **				ppArgument);

#endif //__CGI_H__
