/**
TESTING Task_GetArg
test should create 2 rr tasks, that pulse the pin defined in the task->arg
 */

#include "common.h"
#include "os.h"

#define pulse_width 100 //10 os ticks
#define OUTPUT_PIN_1 7 //digital pin 7;
#define OUTPUT_PIN_2 7 //digital pin 7;


void pulse_pin_1(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(Task_GetArg())
        _delay_ms(50);
        PORTB = 0;
        _delay_ms(10);
    }
}

void pulse_pin_2(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(Task_GetArg())
        _delay_ms(50);
        PORTB = 0;
        _delay_ms(10);
    }
}

int r_main(void)
{
    DDRD = _BV(OUTPUT_PIN)
    PORTB = 0;
    Task_Create_RR(pulse_pin_1, OUTPUT_PIN_1);
    Task_Create_RR(pulse_pin_2, OUTPUT_PIN_2);

}
