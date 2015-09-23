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
*	purpose:	general CGI routines implementation
************************************************/

#include "cgi.h"

static int 		g_bLedState 		= 0;

/* cgiLed */
int ICACHE_FLASH_ATTR cgiLed(HttpdConnection *	ptConnection)
{
	/* initialization */
	int 	iRet				= 0;
	int 	length 				= 0;
	char 	pszTokenValue[1024] = { 0 };
	
	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	length = httpdFindArg(ptConnection->ptPost->pbBuffer, CGI_TOKEN_LED_NAME_STRING, pszTokenValue, sizeof(pszTokenValue));
	if (0 < length)
	{
		/* acquire token value */
		int 	bValue = 0;

		bValue = atoi(pszTokenValue);

		if (CGI_TOKEN_LED_SWITCH_STATUS_STRING == bValue)
		{

#ifdef CGI_DEBUG
			os_printf("cgiLed: retrieved switch request\n");
#endif

			g_bLedState = ~g_bLedState;
			ioLed(g_bLedState);
		}
	}

#ifdef CGI_DEBUG
	os_printf("cgiLed: completed request\n");
#endif
	
	httpdRedirect(ptConnection, "/index.tpl");

	iRet = HTTPD_CGI_DONE;

lblCleanup:
	return iRet;
}

/* tplMain	*/
int ICACHE_FLASH_ATTR tplMain(HttpdConnection *	ptConnection, 
							  char *			pszToken, 
							  void **			ppArgument)
{
	/* initialization */
	int 	iWifiMode 		= 0;
	int 	iRet			= 0;
	char	pszBuffer[128]	= { 0 };

	if (NULL == pszToken)
	{
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}
	
	static struct station_config stconf = { 0 };

	wifi_station_get_config(&stconf);

	if (0 == os_strcmp(pszToken, CGI_TOKEN_WIFIMODE_STRING)) 
	{
		iWifiMode = wifi_get_opmode();
		if (1 == iWifiMode)
		{
			os_strcpy(pszBuffer, "Client");
		}
		if (2 == iWifiMode)
		{
			os_strcpy(pszBuffer, "SoftAP");
		}
		if (3 == iWifiMode)
		{
			os_strcpy(pszBuffer, "STA+AP");
		}
	}
	else if (0 == os_strcmp(pszToken, CGI_TOKEN_CURRSSID_STRING))
	{
		os_strcpy(pszBuffer, (char *)stconf.ssid);
	}
	else if (0 == os_strcmp(pszToken, CGI_TOKEN_WIFIPASS_STRING))
	{
		os_strcpy(pszBuffer, (char *)stconf.password);
	}
	else if (0 == os_strcmp(pszToken, CGI_TOKEN_WIFIWARN_STRING))
	{
		iWifiMode=wifi_get_opmode();
		if (2 == iWifiMode)
		{
			os_strcpy(pszBuffer, "<b>Can't scan in this mode.</b> Click <a href=\"setmode.cgi?mode=3\">here</a> to go to STA+AP mode.");
		}
		else 
		{
			os_strcpy(pszBuffer, "Click <a href=\"setmode.cgi?mode=2\">here</a> to go to standalone AP mode.");
		}
	}
	else if (0 == os_strcmp(pszToken, CGI_TOKEN_CHECKBOX_STATUS_STRING))
	{
		if (0 == g_bLedState)
		{
			os_strcpy(pszBuffer, (char *)CGI_TOKEN_CHECKBOX_CHECKED_STRING);
		}
	}
	else if (0 == os_strcmp(pszToken, CGI_TOKEN_BOOTMODE_STRING))
	{
		os_sprintf(pszBuffer, "%d", system_upgrade_userbin_check());
	}

	httpdSend(ptConnection, pszBuffer, -1);
	iRet = HTTPD_CGI_DONE;

lblCleanup:
	return iRet;
}
