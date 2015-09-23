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
*	purpose:	flash CGI routines header
************************************************/

#ifndef __CGIFLASH_H__
#define __CGIFLASH_H__

#include "shared.h"
#include "string.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "io.h"
#include "ip_addr.h"
#include "upgrade.h"
#include "flash.h"

#define	MAX_BIN_BUFFER 1024

#define LOOKING_FOR_SECTION			1
#define CHECKING_HEADERS 			2
#define IN_DATA 					3
#define	FINISHED_UPLOADING_STRING	"Finished uploading"

/************************************************
*	name:			cgiReadFlash
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		read the SPI flash (512KB)
************************************************/
int cgiReadFlash(HttpdConnection *	ptConnection);

/************************************************
*	name:			cgiUploadRaw
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		replace ESPFS image via http POST
************************************************/
int cgiUploadRaw(HttpdConnection *	ptConnection);

/************************************************
*	name:			cgiUpgradeRaw
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		wrap cgiUploadRaw and schedule a reboot once completed
************************************************/
int cgiUpgradeRaw(HttpdConnection *	ptConnection);

/************************************************
*	name:			cgiGetAppVer
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		retrieve app version
************************************************/
int cgiGetAppVer(HttpdConnection *	ptConnection);

#endif //__CGIFLASH_H__
