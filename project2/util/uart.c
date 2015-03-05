/**
 * @file uart.c
 *
 * @author Hong-Yi Wang
 */
#include <avr/io.h>
#include "uart.h"

/**
 * @brief Initialize the UART by enabling both the transmitter and receiver,
 * setting frame format to be 8 data bits, and setting the bit rate to
 * 38,400 bps.
 */
void uart_init()
{
	// set the U2Xn bit
	// under asynchronous mode,
	// setting this bit will double the transfer rate
	UCSR1A |= _BV(U2X1);

	// enable the transmitter
	UCSR1B |= _BV(TXEN1);

	// enable the receiver
	UCSR1B |= _BV(RXEN1);

	// set frame format: 8 data bits
	UCSR1C |= _BV(UCSZ11)|_BV(UCSZ10);

	// set the bit rate to 38,400 bps
	// please refer to at90usb1287 manual page 205
	UBRR1H = 0;
	UBRR1L = 25;
}

/**
 * @brief Transmit a character to UART.
 * @param c	The character to transmit.
 */
void uart_putchar(char c)
{
	// wait for empty transmit buffer
	while ( !(UCSR1A & _BV(UDRE1)) );

	// put data into buffer, sends the data
	UDR1 = c;
}

/**
 * @brief Transmit an array of characters to UART.
 * @param str		The array of characters to transmit.
 * @param str_size	The size of the character array.
 */
void uart_transmit(char* str, uint8_t str_size)
{
	uint8_t i;
	for (i = 0; i < str_size; i++)
	{
		uart_putchar(str[i]);
	}
}
