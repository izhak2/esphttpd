/* ----------------------------------------------------------------------------
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		shared.h
*	purpose:	shared between all files
************************************************/

#ifndef	__SHARED_H__
#define	__SHARED_H__

#include "config.h"

#define	NULL_STRING						'\0'
#define	EXTENSION_SYMBOL_STRING			"."
#define EQUAL_SYMBOL_STRING 			"="
#define PLUS_SYMBOL_STRING				"+"
#define AMPERSAND_SYMBOL_STRING			"&"
#define COLON_SYMBOL_STRING				":"
#define SPACE_SYMBOL_STRING				" "
#define QUERY_SYMBOL_STRING				"?"
#define ASTERISK_SYMBOL_STRING			"*"
#define DASH_SYMBOL_STRING				"-"
#define NEWLINE_SYMBOL_STRING			"\n"
#define NEWLINE_R_SYMBOL_STRING			"\r"
#define PERCENTAGE_SYMBOL_STRING		"%%"
#define	SINGLE_PRECENTAGE_SYMBOL_STRING	"%"

#define malloc	os_malloc

#define UNUSED(x)						(void)(x)

#endif	//__SHARED_H__
