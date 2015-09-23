/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		base64.h
*	purpose:	Base64 functions header
************************************************/

#ifndef	__BASE64_H__
#define	__BASE64_H__

#include "shared.h"
#include "c_types.h"

/************************************************
*	name:			base64_decode
*	parameters:		cbInputBufferLength		- pszInputBuffer length
*					pszInputBuffer			- input buffer
*					cbOutputBufferLength	- pszOutputbuffer length
*					pszOutputbuffer			- output buffer
*	return value:	
*	purpose:		base64 decoder
************************************************/
int base64_decode(size_t			cbInputBufferLength, 
				  const char *		pszInputBuffer, 
				  size_t			cbOutputBufferLength, 
				  unsigned char *	pszOutputbuffer);

#ifdef BASE64_ENCODE_FUNCTION
/************************************************
*	name:			base64_encode
*	parameters:		cbInputBufferLength		- pszInputBuffer length
*					pszInputBuffer			- input buffer
*					cbOutputBufferLength	- pszOutputbuffer length
*					pszOutputbuffer			- output buffer
*	return value:	
*	purpose:		base64 encoder
************************************************/
int base64_encode(size_t				cbInputBufferLength, 
				  const unsigned char *	pszInputBuffer, 
				  size_t				cbOutputBufferLength,	
				  char *				pszOutputbuffer);
#endif //BASE64_ENCODE_FUNCTION

#endif	//__BASE64_H__
