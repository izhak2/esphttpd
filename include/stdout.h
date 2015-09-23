/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		stdout.h
*	purpose:	stdout related functions header
************************************************/

#ifndef __STDOUT_H__
#define __STDOUT_H__

#include "shared.h"
#include "ets_sys.h"
#include "osapi.h"
#include "uart_hw.h"

/************************************************
*	name:			stdoutInit
*	parameters:		none
*	return value:	none
*	purpose:		initialize stdout
************************************************/
void	stdoutInit(void);

#endif //__STDOUT_H__
