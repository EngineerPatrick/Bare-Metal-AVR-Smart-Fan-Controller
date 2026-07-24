/**
*
*	@file fan_driver.h
*
*	@brief Public API for the fan_driver module.
*
*	@details Module to configure and update the PWM to control
*	the fan speed.
*
*	Implements the fan tachometer feedback to adjust the PWM
*	in a finite number of steps.
*
*/

#ifndef FAN_DRIVER_H
#define FAN_DRIVER_H

#include <stdint.h>

typedef enum {
	FAN_DRIVER_ERR_OK = 0,
	FAN_DRIVER_ERR_TACH,
	FAN_DRIVER_ERR_CONFIG
} fan_driver_errors;

/**
*
*	@brief	Configures and starts the PWM to 25 kHz.
*
*	@post 	On success the PWM is set to 25 kHz and the duty-cycle is set to 50%
*
*/
void fan_driver_boot(void);

/**
*
*	@brief	Updates the PWM duty-cycle integrating the fan tachometer feedback and exposes the measured speed.
*
*	@param	target_speed_rpm 		Target speed value
*	@param	measured_speed_rpm 		Measured speed value
*
*	@retval	FAN_DRIVER_ERR_OK		If no error occurs
*	@retval	FAN_DRIVER_ERR_TACH		If there was no tachometer reading for a period of FAN_DRIVER_TIMEOUT_MS
*	@retval	FAN_DRIVER_ERR_CONFIG	If (FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM) < 80 || !(1 <= MIN_DC_REG_STEP <= 10) || !(0 <= MIN_DC_REG_VALUE <= 40)
*
*	@pre 	FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM >= 80
*	@pre 	1 <= MIN_DC_REG_STEP <= 10
*	@pre 	0 <= MIN_DC_REG_VALUE <= 40
*	@post 	If passed configuration parameters are invalid the speed is left unchanged
*	@post 	If fan_driver_boot has never been called it is called
*	@post 	If the tachometer reading times out the speed is not adjusted with the feedback but is still an estimated value
*	@post 	On success the PWM duty-cycle is updated to approximately match the fan speed to the target_speed_rpm
*	@post 	On success the measured speed value is saved in measured_speed_rpm
*
*/
fan_driver_errors fan_driver_update(uint16_t target_speed_rpm, uint16_t* measured_speed_rpm);

/**
*
*	@brief	Stops and resets the PWM and the fan tachometer readings.
*
*	@post 	If fan_driver_boot has been called the PWM and the tachometer readings are stopped and reset
*
*/
void fan_driver_stop(void);

#endif
