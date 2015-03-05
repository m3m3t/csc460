/**
 * @file uart.h
 *
 * @brief This module is responsible for transmitting characters to UART.
 *
 * @author Hong-Yi Wang
 */

/**
 * @brief Initialize the UART by enabling both the transmitter and receiver,
 * setting frame format to be 8 data bits, and setting the bit rate to
 * 38,400 bps.
 */
void uart_init();

/**
 * @brief Transmit a character to UART.
 * @param c	The character to transmit.
 */
void uart_putchar(char c);

/**
 * @brief Transmit an array of characters to UART.
 * @param str		The array of characters to transmit.
 * @param str_size	The size of the character array.
 */
void uart_transmit(char* str, uint8_t str_size);
