/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		cgiwifi.h
*	purpose:	wifi CGI routines header
************************************************/

#ifndef __CGIWIFI_H__
#define __CGIWIFI_H__

#include "shared.h"

#include "httpd.h"
#include "string.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "cgi.h"
#include "io.h"

#define	WIFI_MODE_STRING_LENGTH		1
#define	WIFI_MAX_SSID_LENGTH		32
#define	WIFI_MAX_PASSWORD_LENGTH	64
#define	WIFI_MODE_STRING			"mode"

/* WiFi access point data */
typedef struct _ApData
{
	char ssid[32];
	char rssi;
	char enc;
} ApData;

/* scan result */
typedef struct _ScanResultData
{
	char 		scanInProgress;
	ApData **	apData;
	int 		noAps;
} ScanResultData;

typedef enum
{
	/* according to SDK's wifi_get_opmode() */
	WIFI_STATION_MODE = 0,
	WIFI_SOFTAP_MODE,
	WIFI_STATION_AND_SOFTAP_MODE
} WIFI_OPMODES;

/************************************************
*	name:			cgiWiFiScan
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		called from the AJAX
*					initiates scan for access points and if available will return the result of an earlier scan
*					result is embedded in JSON parsed by the JS
************************************************/
int cgiWiFiScan(HttpdConnection *	ptConnection);

/************************************************
*	name:			cgiWiFiConnect
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		connect to a specific access point given ESSID and password
*	remarks:		STA+AP mode to AP mode then again STA+AP will auto connect to first hotspot (unless specified otherwise)
************************************************/
int cgiWiFiConnect(HttpdConnection *	ptConnection);

/************************************************
*	name:			cgiWifiSetMode
*	parameters:		ptConnection - HTTP connection
*	return value:	CGI status
*	purpose:		set wifi mode
************************************************/
int cgiWifiSetMode(HttpdConnection *	ptConnection);

#endif //__CGIWIFI_H__
