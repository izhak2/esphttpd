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
*	purpose:	io related functions implementation
************************************************/

#include "io.h"

/* ioLed */
void ICACHE_FLASH_ATTR ioLed(int iNewState)
{
	if (iNewState)
	{
		/* enable */
		gpio_output_set((1 << LEDGPIO), 0, (1 << LEDGPIO), 0);
		gpio_output_set((1 << BTNGPIO), 0, (1 << BTNGPIO), 0);
	}
	else
	{
		/* disable */
		gpio_output_set(0, (1 << LEDGPIO), (1 << LEDGPIO), 0);
		gpio_output_set(0, (1 << BTNGPIO), (1 << BTNGPIO), 0);
	}
}

/* ioInit */
void ioInit(void)
{
	/* choose gpios */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);

	/* set to 0 */
	gpio_output_set(0, (1 << LEDGPIO), (1 << LEDGPIO), 0);
	gpio_output_set(0, (1 << BTNGPIO), (1 << BTNGPIO), 0);
}
