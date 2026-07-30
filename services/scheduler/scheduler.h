/**
*
*	@file scheduler.h
*
*	@brief Public API for the scheduler module.
*
*	@details Module to implement a scheduler using the ATmega328P
*	8-bit timer 0.
*
*	Can be used for both delays creation and event polling.
*
*/

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

/**
*
*	@brief	Configures and starts 8-bit timer 0 to 1 ms.
*
*	@post 	If 8-bit timer 0 has never been configured it is configured to 1 ms
*	@post 	The timer is started
*	@post 	If the timer was already running it is left unchanged
*
*/
void scheduler_timer_start(void);

/**
*
*	@brief	Stops the timer if it is running and resets it.
*
*	@post 	If the timer was running it is stopped
*	@post 	The timer is reset
*
*/
void scheduler_timer_stop(void);

/**
*
*	@brief	Returns the timestamp of the current timer value.
*
*	@retval	0						If scheduler_timer_start has never been called
*
*	@post 	The current timer value is returned
*
*/
uint16_t scheduler_timer_get_timestamp(void);

/**
*
*	@brief	Polls the timer to check if it reached the target and updates the starting time.
*
*	@param	target_duration_ms	    Target duration of the timer
*	@param	t_zero_ms			    Starting time
*
*	@retval	0					    If the timer has not yet reached the target duration or if t_zero_ms == NULL || !target_duration_ms
*	@retval	0					    If scheduler_timer_start has never been called
*	@retval	1					    If the timer has reached the target duration
*
*	@pre    This function is being polled at a frequency f > 16 mHz
*	@post 	If the timer has reached the target duration *t_zero_ms is updated to the current timestamp
*
*/
uint8_t scheduler_timer_poll(uint16_t* t_zero_ms, uint16_t target_duration_ms);


/**
*
*	@brief	Creates a delay of a given duration.
*
*	@param	target_duration_ms		Target duration of the delay
*
*	@post 	If scheduler_timer_start has never been called the delay will not be created
*	@post 	On success the CPU idles for target_duration_ms
*
*/
void scheduler_timer_delay(uint16_t target_duration_ms);

#endif
