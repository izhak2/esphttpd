/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		heatshrink_decoder.c
*	purpose:	wrapper to avoid moving c-files, sets server-specific configuration
************************************************/

#include "config.h"

#ifdef EFS_HEATSHRINK

#define _STDLIB_H_
#define _STRING_H_
#define _STDDEF_H

#include "c_types.h"
#include "mem.h"
#include "osapi.h"
#include "heatshrink_config_httpd.h"

#define memset(x,y,z) os_memset(x,y,z)
#define memcpy(x,y,z) os_memcpy(x,y,z)

#include "../lib/heatshrink/heatshrink_decoder.c"

#endif
