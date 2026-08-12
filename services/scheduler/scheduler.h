/**
*
*	@file scheduler.h
*
*	@brief Public API for the scheduler module.
*
*	@details Module for a system scheduler using the ATmega328P Timer0.
*
*	Can be used to poll-check if a specific amount of time has passed or
*	to create a blocking delay of a specific duration.
*
*/

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

/**
*
*	@brief	Configures and starts the timer to 1 ms tick.
*
*	@pre 	The the global interrupt must be activated by calling sei
*	@post 	If sei has not been called the correct execution is not guaranteed
*	@post 	If the timer was already booted it is left unchanged
*	@post 	On success Timer0 is configured in CTC mode to generate a 1 ms tick
*
*/
void scheduler_timer_boot(void);

/**
*
*	@brief	Stops the timer and resets it.
*
*	@post 	If the timer was running it is stopped and reset
*	@post 	If the timer was not booted it is left unchanged
*
*/
void scheduler_timer_stop(void);

/**
*
*	@brief	Returns the timestamp of the current timer value.
*
*	@retval	0								If the timer has not been booted
*
*	@post 	The current timer timestamp is returned
*
*/
uint16_t scheduler_timestamp_capture(void);

/**
*
*	@brief	Polls the timer to check if it reached the target duration.
*
*	@param	target_duration_ms	 		   Target duration
*	@param	t_0_ms						   Starting timestamp
*
*	@retval	0					 		   If the timer has not been booted
*	@retval	0					 		   If the timer has not reached the target duration or if !target_duration_ms
*	@retval	1					 		   If the timer has reached the target duration
*
*	@pre 	The scheduler and the global interrupt must be activated by calling scheduler_timer_boot and sei
*	@pre    This function is being polled at a frequency higher than 16 mHz
*	@post 	If scheduler_timer_boot and sei have not been called the correct execution is not guaranteed
*	@post 	If this function is being polled at a frequency lower than 16 mHz the correct execution is not guaranteed
*	@post	On success 1 is returned if the timer has reached or surpassed the target duration
*
*/
uint8_t scheduler_timer_elapsed(uint16_t t_0_ms, uint16_t target_duration_ms);


/**
*
*	@brief	Creates a blocking delay of a specific target duration.
*
*	@param	target_duration_ms	  		  Target duration
*
*	@pre 	The scheduler and the global interrupt must be activated by calling scheduler_timer_boot and sei
*	@post 	If sei has not been called the correct execution is not guaranteed
*	@post 	If scheduler_timer_boot has not been called the delay will not be created
*	@post 	On success the CPU undergoes a busy-wait for a time equal to the target duration
*
*/
void scheduler_timer_delay(uint16_t target_duration_ms);

#endif
