/**
 *	@file util.h
 *
 *  @date 	Created on: 25-May-2009
 *  @author	Heng Lien
 *
 *  @brief	This file contains useful bitwise operation tools
 *
 */

#ifndef _UTIL_H_
#define _UTIL_H_


/**
 * Macro for clearing the pre-scaler and make clock go 8MHz
 */
#define CLOCK_8MHZ() 	CLKPR = _BV(CLKPCE); CLKPR = 0x00;

/**
 * Macro for getting the low byte of a 2 byte variable
 */
#define LOWBYTE(v)   ((unsigned char) (v))

/**
 * Macro getting the high byte of a 2 byte variable
 */
#define HIGHBYTE(v)  ((unsigned char) (((unsigned int) (v)) >> 8))

/**
 * Set bit macro
 */
#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))

/**
 * Set bit macro
 */
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))

/**
 * Macro for testing a bit in the register
 */
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))



#endif /* UTIL_H_ */
