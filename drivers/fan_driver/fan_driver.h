/**
*
*	@file fan_driver.h
*
*	@brief Public API for the fan_driver module.
*
*	@details Module to configure and update the PWM to control
*	the speed of a fan.
*
*	Implements the fan tachometer to adjust the PWM to
*	the approximate the target speed.
*
*/

#ifndef FAN_DRIVER_H
#define FAN_DRIVER_H

#include <stdint.h>

typedef enum {
	FAN_DRIVER_ERR_OK = 0,
	FAN_DRIVER_ERR_TACH
} fan_driver_errors;

/*
*	
*	Approximate values for 9V DC supply calculated from the tachometer
*	pulses measured using the Input Capture feature
*
*	Duty-cycle from ~3.75% to ~100%
*
*/
#define FAN_DRIVER_MAX_SPEED_RPM 2200
#define FAN_DRIVER_MIN_SPEED_RPM 330

/**
*
*	@brief	Configures and starts the PWM to 25kHz.
*
*	@post 	On success the PWM is set to 25kHz and the duty-cycle is set to 3.75%
*
*/
void fan_driver_boot(void);

/**
*
*	@brief	Updates the PWM duty-cycle integrating the fan tachometer to adjust the speed.
*
*	@param	target_speed_rpm		Target speed value
*
*	@retval	FAN_DRIVER_ERR_OK		If no error occurs
*	@retval	FAN_DRIVER_ERR_TACH		If there was no tachometer reading for a period of FAN_DRIVER_TIMEOUT_MS
*
*	@post 	If fan_driver_boot has never been called it is called
*	@post 	If the tachometer reading times out the speed is not adjusted but is still an approximation
*	@post 	On success the PWM duty-cycle is updated to a value for the best possible approximation of target_speed_rpm
*
*/
fan_driver_errors fan_driver_update(uint16_t target_speed_rpm);

/**
*
*	@brief	Stops and resets the PWM and the fan tachometer readings.
*
*	@post 	If fan_driver_boot has been called the PWM and the tachometer readings are stopped and reset
*
*/
void fan_driver_stop(void);

#endif
