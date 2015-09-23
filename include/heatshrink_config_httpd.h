/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		heatshrink_config_httpd.h
*	purpose:	heatshrink configuration for the decompressor
************************************************/

#ifndef __HEATSHRINK_CONFIG_H__
#define __HEATSHRINK_CONFIG_H__

/* enable dynamic allocation */
#define HEATSHRINK_DYNAMIC_ALLOC				1

/* enable heatshrink debug logging */
#define HEATSHRINK_DEBUGGING_LOGS				0

/* enable indexing for faster compression - requires additional space */
#define HEATSHRINK_USE_INDEX					1

#if HEATSHRINK_DYNAMIC_ALLOC
    /* Optional replacement of malloc/free */
    #define HEATSHRINK_MALLOC(_SIZE) 			os_malloc(_SIZE)
    #define HEATSHRINK_FREE(_ADDRESS, _SIZE)	os_free(_ADDRESS)
#else
    /* Required parameters for static configuration */
    #define HEATSHRINK_STATIC_INPUT_BUFFER_SIZE 32
    #define HEATSHRINK_STATIC_WINDOW_BITS 		8
    #define HEATSHRINK_STATIC_LOOKAHEAD_BITS 	4
#endif //HEATSHRINK_DYNAMIC_ALLOC

#endif	//__HEATSHRINK_CONFIG_H__
