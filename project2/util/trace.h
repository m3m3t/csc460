#include <avr/io.h>

/** @brief Define the parameters of a trace. */
typedef struct _trace
{
	/** The id of the task to trace. */
	uint8_t task;
	/** The id of the function to trace. */
	uint8_t function;
	/** The time when the trace is recorded (in number of ticks) */
	uint16_t time;
} Trace;

enum {
	SYSTEM1=1,
	SYSTEM2,
	SYSTEM3,
	PERIODIC1,
	PERIODIC2,
	PERIODIC3,
	BRR1,
	BRR2,
	BRR3
};

/**
 * @brief Initialize the trace module.
 */
void trace_init();

/**
 * @brief Record a trace.
 * @param trace	The trace to be recorded.
 * @pre If the buffer for storing the traces is full then calls to trace_add
 * will have no effect.
 */
void trace_add(Trace* trace);

/**
 * @brief Print the traces to UART.
 * @param display_time Any value other than 0 will cause the time to be
 * displayed.
 */
void print_traces(uint8_t display_time);
