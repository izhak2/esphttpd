/* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
* this notice you can do whatever you want with this stuff. If we meet some day, 
* and you think this stuff is worth it, you can buy me a beer in return. 
*
* project re-write by <izhak2@gmail.com>. for updates follow the changelog
* ---------------------------------------------------------------------------- */

/************************************************
*	file:		stdout.c
*	purpose:	stdout related functions implementation
************************************************/

#include "stdout.h"

/************************************************
*	name:			stdoutUartTxd
*	parameters:		bByte - char to transmit
*	return value:	none
*	purpose:		low level transmit char
************************************************/
static void ICACHE_FLASH_ATTR stdoutUartTxd(char bByte)
{
	/* wait for room in the FIFO */
	while (126 <= ((READ_PERI_REG(UART_STATUS(0)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT));
	
	/* send the character */
	WRITE_PERI_REG(UART_FIFO(0), bByte);
}

/************************************************
*	name:			stdoutPutchar
*	parameters:		bByte - char to put
*	return value:	none
*	purpose:		put char to uart, deal with \r\n
************************************************/
static void ICACHE_FLASH_ATTR stdoutPutchar(char bByte)
{
	/* convert \n to \r\n */
	if (NEWLINE_SYMBOL_STRING == bByte)
	{
		stdoutUartTxd(NEWLINE_R_SYMBOL_STRING);
	}

	stdoutUartTxd(bByte);
}

/* initialize stdout */
void stdoutInit(void)
{
	/* enable TxD pin */
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
	
	/* Set baud rate and other serial parameters to 115200,n,8,1 */
	uart_div_modify(0, UART_CLK_FREQ/BIT_RATE_115200);
	WRITE_PERI_REG(UART_CONF0(0), (STICK_PARITY_DIS)|(ONE_STOP_BIT << UART_STOP_BIT_NUM_S)| \
				(EIGHT_BITS << UART_BIT_NUM_S));

	/* reset tx & rx fifo */
	SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
	CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
	
	/* clear pending interrupts */
	WRITE_PERI_REG(UART_INT_CLR(0), 0xffff);

	/* install our own putchar handler */
	os_install_putc1((void *)stdoutPutchar);
}
