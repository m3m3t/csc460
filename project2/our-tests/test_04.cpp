/**
TESTING periodic short
test should pulse the output pin  every period/ time task is run
 */

#include "common.h"
#include "os.h"

#define pulse_width 2 //10 os ticks
#define OUTPUT_PIN 7 //digital pin 7;

void pulse_pin(void)
{
    //pulse pin every time task run
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN)
        PORTB = 0;
        Task_Next();
    }
}

int r_main(void)
{
    DDRD = _BV(OUTPUT_PIN)
    PORTB = 0;
    Task_Create_Periodic(pulse_pin, 0, pulse_width, pulse_width - 9, pulse_width);
}
