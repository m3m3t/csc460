/**
 * @file	led.c
 * @author	Hong-Yi Wang, Heng Lien
 * @date	Tue May 19 19:00:00 2009
 *
 * @brief	Implementation of the LED controlling module.
 *
 */

#include "led.h"
#include "util.h"

void led_init(void)
{
  DDRD = 0xFF;
  SETBIT(DDRD, PORTD4);
  SETBIT(DDRD, PORTD5);
  SETBIT(DDRD, PORTD6);
  SETBIT(DDRD, PORTD7);
}

void led_D2_off(void)
{
	CLEARBIT(PORTD, PIND4);
	CLEARBIT(PORTD, PIND5);
}

void led_D2_red_on(void)
{
	SETBIT(PORTD, PIND4);
}

void led_D2_red_off(void)
{
	CLEARBIT(PORTD, PIND4);
}

void led_D2_green_on(void)
{
	SETBIT(PORTD, PIND5);
}


void led_D2_green_off(void)
{
	CLEARBIT(PORTD, PIND5);
}


void led_D5_off(void)
{
	CLEARBIT(PORTD, PIND6);
	CLEARBIT(PORTD, PIND7);
}

void led_D5_red_on(void)
{
	SETBIT(PORTD, PIND7);
}


void led_D5_red_off(void)
{
	CLEARBIT(PORTD, PIND7);
}


void led_D5_green_on(void)
{
	SETBIT(PORTD, PIND6);
}


void led_D5_green_off(void)
{
	CLEARBIT(PORTD, PIND6);
}
