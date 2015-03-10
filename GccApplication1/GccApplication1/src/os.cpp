/**
* @file os.c
*
* @brief A Real Time Operating System
*
* Our implementation of the operating system described by Mantis Cheng in os.h.
*
* @author Scott Craig
* @author Justin Tanner
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "kernel.h"
#include "error_code.h"

/* Needed for memset */
//#include <string.h>

/** @brief main function provided by user application. The first task to run. */
int r_main();

/** PPP and PT defined in user application. */
//extern const unsigned char PPP[];

/** PPP and PT defined in user application. */
//extern const unsigned int PT;

/** The task descriptor of the currently RUNNING task. */
static task_descriptor_t* cur_task = NULL;

/** Since this is a "full-served" model, the kernel is executing using its own stack. */
static volatile uint16_t kernel_sp;

/** This table contains all task descriptors, regardless of state, plus idler. */
static task_descriptor_t task_desc[MAXPROCESS + 1];

/** The special "idle task" at the end of the descriptors array. */
static task_descriptor_t* idle_task = &task_desc[MAXPROCESS];

/** The current kernel request. */
static volatile kernel_request_t kernel_request;

/** Arguments for Task_Create() request. */
static volatile create_args_t kernel_request_create_args;

/** Return value for Task_Create() request. */
static volatile int kernel_request_retval;

/** Number of tasks created so far */
static queue_t dead_pool_queue;

/** The ready queue for RR tasks. Their scheduling is round-robin. */
static queue_t rr_queue;

/** The ready queue for Periodic tasks. */
static queue_t per_queue;

/** The ready queue for SYSTEM tasks. Their scheduling is first come, first served. */
static queue_t system_queue;

/** time remaining in current slot */
static volatile uint8_t ticks_remaining = 0;


/** Index of name of task in current slot in PPP array. An even number from 0 to 2*(PT-1). */
//static unsigned int slot_name_index = 0;

/** The task descriptor for index "name of task" */
//static task_descriptor_t* name_to_task_ptr[MAXNAME + 1];

/** The task descriptors for tasks in PPP */
static task_descriptor_t* task_in_PPP[MAXPROCESS];

/** Error message used in OS_Abort() */
static uint8_t volatile error_msg = ERR_RUN_1_USER_CALLED_OS_ABORT;

/**running time */
static uint16_t volatile running_time = 0;
static uint16_t volatile timer_time = 0;
/* Forward declarations */
/* kernel */
static void kernel_main_loop(void);
static void kernel_dispatch(void);
static void kernel_handle_request(void);
/* context switching */
static void exit_kernel(void) __attribute((noinline, naked));
static void enter_kernel(void) __attribute((noinline, naked));
extern "C" void TIMER1_COMPA_vect(void) __attribute__ ((signal, naked));

static int kernel_create_task();
static void kernel_terminate_task(void);
/* queues */

static void enqueue(queue_t* queue_ptr, task_descriptor_t* task_to_add);
static task_descriptor_t* dequeue(queue_t* queue_ptr);

static void kernel_update_ticker(void);
//static void check_PPP_names(void);
static void idle (void);
static void _delay_25ms(void);

static void abort(int msg);

struct service {
	int16_t value;
	uint16_t waiting;
	queue_t task_list;
};

static SERVICE services[MAXSERVICE];
static uint16_t service_cntr = 0;

static uint16_t ppp_tasks_len = 0;


/*
* FUNCTIONS
*/
/**
*  @brief The idle task does nothing but busy loop.
*/
static void idle (void)
{
	for(;;)
	{
	}
}


/**
* @fn kernel_main_loop
*
* @brief The heart of the RTOS, the main loop where the kernel is entered and exited.
*
* The complete function is:
*
*  Loop
*<ol><li>Select and dispatch a process to run</li>
*<li>Exit the kernel (The loop is left and re-entered here.)</li>
*<li>Handle the request from the process that was running.</li>
*<li>End loop, go to 1.</li>
*</ol>
*/
static void kernel_main_loop(void)
{
	for(;;)
	{
		kernel_dispatch();
		
		exit_kernel();
		
		/* if this task makes a system call, or is interrupted,
		* the thread of control will return to here. */
		
		kernel_handle_request();
	}
}


/**
* @fn kernel_dispatch
*
*@brief The second part of the scheduler.
*
* Chooses the next task to run.
*
*/
static void kernel_dispatch(void)
{
	/* If the current state is RUNNING, then select it to run again.
	* kernel_handle_request() has already determined it should be selected.
	*/
	
	if(cur_task->state != RUNNING || cur_task == idle_task)
	{
		
		if(system_queue.head != NULL)
		{
			cur_task = dequeue(&system_queue);
		}
		else if(per_queue.head != NULL)
		{
			/* Keep running the current PERIODIC task. */
			cur_task = dequeue(&per_queue);
		}
		else if(rr_queue.head != NULL)
		{
			cur_task = dequeue(&rr_queue);
		}
		else
		{
			/* No task available, so idle. */
			cur_task = idle_task;
		}
		
		cur_task->state = RUNNING;
	}
}


/**
* @fn kernel_handle_request
*
*@brief The first part of the scheduler.
*
* Perform some action based on the system call or timer tick.
* Perhaps place the current process in a ready or waitng queue.
*/
static void kernel_handle_request(void)
{
	switch(kernel_request)
	{
	case NONE:
		/* Should not happen. */
		break;
		
	case TIMER_EXPIRED:
		kernel_update_ticker();
		
		/* Round robin tasks get pre-empted on every tick. */
		if(cur_task->level == RR && cur_task->state == RUNNING)
		{
			cur_task->state = READY;
			enqueue(&rr_queue, cur_task);
		}
		break;
		
	case TASK_CREATE:
		kernel_request_retval = kernel_create_task();
		
		/* Check if new task has higer priority, and that it wasn't an ISR
		* making the request.
		*/
		if(kernel_request_retval)
		{
			/* If new task is SYSTEM and cur is not, then don't run old one */
			if(kernel_request_create_args.level == SYSTEM && cur_task->level != SYSTEM)
			{
				cur_task->state = READY;
			}
			

			/* If cur is RR, it will be pre-empted by a new SYSTEM. */
			if(cur_task->level == RR &&
			kernel_request_create_args.level == SYSTEM){
				cur_task->state = READY;
			}
			/* If cur is RR, it might be pre-empted by a new PERIODIC. */
			if(cur_task->level == RR &&
			kernel_request_create_args.level == PERIODIC){
				if(kernel_request_create_args.start == running_time){
					cur_task->state = READY;
				}
				
			}
			
			/* enqueue READY RR tasks. */
			if(cur_task->level == RR && cur_task->state == READY)
			{
				enqueue(&rr_queue, cur_task);
			}
		}
		break;
		
	case TASK_TERMINATE:
		if(cur_task != idle_task)
		{
			kernel_terminate_task();
		}
		break;
	case TASK_NEXT:
		switch(cur_task->level)
		{
		case SYSTEM:
			enqueue(&system_queue, cur_task);
			break;
			
		case PERIODIC:
			cur_task->state = WAITING;
			cur_task->time_remaining = 0;
			break;
			
		case RR:
			enqueue(&rr_queue, cur_task);
			break;
			
		default: /* idle_task */
			break;
		}
		
		cur_task->state = READY;
		break;
		
	case TASK_GET_ARG:
		/* Should not happen. Handled in task itself. */
		break;
		
	default:
		/* Should never happen */
		error_msg = ERR_RUN_5_RTOS_INTERNAL_ERROR;
		OS_Abort();
		break;
	}
	
	kernel_request = NONE;
}


/*
* Context switching
*/
/**
* It is important to keep the order of context saving and restoring exactly
* in reverse. Also, when a new task is created, it is important to
* initialize its "initial" context in the same order as a saved context.
*
* Save r31 and SREG on stack, disable interrupts, then save
* the rest of the registers on the stack. In the locations this macro
* is used, the interrupts need to be disabled, or they already are disabled.
*/
#define    SAVE_CTX_TOP()       asm volatile (\
"push   r31             \n\t"\
"in     r31,0x3c        \n\t"\
"push   r31             \n\t"\
"in     r31,__SREG__    \n\t"\
"cli                    \n\t"::); /* Disable interrupt */

#define STACK_SREG_SET_I_BIT()    asm volatile (\
"ori    r31, 0x80        \n\t"::);

#define    SAVE_CTX_BOTTOM()       asm volatile (\
"push   r31             \n\t"\
"push   r30             \n\t"\
"push   r29             \n\t"\
"push   r28             \n\t"\
"push   r27             \n\t"\
"push   r26             \n\t"\
"push   r25             \n\t"\
"push   r24             \n\t"\
"push   r23             \n\t"\
"push   r22             \n\t"\
"push   r21             \n\t"\
"push   r20             \n\t"\
"push   r19             \n\t"\
"push   r18             \n\t"\
"push   r17             \n\t"\
"push   r16             \n\t"\
"push   r15             \n\t"\
"push   r14             \n\t"\
"push   r13             \n\t"\
"push   r12             \n\t"\
"push   r11             \n\t"\
"push   r10             \n\t"\
"push   r9              \n\t"\
"push   r8              \n\t"\
"push   r7              \n\t"\
"push   r6              \n\t"\
"push   r5              \n\t"\
"push   r4              \n\t"\
"push   r3              \n\t"\
"push   r2              \n\t"\
"push   r1              \n\t"\
"push   r0              \n\t"::);

/**
* @brief Push all the registers and SREG onto the stack.
*/
#define    SAVE_CTX()    SAVE_CTX_TOP();SAVE_CTX_BOTTOM();

/**
* @brief Pop all registers and the status register.
*/
#define    RESTORE_CTX()    asm volatile (\
"pop    r0                \n\t"\
"pop    r1                \n\t"\
"pop    r2                \n\t"\
"pop    r3                \n\t"\
"pop    r4                \n\t"\
"pop    r5                \n\t"\
"pop    r6                \n\t"\
"pop    r7                \n\t"\
"pop    r8                \n\t"\
"pop    r9                \n\t"\
"pop    r10             \n\t"\
"pop    r11             \n\t"\
"pop    r12             \n\t"\
"pop    r13             \n\t"\
"pop    r14             \n\t"\
"pop    r15             \n\t"\
"pop    r16             \n\t"\
"pop    r17             \n\t"\
"pop    r18             \n\t"\
"pop    r19             \n\t"\
"pop    r20             \n\t"\
"pop    r21             \n\t"\
"pop    r22             \n\t"\
"pop    r23             \n\t"\
"pop    r24             \n\t"\
"pop    r25             \n\t"\
"pop    r26             \n\t"\
"pop    r27             \n\t"\
"pop    r28             \n\t"\
"pop    r29             \n\t"\
"pop    r30             \n\t"\
"pop    r31             \n\t"\
"out    __SREG__, r31    \n\t"\
"pop    r31             \n\t"\
"out    0x3c, r31    \n\t"\
"pop    r31             \n\t"::);


/**
* @fn exit_kernel
*
* @brief The actual context switching code begins here.
*
* This function is called by the kernel. Upon entry, we are using
* the kernel stack, on top of which is the address of the instruction
* after the call to exit_kernel().
*
* Assumption: Our kernel is executed with interrupts already disabled.
*
* The "naked" attribute prevents the compiler from adding instructions
* to save and restore register values. It also prevents an
* automatic return instruction.
*/
static void exit_kernel(void)
{
	/*
	* The PC was pushed on the stack with the call to this function.
	* Now push on the I/O registers and the SREG as well.
	*/
	SAVE_CTX();
	
	/*
	* The last piece of the context is the SP. Save it to a variable.
	*/
	kernel_sp = SP;
	
	/*
	* Now restore the task's context, SP first.
	*/
	SP = (uint16_t)(cur_task->sp);
	
	/*
	* Now restore I/O and SREG registers.
	*/
	RESTORE_CTX();
	
	/*
	* return explicitly required as we are "naked".
	* Interrupts are enabled or disabled according to SREG
	* recovered from stack, so we don't want to explicitly
	* enable them here.
	*
	* The last piece of the context, the PC, is popped off the stack
	* with the ret instruction.
	*/
	asm volatile ("ret\n"::);
}


/**
* @fn enter_kernel
*
* @brief All system calls eventually enter here.
*
* Assumption: We are still executing on cur_task's stack.
* The return address of the caller of enter_kernel() is on the
* top of the stack.
*/
static void enter_kernel(void)
{
	/*
	* The PC was pushed on the stack with the call to this function.
	* Now push on the I/O registers and the SREG as well.
	*/
	SAVE_CTX();
	
	/*
	* The last piece of the context is the SP. Save it to a variable.
	* 17 bit stack pointer, stored as EIND (extended indirect register), sp High bits, sp low bits
	*/
	cur_task->sp = (uint8_t*) SP;
	
	/*
	* Now restore the kernel's context, SP first.
	*/
	SP = kernel_sp;
	
	/*
	* Now restore I/O and SREG registers.
	*/
	RESTORE_CTX();
	
	/*
	* return explicitly required as we are "naked".
	*
	* The last piece of the context, the PC, is popped off the stack
	* with the ret instruction.
	*/
	asm volatile ("ret\n"::);
}


/**
* @fn TIMER1_COMPA_vect
*
* @brief The interrupt handler for output compare interrupts on Timer 1
*
* Used to enter the kernel when a tick expires.
*
* Assumption: We are still executing on the cur_task stack.
* The return address inside the current task code is on the top of the stack.
*
* The "naked" attribute prevents the compiler from adding instructions
* to save and restore register values. It also prevents an
* automatic return instruction.
*/
void TIMER1_COMPA_vect(void)
{
	//PORTB ^= _BV(PB7);		// Arduino LED
	/*
	* Save the interrupted task's context on its stack,
	* and save the stack pointer.
	*
	* On the cur_task's stack, the registers and SREG are
	* saved in the right order, but we have to modify the stored value
	* of SREG. We know it should have interrupts enabled because this
	* ISR was able to execute, but it has interrupts disabled because
	* it was stored while this ISR was executing. So we set the bit (I = bit 7)
	* in the stored value.
	*/
	SAVE_CTX_TOP();
	
	STACK_SREG_SET_I_BIT();
	
	SAVE_CTX_BOTTOM();
	
	/*
	* Save the tasks stack pointer
	*/
	cur_task->sp = (uint8_t*)SP;
	
	/*
	* Now that we already saved a copy of the stack pointer
	* for every context including the kernel, we can move to
	* the kernel stack and use it. We will restore it again later.
	*/
	SP = kernel_sp;
	
	/*
	* Inform the kernel that this task was interrupted.
	*/
	kernel_request = TIMER_EXPIRED;
	
	/*
	* Restore the kernel context. (The stack pointer is restored again.)
	*/
	SP = kernel_sp;
	
	/*
	* Now restore I/O and SREG registers.
	*/
	RESTORE_CTX();
	
	/*
	* We use "ret" here, not "reti", because we do not want to
	* enable interrupts inside the kernel.
	* Explilictly required as we are "naked".
	*
	* The last piece of the context, the PC, is popped off the stack
	* with the ret instruction.
	*/
	asm volatile ("ret\n"::);
}


/*
* Tasks Functions
*/
/**
*  @brief Kernel function to create a new task.
*
* When creating a new task, it is important to initialize its stack just like
* it has called "enter_kernel()"; so that when we switch to it later, we
* can just restore its execution context on its stack.
* @sa enter_kernel
*/
static int kernel_create_task()
{
	/* The new task. */
	task_descriptor_t *p;
	uint8_t* stack_bottom;
	
	
	if (dead_pool_queue.head == NULL)
	{
		/* Too many tasks! */
		return 0;
	}
	
	
	/* idling "task" goes in last descriptor. */
	if(kernel_request_create_args.level == NULL)
	{
		p = &task_desc[MAXPROCESS];
	}
	/* Find an unused descriptor. */
	else
	{
		p = dequeue(&dead_pool_queue);
	}
	
	stack_bottom = &(p->stack[MAXSTACK-1]);
	
	/* The stack grows down in memory, so the stack pointer is going to end up
	* pointing to the location 32 + 1 + +1 + 3 + 3 = 40 bytes above the bottom, to make
	* room for (from bottom to top):
	*   the address of Task_Terminate() to destroy the task if it ever returns,
	*   the address of the start of the task to "return" to the first time it runs,
	*   register 31,
	*	EIND
	*   the stored SREG, and
	*   registers 30 to 0.
	*/
	uint8_t* stack_top = stack_bottom - (32 + 1 + 1 + 3 + 3);
	
	/* Not necessary to clear the task descriptor. */
	/* memset(p,0,sizeof(task_descriptor_t)); */
	
	/* stack_top[0] is the byte above the stack.
	* stack_top[1] is r0. */
	stack_top[2] = (uint8_t) 0; /* r1 is the "zero" register. */
	/* stack_top[31] is r30. */
	stack_top[32] = (uint8_t) _BV(SREG_I); /* set SREG_I bit in stored SREG. */
	/* stack_top[33] is EIND. */
	/* stack_top[34] is r31. */
	
	/* We are placing the address (16-bit) of the functions
	* onto the stack in reverse byte order (least significant first, followed
	* by most significant).  This is because the "return" assembly instructions
	* (ret and reti) pop addresses off in BIG ENDIAN (most sig. first, least sig.
	* second), even though the AT90 is LITTLE ENDIAN machine.
	*/
	stack_top[35] = (uint8_t)0; //EIND
	stack_top[36] = (uint8_t)((uint16_t)(kernel_request_create_args.f) >> 8);
	stack_top[37] = (uint8_t)(uint16_t)(kernel_request_create_args.f);
	
	stack_top[38] = (uint8_t)0; //EIND
	stack_top[39] = (uint8_t)((uint16_t)Task_Terminate >> 8);
	stack_top[40] = (uint8_t)(uint16_t)Task_Terminate;
	
	/*
	* Make stack pointer point to cell above stack (the top).
	* Make room for 32 registers, SREG and two return addresses.
	* 17 bit stack pointer, stored as EIND (extended indirect register), sp High bits, sp low bits
	*/
	p->sp = stack_top;
	
	p->state = READY;
	p->arg = kernel_request_create_args.arg;
	p->level = kernel_request_create_args.level;
	p->name = kernel_request_create_args.name;
	p->period = kernel_request_create_args.period;
	p->wcet = kernel_request_create_args.wcet;
	p->start = kernel_request_create_args.start; //when to start the periodic task
	p->time_remaining = p->start - running_time;
	p->wcet_remaining = p->wcet;
	
	switch(kernel_request_create_args.level)
	{
		case PERIODIC:
		/* Put this newly created PPP task into the PPP lookup array */
		//name_to_task_ptr[kernel_request_create_args.name] = p;
		
		if(ppp_tasks_len < MAXPROCESS - 1){
			task_in_PPP[ppp_tasks_len] = p;
			ppp_tasks_len++;
		}else{
			//too many
			error_msg = ERR_RUN_7_PERIODIC_TOO_MANY;
			OS_Abort();
		}
		
		break;
		
		case SYSTEM:
		/* Put SYSTEM and Round Robin tasks on a queue. */
		enqueue(&system_queue, p);
		break;
		
		case RR:
		/* Put SYSTEM and Round Robin tasks on a queue. */
		enqueue(&rr_queue, p);
		break;
		
		default:
		/* idle task does not go in a queue */
		break;
	}
	
	
	return 1;
}


/**
* @brief Kernel function to destroy the current task.
*/
static void kernel_terminate_task(void)
{
	/* deallocate all resources used by this task */
	cur_task->state = DEAD;
	if(cur_task->level == PERIODIC)
	{
		//name_to_task_ptr[cur_task->name] = NULL;
		for(int i=0; i < MAXPROCESS; i++){
			if(cur_task == task_in_PPP[i]){
				PORTB = (uint8_t)(_BV(PB7));
				task_in_PPP[i] = NULL;
			}
		}
		
	}
	enqueue(&dead_pool_queue, cur_task);
}

/*
* Queue manipulation.
*/

/**
* @brief Add a task the head of the queue
*
* @param queue_ptr the queue to insert in
* @param task_to_add the task descriptor to add
*/
static void enqueue(queue_t* queue_ptr, task_descriptor_t* task_to_add)
{
	task_to_add->next = NULL;
	
	if(queue_ptr->head == NULL)
	{
		/* empty queue */
		queue_ptr->head = task_to_add;
		queue_ptr->tail = task_to_add;
	}
	else
	{
		/* put task at the back of the queue */
		queue_ptr->tail->next = task_to_add;
		queue_ptr->tail = task_to_add;
	}
}


/**
* @brief Pops head of queue and returns it.
*
* @param queue_ptr the queue to pop
* @return the popped task descriptor
*/
static task_descriptor_t* dequeue(queue_t* queue_ptr)
{
	task_descriptor_t* task_ptr = queue_ptr->head;
	
	if(queue_ptr->head != NULL)
	{
		queue_ptr->head = queue_ptr->head->next;
		task_ptr->next = NULL;
	}
	
	return task_ptr;
}


/**
* @brief Update the current time.
*
* Perhaps move to the next time slot of the PPP.
*/
static void kernel_update_ticker(void)
{
	/* PORTD ^= LED_D5_RED; */
	
	++running_time;
	timer_time = TCNT1;
	
	if(cur_task != NULL && cur_task->state == RUNNING && cur_task->level == PERIODIC){
		//check worst ex. time remaining on task, abort if 0 time
		if(cur_task->wcet > 0){
			cur_task->wcet--;
			}else{
			error_msg = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;
			OS_Abort();
		}
	}
	//update periodic tasks
	for(int i = 0; i < MAXPROCESS; i++){
		//check not null
		if(task_in_PPP[i] != NULL){
			
			task_in_PPP[i]->time_remaining --;
			//check time remaining
			if(task_in_PPP[i]->time_remaining <= 0){
				//if no periodic currently running/ready
					task_in_PPP[i]->state = READY;
					enqueue(&per_queue, task_in_PPP[i]);
					task_in_PPP[i]->time_remaining = task_in_PPP[i]->period;
					task_in_PPP[i]->wcet_remaining = task_in_PPP[i]->wcet;
			
			}
			
		}
	}
	
/*	//scheduling conflicts
	if( cur_task != NULL &&
	cur_task->level == PERIODIC &&
	per_queue.head != per_queue.tail)
	{
		error_msg = ERR_RUN_6_PERIODIC_CONFLICT;
		OS_Abort();
	}*/
}


#undef SLOW_CLOCK

#ifdef SLOW_CLOCK
/**
* @brief For DEBUGGING to make the clock run slower
*
* Divide CLKI/O by 64 on timer 1 to run at 125 kHz  CS3[210] = 011
* 1 MHz CS3[210] = 010
*/
static void kernel_slow_clock(void)
{
	TCCR1B &= ~(_BV(CS12) | _BV(CS10));
	TCCR1B |= (_BV(CS11));
}
#endif

/**
* @brief Setup the RTOS and create main() as the first SYSTEM level task.
*
* Point of entry from the C runtime crt0.S.
*/
void OS_Init()
{
	int i;
	
	/* Set up the clocks */
	TCCR1A = 0;
	TCCR1B |= (_BV(CS11));
	
	#ifdef SLOW_CLOCK
	kernel_slow_clock();
	#endif
	
	
	
	/*
	* Initialize dead pool to contain all but last task descriptor.
	*
	* DEAD == 0, already set in .init4
	*/
	for (i = 0; i < MAXPROCESS - 1; i++)
	{
		task_desc[i].state = DEAD;
		task_in_PPP[i] = NULL;
		//name_to_task_ptr[i] = NULL;
		task_desc[i].next = &task_desc[i + 1];
	}
	task_in_PPP[MAXPROCESS-1] = NULL;
	task_desc[MAXPROCESS - 1].next = NULL;
	dead_pool_queue.head = &task_desc[0];
	dead_pool_queue.tail = &task_desc[MAXPROCESS - 1];
	
	/* Create idle "task" */
	kernel_request_create_args.f = (voidfuncvoid_ptr)idle;
	kernel_request_create_args.level = NULL;
	kernel_create_task();
	
	/* Create "main" task as SYSTEM level. */
	kernel_request_create_args.f = (voidfuncvoid_ptr)r_main;
	kernel_request_create_args.level = SYSTEM;
	kernel_create_task();
	
	/* First time through. Select "main" task to run first. */
	cur_task = task_desc;
	cur_task->state = RUNNING;
	dequeue(&system_queue);
	
	/* Set up Timer 1 Output Compare interrupt,the TICK clock. */
	TIMSK1 |= _BV(OCIE1A);
	OCR1A = TICK_CYCLES;
	TCNT1 = 0;
	/* Clear flag. */
	TIFR1 = _BV(OCF1A);
	
	//NOTE
	/*
	* The main loop of the RTOS kernel.
	*/
	kernel_main_loop();
}




/**
*  @brief Delay function adapted from <util/delay.h>
*/
static void _delay_25ms(void)
{
	//uint16_t i;
	
	/* 4 * 50000 CPU cycles = 25 ms */
	//asm volatile ("1: sbiw %0,1" "\n\tbrne 1b" : "=w" (i) : "0" (50000));
	_delay_ms(25);
}


/** @brief Abort the execution of this RTOS due to an unrecoverable erorr.
*/
void OS_Abort(void)
{
	uint8_t i, j;
	uint8_t flashes;
	
	Disable_Interrupt();
	
	/* Initialize port for output */
	DDRB = ERROR_LED;
	
	if(error_msg < ERR_RUN_1_USER_CALLED_OS_ABORT)
	{
		flashes = error_msg + 1; // initialize time error
		
		for(;;){
			//flash on for 2 second on, off for 1 seconds
			PORTB = ERROR_LED;
			for(;;){
				for(i = 0; i < 80; ++i){
					_delay_25ms();
				}
			}
			PORTB = (uint8_t) 0;
			for(;;){
				for(i = 0; i < 40; ++i){
					_delay_25ms();
				}
			}
			
			// flash number of error code half second on. half off
			
			for(j = 0; j < flashes; j++){
				PORTB = ERROR_LED;
				for(i = 0; i < 20; ++i){
					_delay_25ms();
				}
				PORTB = 0;
				for(i = 0; i < 20; ++i){
					_delay_25ms();
				}
			}
			
			//wait before code repeat
			for(i = 0; i < 20; ++i){
				_delay_25ms();
			}
		}
	}
	else//run time error
	{
		flashes = error_msg + 1 - ERR_RUN_1_USER_CALLED_OS_ABORT; // runtime error number
		
		for(;;){
			
			//flash on for 3 second on, off for 1 seconds
			flashes = error_msg + 1;
			PORTB = ERROR_LED;
			for(;;){
				for(i = 0; i < 120; ++i){
					_delay_25ms();
				}
			}
			PORTB = (uint8_t) 0;
			for(;;){
				for(i = 0; i < 40; ++i){
					_delay_25ms();
				}
			}
			
			//flash run time error number quarter second on. quarter off
			for(j = 0; j < flashes; ++j){
				PORTB = ERROR_LED;
				
				for(i = 0; i < 10; ++i){
					_delay_25ms();
				}
				
				PORTB = (uint8_t) 0;
				
				for(i = 0; i < 10; ++i){
					_delay_25ms();
				}
			}
			//wait before code repeat
			for(i = 0; i < 20; ++i){
				_delay_25ms();
			}
		}
	}
}


/**
* @param f  a parameterless function to be created as a process instance
* @param arg an integer argument to be assigned to this process instanace
* @param level assigned scheduling level: SYSTEM, PERIODIC or RR
* @param name assigned PERIODIC process name
* @return 0 if not successful; otherwise non-zero.
* @sa Task_GetArg(), PPP[].
*
*  A new process  is created to execute the parameterless
*  function @a f with an initial parameter @a arg, which is retrieved
*  by a call to Task_GetArg().  If a new process cannot be
*  created, 0 is returned; otherwise, it returns non-zero.
*  The created process will belong to its scheduling @a level.
*  If the process is PERIODIC, then its @a name is a user-specified name
*  to be used in the PPP[] array. Otherwise, @a name is ignored.
* @sa @ref policy
*/
int Task_Create(void (*f)(void), int arg, unsigned int level, unsigned int name)
{
	int retval;
	uint8_t sreg;
	
	sreg = SREG;
	Disable_Interrupt();
	
	kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = (uint8_t)level;
	kernel_request_create_args.name = (uint8_t)name;
	
	kernel_request = TASK_CREATE;
	enter_kernel();
	
	retval = kernel_request_retval;
	SREG = sreg;
	
	return retval;
}



/**
* @brief The calling task gives up its share of the processor voluntarily.
*/
void Task_Next()
{
	uint8_t volatile sreg;
	
	sreg = SREG;
	Disable_Interrupt();
	
	kernel_request = TASK_NEXT;
	enter_kernel();
	
	SREG = sreg;
}


/**
* @brief The calling task terminates itself.
*/
void Task_Terminate()
{
	uint8_t sreg;
	
	sreg = SREG;
	Disable_Interrupt();
	
	kernel_request = TASK_TERMINATE;
	enter_kernel();
	
	SREG = sreg;
}


/** @brief Retrieve the assigned parameter.
*/
int Task_GetArg(void)
{
	int arg;
	uint8_t sreg;
	
	sreg = SREG;
	Disable_Interrupt();
	
	arg = cur_task->arg;
	
	SREG = sreg;
	
	return arg;
}

uint16_t Now() {
	//Disable interrupts before calcs
	uint16_t now_time;
	uint8_t sreg = SREG;
	Disable_Interrupt();
	
	now_time = running_time * TICK + ((TCNT1 - timer_time)/(F_CPU/TIMER_PRESCALER/1000)); 
	
	SREG = sreg;
	
	return now_time;
}

SERVICE* Service_Init() {
	//make sure we don't have too many services going
	if(service_cntr >= MAXSERVICE) {
		error_msg = ERR_RUN_1_USER_CALLED_OS_ABORT; //create new error message for this
		OS_Abort();
	}

	services[service_cntr].value = 0;
	services[service_cntr].waiting = 0;
	services[service_cntr].task_list.head = NULL;
	services[service_cntr].task_list.tail = NULL;

	return &(services[service_cntr++]);
}

void Service_Subscribe( SERVICE *s, int16_t *v ) {
	//check conditions for subscription
	if(service_cntr >= MAXSERVICE || cur_task->level == PERIODIC)
	abort(ERR_RUN_1_USER_CALLED_OS_ABORT);

	//set task to wait state and add it to publish queue
	cur_task->state = WAITING;
	cur_task->value = v;
	enqueue(&(s->task_list), cur_task);
	s->waiting++;
	Task_Next();
}

void abort(int msg) {
	//TODO: need to update error message codes
	//TODO: add tracing
	error_msg = msg;
	OS_Abort();
}

//this is setup just like the task_terminate
static void task_interrupt() {
	uint8_t sreg;
	sreg = SREG;
	Disable_Interrupt();
	kernel_request = TASK_INTERRUPT;
	enter_kernel();
	SREG = sreg;
}

void Service_Publish( SERVICE *s, int16_t v ) {
	task_descriptor_t *task = NULL;

	// We don't have any waiting services
	if(service_cntr == 0 || s == NULL || s->waiting <= 0) {
		abort(ERR_RUN_1_USER_CALLED_OS_ABORT);
	}
	
	//Since all tasks for this service have the memory address and not the v value,
	//all we need to do is update the service value
	s->value = v;
	
	// Wake up all tasks waiting on the service
	while((task = dequeue(&(s->task_list))) != NULL) {
		if(task->state == WAITING) {
			if(task->level == SYSTEM) {
				
				task->state = READY;
				s->waiting--;

				enqueue(&system_queue, task);
				
				//if current task it interruptable, go for it
				if(cur_task->level != SYSTEM){
					task_interrupt();
				}
			}
			else if(task->level == RR) {
				task->state = READY;
				s->waiting--;
				
				enqueue(&rr_queue, task);
			}
		}
	}

}

/**
* \param f  a parameterless function to be created as a process instance
* \param arg an integer argument to be assigned to this process instanace
* \return 0 if not successful; otherwise non-zero.
* \sa Task_GetArg()
*
*  A new process is created to execute the parameterless
*  function \a f with an initial parameter \a arg, which is retrieved
*  by a call to Task_GetArg().  If a new process cannot be
*  created, 0 is returned; otherwise, it returns non-zero.
*
* \sa \ref policy
*/
int8_t Task_Create_System(void (*f)(void), int16_t arg) {
	int retval;
	uint8_t sreg;

	sreg = SREG;
	Disable_Interrupt();

	kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = SYSTEM;

	kernel_request = TASK_CREATE;
	enter_kernel();

	retval = kernel_request_retval;
	SREG = sreg;

	return retval;
}


int8_t Task_Create_Periodic(void(*f)(void), int16_t arg, uint16_t period, uint16_t wcet, uint16_t start) {
	int retval;
	uint8_t sreg;

	sreg = SREG;
	Disable_Interrupt();

	kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = (uint8_t)PERIODIC;
	kernel_request_create_args.period = period;
	kernel_request_create_args.wcet = wcet;
	kernel_request_create_args.start = start;

	kernel_request = TASK_CREATE;
	enter_kernel();

	retval = kernel_request_retval;
	SREG = sreg;

	return retval;
}

/**
* \param f  a parameterless function to be created as a process instance
* \param arg an integer argument to be assigned to this process instanace
* \return 0 if not successful; otherwise non-zero.
* \sa Task_GetArg()
*
*  A new process is created to execute the parameterless
*  function \a f with an initial parameter \a arg, which is retrieved
*  by a call to Task_GetArg().  If a new process cannot be
*  created, 0 is returned; otherwise, it returns non-zero.
*
* \sa \ref policy
*/
int8_t   Task_Create_RR(void (*f)(void), int16_t arg){
	int retval;
	uint8_t sreg;
	
	sreg = SREG;
	Disable_Interrupt();
	
	kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = RR;
	
	kernel_request = TASK_CREATE;
	enter_kernel();
	
	retval = kernel_request_retval;
	SREG = sreg;
	
	return retval;
}


void sys(){
	PORTB = (uint8_t)(_BV(PB5));
	PORTB = 0;
	Task_Next();
}

void rr(){
	for(;;){
		for(int i = 0; i < 100; i++){
		if(PORTB == 0){
			PORTB = (uint8_t)(_BV(PB6));
		}else{
			PORTB = 0;
		}
		_delay_ms(10);
		}
		Task_Create_System(sys, 0);
	}
}

void per(){
	if( (PORTB & (uint8_t)(_BV(PB7)))  == 0){
		PORTB = (uint8_t)(_BV(PB7));
	}else{
		PORTB = 0;
	}
	Task_Next();
}

/**
* Runtime entry point into the program; just start the RTOS.  The application layer must define r_main() for its entry point.
*/
int main()
{
	DDRB = (uint8_t)(_BV(PB7) | _BV(PB6) | _BV(PB5));
	PORTB = 0;
	for(int i = 0; i < 100; i++){
		_delay_25ms();
	}
	OS_Init();
	return 0;
}



int r_main(){

	Task_Create_RR(rr, 0);
	Task_Create_Periodic(per,0,103,100,140);
	return 0;
}