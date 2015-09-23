/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		user_main.h
*	purpose:	main logic implementation
************************************************/

#include "user_main.h"

#ifdef	ENABLE_HTTP_AHTENTICATION
/************************************************
*	name:			myPassFn
*	parameters:		ptConnection 	- HTTP connection
*					no 					-
*					pszUsername			- username string
*					cbUsernameLength	- pszUsername length
*					pszPassword			- password string
*					cbPasswordLength	- pszPassword length
*	return value:	authenticated or not
*	purpose:		authentication
************************************************/
int myPassFn(HttpdConnData *	ptConnection, 
			 int 				iCrednetialsCounter, 
			 char *				pszUsername, 
			 int 				cbUsernameLength, 
			 char *				pszPassword, 
			 int 				cbPasswordLength)
{
	/* initialization */
	int 	iRet = 0;

	if (0 == iCrednetialsCounter)
	{
		os_strcpy(pszUsername, WEB_USERNAME_STRING);
		os_strcpy(pszPassword, WEB_PASSWORD_STRING);
		iRet = 1;
		goto lblCleanup;
	}

	/* advance crednetials verification can be added */

	return iRet;
}
#endif


/*	list of exported urls, handled top-down
*	specific rules should be place above the more general ones
*	asterisk matches any url starting with everything before it
*	authorization routines act as a 'barrier' and should be placed above the URLs they protect */
HttpdUrlDescriptor g_ptSupportedUrls[] = {
	{"/", 				cgiRedirect, 		"/index.tpl"},
	{"/flash.bin",		cgiReadFlash, 		NULL},
	{"/index.tpl",		cgiEspFsTemplate,	tplMain},
	{"/led.cgi", 		cgiLed, 			NULL},
	{"/flashraw.cgi",	cgiUploadRaw,		NULL},
	{"/flashapp.cgi",	cgiUpgradeRaw,		NULL},
	{"/getappver.cgi",	cgiGetAppVer,		NULL},

#ifdef ENABLE_HTTP_AHTENTICATION
	/* protect WiFi configuration with credentials */
	{"/wifi/*",			authBasic,			myPassFn},
#endif

	{"/index", 			cgiRedirect, 		"/index.tpl"},
	{"/wifiscan.cgi", 	cgiWiFiScan, 		NULL},
	{"/connect.cgi", 	cgiWiFiConnect,		NULL},
	{"/setmode.cgi", 	cgiWifiSetMode,	 	NULL},

	/* catch all cgi function for the filesystem */
	{"*", 				cgiEspFsHook, 		NULL},
	{NULL, 				NULL, 				NULL}
};



/************************************************
*	name:			user_init
*	parameters:		none
*	return value:	none
*	purpose:		main logic
************************************************/
void user_init(void)
{	
	/* initialization */
	struct softap_config 	tWifiSettings = { 0 };
	uint8 					puMacAddress[6] = { 0 };

	os_memset((void *)&tWifiSettings, 0, sizeof(tWifiSettings));

	wifi_softap_get_config(&tWifiSettings);
	
	/* retrieve mac address */
	wifi_get_macaddr(1, puMacAddress);
	
	/* append to ssid name */
	memcpy(tWifiSettings.ssid, HOTSPOT_SSID_STRING, sizeof(HOTSPOT_SSID_STRING));

	os_sprintf(tWifiSettings.ssid + sizeof(HOTSPOT_SSID_STRING) - 1, "%x%x%x%x%x%x", puMacAddress[0],
							  												  	 	 puMacAddress[1],
							  												  	 	 puMacAddress[2],
							  												  	 	 puMacAddress[3],
							  												  	 	 puMacAddress[4],
							  												  	 	 puMacAddress[5]);
	/* set password */
	memcpy(tWifiSettings.password, HOTSPOT_PASSWORD_STRING, os_strlen(HOTSPOT_PASSWORD_STRING));
	
	/* update other settings */
	tWifiSettings.authmode = AUTH_WPA_WPA2_PSK;
	tWifiSettings.ssid_len = 0;
	tWifiSettings.channel = 6;
	tWifiSettings.max_connection = 4;
	
	/* update wifi configuration */
	wifi_softap_set_config(&tWifiSettings);
	
	/* initialize stdout */
	stdoutInit();
	
	/* initialize IOs */
	ioInit();
	
	/* initialize server */
	httpdInit(g_ptSupportedUrls, 80);
	
#ifdef USER_MAIN_DEBUG
	os_printf("user_init: ready; partition %d\n", system_upgrade_userbin_check());
#endif
}
