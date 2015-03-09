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
//#include "uart.h"
//#include "trace/trace.h"

enum { A=1, B, C, D, E, F, G };
const unsigned int PT = 0;
const unsigned char PPP[] = {};

//EVENT* print_event;

void round_robin(void)
{
    for(;;)
    {
       //Event_Signal(print_event);
        Task_Next();
    }
}

int main(void)
{
    /* setup the test */
    //uart_init();
    //uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    //set_test(1);

   // print_event = Event_Init();

    /* Run clock at 8MHz. */
    //TCCR3B = _BV(CS30);

    Task_Create_RR(round_robin, 1);

    //Event_Wait(print_event);
    //print_trace();
}
