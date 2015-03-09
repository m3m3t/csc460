/**
TESTING NOW()

test should set output pin HIGH every period/ time task is run
output pin will go low if Now() is not expected value;
 */

#include "common.h"
#include "os.h"

#define pulse_width 10 //10 os ticks
#define OUTPUT_PIN 7 //digital pin 7;

uint16_t current_time = 0;
uint16_t last_time = 0;

void pulse_pin(void)
{
    //pulse pin every time task run
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN)
        last_time = current_time;
        current_time = Now();
        
        if(current_time - last_time != 10 * 5){
            PORTB = 0;
        }
        Task_Next();
    }
}

int r_main(void)
{
    DDRD = _BV(OUTPUT_PIN)
    PORTB = 0;
    Task_Create_Periodic(pulse_pin, 0, pulse_width, pulse_width - 9, pulse_width);
}
