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
*	purpose:	wifi CGI routines implementation
************************************************/

#include "cgiwifi.h"

/* new access point configuration */
static struct station_config tTmpStationConfiguration	= { 0 };

/* static scan storage */
static ScanResultData cgiWifiAps						= { 0 };

/************************************************
*	name:			wifiScanDoneCb
*	parameters:		ptBssInfo - bss info list head
*					
*	return value:	none
*	purpose:		once scan is completed stores the result in cgiWifiAps struct
************************************************/
void ICACHE_FLASH_ATTR wifiScanDoneCb(void *	ptBssInfo, 
									  STATUS 	eStatus)
{
	/* initialization */
	int 			iIndex 			= 0;
	struct bss_info *ptBssInfoNode 	= (struct bss_info *)ptBssInfo;
	
#ifdef CGIWIFI_DEBUG
	os_printf("wifiScanDoneCb: eStatus code %d\n", eStatus);
#endif
	
	if (OK != eStatus)
	{
		/* if scan failed - abort */
		cgiWifiAps.scanInProgress = 0;
		goto lblCleanup;
	}

	/* clear previous ap data if needed */
	if (NULL != cgiWifiAps.apData)
	{
		for (iIndex = 0; iIndex < cgiWifiAps.noAps; ++iIndex)
		{
			if (NULL != cgiWifiAps.apData[iIndex])
			{
				os_free(cgiWifiAps.apData[iIndex]);
				cgiWifiAps.apData[iIndex] = NULL;
			}
		}

		os_free(cgiWifiAps.apData);
		cgiWifiAps.apData = NULL;
	}

	/* count amount of access points found */
	for (iIndex = 0; NULL != ptBssInfoNode; ptBssInfoNode = ptBssInfoNode->next.stqe_next, ++iIndex);

	/* allocate memory for access point data */
	cgiWifiAps.apData = (ApData **)os_malloc(sizeof(ApData *) * iIndex);
	cgiWifiAps.noAps = iIndex;

#ifdef CGIWIFI_DEBUG
	os_printf("wifiScanDoneCb: scan done and found %d access points\n", iIndex);
#endif

	/* traverse through access points found and copy data to static struct */
	for (iIndex = 0, ptBssInfoNode = (struct bss_info *)ptBssInfo; NULL != ptBssInfoNode; ptBssInfoNode = ptBssInfoNode->next.stqe_next, ++iIndex)
	{
		if (iIndex >= cgiWifiAps.noAps)
		{
			/* verify ptBssInfoNode wasn't changed to prevent write in unallocated memory */

#ifdef CGIWIFI_DEBUG
			os_printf("wifiScanDoneCb: error - more access points than the %d allocated count\n", cgiWifiAps.noAps);
#endif
			break;
		}

		/* store ap data */
		cgiWifiAps.apData[iIndex] = (ApData *)os_malloc(sizeof(ApData));
	
		cgiWifiAps.apData[iIndex]->rssi = ptBssInfoNode->rssi;
		cgiWifiAps.apData[iIndex]->enc = ptBssInfoNode->authmode;

		strncpy(cgiWifiAps.apData[iIndex]->ssid, (char *)ptBssInfoNode->ssid, WIFI_MAX_SSID_LENGTH);

	}

	/* update scan completed */
	cgiWifiAps.scanInProgress = 0;

lblCleanup:
	return;
}

/************************************************
*	name:			wifiStartScan
*	parameters:		none
*	return value:	none
*	purpose:		initiate WiFi access point scan if one isn't present
************************************************/
static void ICACHE_FLASH_ATTR wifiStartScan(void)
{
	/* verify if scan is in progress */
	if (true == cgiWifiAps.scanInProgress)
	{
		return;
	}
	
	/* otherwise initiate scan */
	cgiWifiAps.scanInProgress = 1;
	wifi_station_scan(NULL, wifiScanDoneCb);
}

/* cgiWifiScan */
int ICACHE_FLASH_ATTR cgiWiFiScan(HttpdConnection *	ptConnection)
{
	/* initialization */
	int 	iRet 				= 0;
	int 	iIndex				= 0;
	int 	cbContentLength		= 0;
	char 	pszContent[1024]	= { 0 };

	httpdStartResponse(ptConnection, 200);
	httpdHeader(ptConnection, "Content-Type", "text/json");
	httpdEndHeaders(ptConnection);

	if (1 == cgiWifiAps.scanInProgress)
	{
		/* scan in progress - notify JS */
		cbContentLength = os_sprintf(pszContent, "{\n \"result\": { \n\"inProgress\": \"1\"\n }\n}\n");
		httpdSend(ptConnection, pszContent, cbContentLength);
	}
	else
	{
		/* scan finished - update content */
		cbContentLength = os_sprintf(pszContent, "{\n \"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
		httpdSend(ptConnection, pszContent, cbContentLength);
		
		if (NULL == cgiWifiAps.apData)
		{
			cgiWifiAps.noAps = 0;
		}

		for (iIndex = 0; iIndex < cgiWifiAps.noAps; ++iIndex)
		{
			/* update json with access point data */
			cbContentLength = os_sprintf(pszContent, "{\"essid\": \"%s\", \"rssi\": \"%d\", \"enc\": \"%d\"}%s\n", 
										 cgiWifiAps.apData[iIndex]->ssid, cgiWifiAps.apData[iIndex]->rssi,
										 cgiWifiAps.apData[iIndex]->enc, (iIndex == cgiWifiAps.noAps - 1) ? "" : ",");
			httpdSend(ptConnection, pszContent, cbContentLength);
		}

		cbContentLength=os_sprintf(pszContent, "]\n}\n}\n");
		httpdSend(ptConnection, pszContent, cbContentLength);
		
		/* begin new scan */
		wifiStartScan();
	}

	iRet = HTTPD_CGI_DONE;
	return iRet;
}

/************************************************
*	name:			resetTimerCb
*	parameters:		ptArgument - callback argument
*	return value:	none
*	purpose:		execute some time after a connection attempt to an access point.
*					if connection succeeds, switches to soft-ap mode
************************************************/
static void ICACHE_FLASH_ATTR resetTimerCb(void *	ptArgument)
{
	/* initialization */
	int iStationConnectStatus	= wifi_station_get_connect_status();
	
	if (STATION_GOT_IP == iStationConnectStatus)
	{
		/* switch to soft-ap mode */

#ifdef CGIWIFI_DEBUG
		os_printf("resetTimerCb: retrieved ip; switching to soft-ap mode\n");
#endif

		wifi_set_opmode(WIFI_SOFTAP_MODE);
		system_restart();
	}
	else
	{

#ifdef CGIWIFI_DEBUG
		os_printf("resetTimerCb: connect failed; not switching to soft-ap mode\n");
#endif
	}
}

/* cgiWifiConnect */
int ICACHE_FLASH_ATTR cgiWiFiConnect(HttpdConnection *	ptConnection) 
{
	/* initialization */
	int 			iRet 				= 0;
	int 			iWifiMode			= 0;
	char 			pszSsid[128]		= { 0 };
	char 			pszPassword[128]	= { 0 };
	static ETSTimer resetTimer;
	
	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}
	
	/* retrieve credentials */
	httpdFindArg(ptConnection->ptPost->pbBuffer, "essid", pszSsid, sizeof(pszSsid));
	httpdFindArg(ptConnection->ptPost->pbBuffer, "passwd", pszPassword, sizeof(pszPassword));

	/* copy to config struct */
	os_strncpy((char *)tTmpStationConfiguration.ssid, pszSsid, WIFI_MAX_SSID_LENGTH);
	os_strncpy((char *)tTmpStationConfiguration.password, pszPassword, WIFI_MAX_PASSWORD_LENGTH);

#ifdef CGIWIFI_DEBUG
	os_printf("cgiWiFiConnect: trying to connect to ap %s with password %s\n", pszSsid, pszPassword);
#endif

	/* connect */
	wifi_station_disconnect();
	wifi_station_set_config(&tTmpStationConfiguration);
	wifi_station_connect();

	/* retrieve wifi mode */
	iWifiMode = wifi_get_opmode();

	if (WIFI_SOFTAP_MODE != iWifiMode)
	{
		/* schedule disconnect/connect */
		os_timer_disarm(&resetTimer);
		os_timer_setfn(&resetTimer, resetTimerCb, NULL);
		os_timer_arm(&resetTimer, 4000, 0);
	}

	iRet = HTTPD_CGI_DONE;

lblCleanup:
	return iRet;
}

/* cgiWifiSetMode */
int ICACHE_FLASH_ATTR cgiWifiSetMode(HttpdConnection *	ptConnection) 
{
	/* initialization */
	int 	iRet 				= 0;
	int 	iWifiMode			= 0;
	int 	cbWifiMode			= 0;
	char 	pszWifiMode[1024]	= { 0 };

	if (NULL == ptConnection->ptEspConnection)
	{
		/* connection aborted - clean up */
		iRet = HTTPD_CGI_DONE;
		goto lblCleanup;
	}

	cbWifiMode = httpdFindArg(ptConnection->pbGetArguments, WIFI_MODE_STRING, pszWifiMode, sizeof(pszWifiMode));
	
#ifdef CGIWIFI_DEBUG
	os_printf("cgiWifiSetMode: %s\n", pszWifiMode);
#endif

	if (WIFI_MODE_STRING_LENGTH == cbWifiMode)
	{
		iWifiMode = atoi(pszWifiMode);

		/* ignore requests to switch to non-wifi mode (0) to prevent chip from losing connectivity */
		if ((0 < iWifiMode) && (4 > iWifiMode))
		{
			/* set mode and restart */
			
#ifdef CGIWIFI_DEBUG
			os_printf("cgiWifiSetMode: setting mode and restarting\n");
#endif

			wifi_set_opmode(iWifiMode);
			system_restart();
		}
	}

	httpdRedirect(ptConnection, "index.tpl");
	
	iRet = HTTPD_CGI_DONE;

lblCleanup:
	return iRet;
}
