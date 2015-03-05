#include <string.h>
#include <stdio.h>
#include <avr/io.h>

#include "trace.h"
#include "uart.h"

/** The size of the buffer for storing the traces. */
#define	MAX_NUM_TRACES	128

/** A buffer for storing the traces. */
Trace traces[MAX_NUM_TRACES];

/** The number of traces stored in the buffer. */
static uint8_t num_traces = 0;

/**
 * @brief Initialize the trace module.
 */
void trace_init()
{
	uart_init();
}

/**
 * @brief Record a trace.
 * @param trace	The trace to be recorded.
 * @pre If the buffer for storing the traces is full then calls to trace_add
 * will have no effect.
 */
void trace_add(Trace* trace)
{
	if (num_traces < MAX_NUM_TRACES)
	{
		traces[num_traces].task = trace->task;
		traces[num_traces].function = trace->function;
		traces[num_traces].time = trace->time;
		num_traces++;
	}
}

/**
 * @brief Print the traces to UART.
 * @param display_time Any value other than 0 will cause the time to be
 * displayed.
 */
void print_traces(uint8_t display_time)
{
	uint8_t i;
	char task[10];
	char msg[30];
	for (i = 0; i < num_traces; i++)
	{
		switch (traces[i].task)
		{
			case SYSTEM1:
				snprintf(task,10,"SYSTEM1");
				break;
			case SYSTEM2:
				snprintf(task,10,"SYSTEM2");
				break;
			case SYSTEM3:
				snprintf(task,10,"SYSTEM3");
				break;
			case PERIODIC1:
				snprintf(task,10,"PERIODIC1");
				break;
			case PERIODIC2:
				snprintf(task,10,"PERIODIC2");
				break;
			case PERIODIC3:
				snprintf(task,10,"PERIODIC3");
				break;
			case BRR1:
				snprintf(task,10,"BRR1");
				break;
			case BRR2:
				snprintf(task,10,"BRR2");
				break;
			case BRR3:
				snprintf(task,10,"BRR3");
				break;
		}

		if (display_time)
			snprintf(msg,30,"%10s %u\n\r",task,traces[i].time);
		else
			snprintf(msg,30,"%10s\n\r",task);
		uart_transmit(msg,strlen(msg));
	}
}
