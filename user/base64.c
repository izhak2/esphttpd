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
*	purpose:	Base64 functions implementation - adapted from Jon Mayo
************************************************/

#include "base64.h"

static const uint8_t base64dec_tab[256]= {
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
	 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
	255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
	255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

/*	base64_decode	*/
int ICACHE_FLASH_ATTR base64_decode(size_t			cbInputBufferLength, 
									const char *	pszInputBuffer, 
									size_t			cbOutputBufferLength, 
									unsigned char *	pszOutputbuffer)
{
	/* initialization */
	unsigned int 	ii	= 0;
	unsigned int 	io	= 0;
	unsigned int 	rem	= 0;
	uint32_t 		v	= 0;

	for(io = 0, ii = 0, v = 0, rem = 0; ii < cbInputBufferLength; ++ii)
	{
		unsigned char ch;
		if (isspace((int)pszInputBuffer[ii]))
		{
			continue;
		}
		if ((*EQUAL_SYMBOL_STRING) == pszInputBuffer[ii])
		{
			break; /* stop at = */
		}
		
		ch = base64dec_tab[(unsigned int)pszInputBuffer[ii]];
		if (255 == ch)
		{
			/* stop at a parse error */
			break;
		}
		
		v = (v << 6) | ch;
		rem += 6;

		if(8 <= rem)
		{
			rem -= 8;
			if (io >= cbOutputBufferLength)
			{
				/* truncation is failure */
				return -1;
			}
			pszOutputbuffer[io++] = ( v >> rem) & 255;
		}
	}

	if(8 <= rem)
	{
		rem -= 8;
		if (io >= cbOutputBufferLength)
		{
			/* truncation is failure */
			return -1;
		}
		pszOutputbuffer[io++]= (v >> rem) & 255;
	}

	return io;
}

#ifdef BASE64_ENCODE_FUNCTION

static const uint8_t base64enc_tab[64]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* base64_encode 	*/
int base64_encode(size_t 				cbInputBufferLength, 
				  const unsigned char *	pszInputBuffer, 
				  size_t 				cbOutputBufferLength, 
				  char *				pszOutputbuffer)
{
	/* initialization */
	unsigned int 	ii	= 0;
	unsigned int 	io	= 0;
	unsigned int 	rem	= 0;
	uint32_t 		v	= 0;

	for(io = 0, ii = 0, v = 0, rem = 0; ii <cbInputBufferLength; ++ii)
	{
		unsigned char ch;
		ch = pszInputBuffer[ii];
		v = (v << 8) | ch;
		rem += 8;
		while(6 <= rem)
		{
			rem -= 6;
			if (io >= cbOutputBufferLength)
			{
				/* truncation is failure */
				return -1;
			}
			pszOutputbuffer[io++] = base64enc_tab[(v >> rem) & 63];
		}
	}

	if (rem)
	{
		v <<= (6 - rem);
		if (io >= cbOutputBufferLength)
		{
			return -1; /* truncation is failure */
		}
		pszOutputbuffer[io++] = base64enc_tab[v & 63];
	}

	while(io & 3)
	{
		if (io >= cbOutputBufferLength)
		{
			/* truncation is failure */
			return -1;
		}
		pszOutputbuffer[io++] = (*EQUAL_SYMBOL_STRING);
	}

	if (io >= cbOutputBufferLength)
	{
		/* no room for null terminator */
		return -1;
	}

	pszOutputbuffer[io] = 0;
	return io;
}

#endif //BASE64_ENCODE_FUNCTION
