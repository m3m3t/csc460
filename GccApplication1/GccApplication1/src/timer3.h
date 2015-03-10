/*
 * timer3.h
 *
 * Created: 09/03/2015 1:40:11 AM
 *  Author: memet
 */ 


#ifndef TIMER3_H_
#define TIMER3_H_

#include <avr/interrupt.h>

namespace timer3 {
	extern unsigned long msecs;
	extern void (*func)();
	extern volatile unsigned long count;
	extern volatile char overflowing;
	extern volatile unsigned int tcnt3;

	void set(unsigned long ms, void (*_callback)());
	void start();
	void stop();
	void _overflow();
}




#endif /* TIMER3_H_ */