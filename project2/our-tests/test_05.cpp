/**
TESTING round robin
test should create rr task that pulse their pin every time they are run
 */

#include "common.h"
#include "os.h"

#define pulse_width 100 //10 os ticks
#define OUTPUT_PIN 7 //digital pin 7;


void pulse_pin(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN)
        _delay_ms(50);
        PORTB = 0;
        _delay_ms(10);
    }
}

int r_main(void)
{
    DDRD = _BV(OUTPUT_PIN)
    PORTB = 0;
    Task_Create_RR(pulse_pin, 0);
}

