/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		io.c
*	purpose:	io related functions header
************************************************/

#ifndef __IO_H__
#define __IO_H__

#include "shared.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "gpio.h"

#define LEDGPIO 	2
#define BTNGPIO 	0

/************************************************
*	name:			ioLed
*	parameters:		iNewState - gpios new state
*	return value:	none
*	purpose:		set gpios state
************************************************/
void ICACHE_FLASH_ATTR	ioLed(int iNewState);

/************************************************
*	name:			ioInit
*	parameters:		none
*	return value:	none
*	purpose:		initialize gpios
************************************************/
void 					ioInit(void);

#endif //__IO_H__
