/*
 * timer.h
 *
 * Created: 09/03/2015 12:17:16 AM
 *  Author: memet
 */ 


#ifndef TIMER2_H_
#define TIMER2_H_

#include <avr/interrupt.h>

namespace timer2 {
	extern unsigned long msecs;
	extern void (*func)();
	extern volatile unsigned long count;
	extern volatile char overflowing;
	extern volatile unsigned int tcnt2;

	void set(unsigned long ms, void (*_callback)());
	void start();
	void stop();
	void _overflow();
}




#endif /* TIMER2_H_ */