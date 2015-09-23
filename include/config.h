/* ----------------------------------------------------------------------------
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		config.h
*	purpose:	general configuration for build
************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* heatshrink support settings */
#define	EFS_HEATSHRINK

/* OTA settings */
// embedded in makefile, look for 'OTA' variable

/* filesystem position in flash */
#ifndef	OTA
#define	ESPFS_PART 	1
#define	ESPFS_PART2	1
#else
#define	ESPFS_PART 	4
#define	ESPFS_PART2	1
#endif

/* base64 settings */
//#define BASE64_ENCODE_FUNCTION

/* espfs test settings */
//#define	ESPFS_TEST

/* debug strings settings */
#define	AUTH_DEBUG
#define	BASE64_DEBUG
#define	CGI_DEBUG
#define	CGIFLASH_DEBUG
#define	CGIWIFI_DEBUG
#define	ESPFS_DEBUG
#define	FLASH_DEBUG
#define	HTTPD_DEBUG
#define	IO_DEBUG
#define	STDOUT_DEBUG
#define	USER_MAIN_DEBUG

/* hotspot settings */
#define	HOTSPOT_SSID_STRING		"ESPHTTPD"
#define	HOTSPOT_PASSWORD_STRING	"12345678"

/* httpd settings */
#define HTTPD_MAX_CONNECTIONS	8		//Max amount of connections

/* httpd authentication settings */
//#define	ENABLE_HTTP_AHTENTICATION

#ifdef 	ENABLE_HTTP_AHTENTICATION
#define	WEB_USERNAME_STRING		"admin"
#define	WEB_PASSWORD_STRING		"s3cr3t"
#endif

/* httpd realm setting */
#define HTTP_AUTH_REALM 		"Protected"

#endif //__CONFIG_H__
