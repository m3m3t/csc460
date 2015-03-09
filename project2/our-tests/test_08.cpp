/**
TESTING multple periodic tasks with periods that are multiples of each other
test should create 4 periodic tasks that pulse their pin on period
 */

#include "common.h"
#include "os.h"

#define pulse_width 10 //10 os ticks
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
        Task_Next();
    }
}

void pulse_pin_2(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN_2)
        _delay_ms(50);
        PORTB = 0;
        _delay_ms(10);
        Task_Next();
    }
}
void pulse_pin_3(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN_3)
        _delay_ms(50);
        PORTB = 0;
        _delay_ms(10);
        Task_Next();
    }
}
void pulse_pin_4(void)
{
    //pulse pin on for 50ms off for 10 ms
    for(;;)
    {
        PORTB = _BV(OUTPUT_PIN_4)
        _delay_ms(50);
        PORTB = 0;
        _delay_ms(10);
        Task_Next();
    }
}

int r_main(void)
{
    DDRD = _BV(OUTPUT_PIN)
    PORTB = 0;
    Task_Create_Periodic(pulse_pin_1, 0, pulse_width, 2, pulse_width);
    Task_Create_Periodic(pulse_pin_2, 0, pulse_width*2, 2, pulse_width);
    Task_Create_Periodic(pulse_pin_3, 0, pulse_width*3, 2, pulse_width);
    Task_Create_Periodic(pulse_pin_4, 0, pulse_width*4, 2, pulse_width);

}

