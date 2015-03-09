/**
 * @file   test001.c
 * @author Scott Craig and Justin Tanner
 * @date   Mon Oct 29 16:19:32 2007
 *
 * @brief  Test 001 - sanity test, can we print to UART
 *
 */

#include "common.h"
#include "os.h"

#define pulse_width 10
enum { A=1, B, C, D, E, F, G };
const unsigned int PT = 0;
const unsigned char PPP[] = {};


void pulse(void)
{
    DDRD = _BV(7)//digital pin 7;
    for(;;)
    {
        PORTB = _BV(7)
        _delay_25ms();
        PORTB = 0;
        _delay_25ms();
        Task_Next();
    }
}

int r_main(void)
{
    Task_Create_Periodic(pulse_pin, 0, );
}
