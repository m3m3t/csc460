/**
 * @file	led.h
 * @author	Hong-Yi Wang, Heng Lien
 * @date	Tue May 19 19:00:00 2009
 *
 * @brief	Header file for led controlling module
 *
 */

#ifndef _LED_H_
#define _LED_H_

#include <avr/io.h>

/**
 * Initialization for LEDS: D2(red, green) and D5(red, green)
 */
void led_init(void);

/**
 * Turn off both leds (red and green) on D2
 */
void led_D2_off(void);

/**
 * Turn on D2.Red Led
 */
void led_D2_red_on(void);

/**
 * Turn off D2.Red Led
 */
void led_D2_red_off(void);

/**
 * Turn on D2.Green Led
 */
void led_D2_green_on(void);

/**
 * Turn off D2.Green led
 */
void led_D2_green_off(void);

/**
 * Turn off both leds (red, green) on D5
 */
void led_D5_off(void);

/**
 * Turn on D5.Red led
 */
void led_D5_red_on(void);

/**
 * Turn off D5.Red led
 */
void led_D5_red_off(void);

/**
 * Turn on D5.Green led
 */
void led_D5_green_on(void);

/**
 * Turn off D5.Green led
 */
void led_D5_green_off(void);

#endif /** END _LED_H_ **/
