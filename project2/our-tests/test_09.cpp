/**
TESTING periodic and rr simultaneously
test should create 21 of each, rr pulses pin for 25ms continuously, periodic turn on for 25ms, then off util next run
 */

#include "common.h"
#include "os.h"

#define pulse_width 100 //10 os ticks

void pulse_pin_1(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(Task_GetArg())
        _delay_ms(25);
        PORTB = 0;
        _delay_ms(25);
    }
}

void pulse_pin_2(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(Task_GetArg())
        _delay_ms(25);
        PORTB = 0;
    }
}

int r_main(void)
{
    DDRD = _BV(OUTPUT_PIN)
    PORTB = 0;
    Task_Create_RR(pulse_pin_1, 0);
    Task_Create_Periodic(pulse_pin_2, 0, pulse_width, pulse_width-1, pulse_width);

}
