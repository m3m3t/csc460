#include "timer2.h"

unsigned long timer2::msecs;
void (*timer2::func)();
volatile unsigned long timer2::count;
volatile char timer2::overflowing;
volatile unsigned int timer2::tcnt2;

void timer2::set(unsigned long ms, void (*_callback)()) {
	float prescaler = 0.0;

	TIMSK2 &= ~(1<<TOIE2);
	TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
	TCCR2B &= ~(1<<WGM22);
	ASSR &= ~(1<<AS2);
	TIMSK2 &= ~(1<<OCIE2A);

	TCCR2B |= (1<<CS22);
	TCCR2B &= ~((1<<CS21) | (1<<CS20));
	prescaler = 64.0;
	
	tcnt2 = 256 - (int)((float)F_CPU * 0.001 / prescaler);

	if (ms == 0)
		msecs = 1;
	else
		msecs = ms;

	func = _callback;
}

void timer2::start() {
	count = 0;
	overflowing = 0;

	TCNT2 = tcnt2;
	TIMSK2 |= (1<<TOIE2);
}

void timer2::stop() {
	TIMSK2 &= ~(1<<TOIE2);
}

void timer2::_overflow() {
	count += 1;

	if (count >= msecs && !overflowing) {
		overflowing = 1;
		count = 0;
		(*func)();
		overflowing = 0;
	}
}

ISR(TIMER2_OVF_vect) {
	TCNT2 = timer2::tcnt2;
	timer2::_overflow();
}
