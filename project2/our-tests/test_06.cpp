/**
TESTING multiple round robin
test should create 4 rr tasks that pulse their pin while they are running
 */

#include "common.h"
#include "os.h"

#define pulse_width 100 //10 os ticks
#define OUTPUT_PIN_1 7 //digital pin 7;
#define OUTPUT_PIN_2 6 //digital pin 6;
#define OUTPUT_PIN_3 5 //digital pin 5;
#define OUTPUT_PIN_4 4 //digital pin 4;


void pulse_pin_1(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN_1)
        _delay_ms(50);
        PORTB = 0;
        _delay_ms(10);
    }
}

void pulse_pin_2(void)
{
    //pulse pin on for 100ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN_2)
        _delay_ms(100);
        PORTB = 0;
        _delay_ms(10);
    }
}

void pulse_pin_3(void)
{
    //pulse pin on for 150ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN_3)
        _delay_ms(150);
        PORTB = 0;
        _delay_ms(10);
    }
}

void pulse_pin_4(void)
{
    //pulse pin on for 75ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN_4)
        _delay_ms(75);
        PORTB = 0;
        _delay_ms(10);
    }
}

int r_main(void)
{
    DDRD = _BV(OUTPUT_PIN)
    PORTB = 0;
    Task_Create_RR(pulse_pin_1, 0);
    Task_Create_RR(pulse_pin_1, 0);
    Task_Create_RR(pulse_pin_1, 0);
    Task_Create_RR(pulse_pin_1, 0);
}
