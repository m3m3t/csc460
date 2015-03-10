/*
 * timer3.cpp
 *
 * Created: 09/03/2015 1:32:50 AM
 *  Author: memet
 */ 
#include "timer3.h"

unsigned long timer3::msecs;
void (*timer3::func)();
volatile unsigned long timer3::count;
volatile char timer3::overflowing;
volatile unsigned int timer3::tcnt3;

void timer3::set(unsigned long ms, void (*_callback)()) {
	float prescaler = 64.0;
	TCCR3A = 0;
	TCCR3B = 0;
	tcnt3 =  65536 - (int)((float)F_CPU * 0.001 / prescaler);	
	TCCR3B |= (1 << CS12); // 256 prescaler
	

	if (ms == 0)
		msecs = 1;
	else
		msecs = ms;

	func = _callback;
}

void timer3::start() {
	count = 0;
	overflowing = 0;

	TCNT3 = tcnt3;
	TIMSK3 |= (1<<TOIE3);
}

void timer3::stop() {
	TIMSK3 &= ~(1<<TOIE3);
}

void timer3::_overflow() {
	count += 1;

	if (count >= msecs && !overflowing) {
		overflowing = 1;
		count = 0;
		(*func)();
		overflowing = 0;
	}
}

ISR(TIMER3_OVF_vect) {
	TCNT3 = timer3::tcnt3;
	timer3::_overflow();
}

