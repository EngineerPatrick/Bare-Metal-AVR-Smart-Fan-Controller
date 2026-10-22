/**
*
*	@file fan_driver.h
*
*	@brief Public API for the fan_driver module.
*
*	@details Module to configure and update the fan controller.
*
*	Implements a feedback loop with a finite number of steps integrating the fan tachometer.
*
*/

#ifndef FAN_DRIVER_H
#define FAN_DRIVER_H

#include <stdint.h>

typedef enum {
	FAN_DRIVER_ERR_OK = 0,
	FAN_DRIVER_ERR_TACH,
	FAN_DRIVER_ERR_PARAM,
	FAN_DRIVER_ERR_INTERNAL
} fan_driver_errors;

/**
*
*	@brief	Boots the fan controller.
*
*	@pre	The global interrupt is activated by calling sei
*	@post	If sei has not been called, or if cli has been called afterwards, then the correct execution is not guaranteed
*	@post	If the fan controller has already been booted by calling this, fan_driver_speed_measure or fan_driver_controller_update, then it is left unchanged
*	@post	On success the fan controller is booted and the PWM duty-cycle is set to 50%
*
*/
void fan_driver_controller_boot(void);

/**
*
*	@brief	Measures the speed of the fan integrating the tachometer and saves it.
*
*	@param	measured_speed_rpm[out]				Measured speed
*
*	@retval	FAN_DRIVER_ERR_OK					If no error occurs
*	@retval	FAN_DRIVER_ERR_TACH					If there is no tachometer reading before the last call
*	@retval	FAN_DRIVER_ERR_PARAM				If measured_speed_rpm == NULL
*	@retval	FAN_DRIVER_ERR_PARAM				If an internal error occurs which results in a division by 0 or in a variable overflow
*
*	@pre	The global interrupt is activated by calling sei
*	@post	If sei has not been called, or if cli has been called afterwards, then the correct execution is not guaranteed
*	@post	If the passed parameter is invalid then the speed is not measured
*	@post	If fan_driver_controller_boot has not been called, or if fan_driver_controller_stop has been called afterwards, then fan_driver_controller_boot is called and this function returns without saving the speed
*	@post	If there is no tachometer reading before the last call, or if the measured speed is lower than the minimum one, then the speed is not saved
*	@post	If an internal error occurs then the speed is not measured
*	@post	On success the speed is measured, clamped within the valid range and saved
*
*	@note	This function returns immediately if it needs to call fan_drvier_controller_boot to allow delaying the check for missed tachometer signal
*
*/
fan_driver_errors fan_driver_speed_measure(uint16_t* measured_speed_rpm);

/**
*
*	@brief	Updates the PWM duty-cycle with a feedback controller.
*
*	@param	target_speed_rpm[in]				Target speed
*
*	@pre	The scheduler is booted by calling scheduler_timer_boot
*	@post	If scheduler_timer_boot has not been called, or if scheduler_timer_stop has been called afterwards, then the correct execution is not guaranteed
*	@post	If fan_driver_controller_boot has not been called, or if fan_driver_controller_stop has been called afterwards, then fan_driver_controller_boot is called and this function returns without saving the speed
*	@post	On success the target speed is clamped within the valid range
*	@post	On success a speed hysteresis value is estimated assuming a linear model
*	@post	On success if the new target speed is outside of the hysteresis range then the fan controller target speed is updated
*	@post	On success of the first call the duty-cycle is set to an estimated value to match the target speed assuming a linear model
*	@post	On success of the next calls the feedback controller waits for the speed to stabilize within a range of 5 RPM for a minimum time
*	@post	On success of the next calls the feedback controller uses the stable speed value to update the duty-cycle to match the speed to the the target speed
*	@post	On success of the next calls the feedback controller stops updating the duty-cycle when the measured stable speed is as close as possible to the target speed
*
*	@note	This function returns immediately if it needs to call fan_drvier_controller_boot to allow delaying the check for missed tachometer signal
*
*/
void fan_driver_controller_update(uint16_t target_speed_rpm);

/**
*
*	@brief	Stops and resets the fan controller.
*
*	@post	If fan_driver_controller_boot has been called and the fan controller is running then it is stopped and reset
*	@post	If fan_driver_controller_boot has not been called, or if fan_driver_controller_stop has been called afterwards, then the fan controller is left unchanged
*	@post	The PWM output pin is set as output with a value of 0 to forbid the fan to spin
*
*/
void fan_driver_controller_stop(void);

/**
*
*	@brief	Test the speed of the fan for a specific value of the duty-cycle register
*
*	@pre	The global interrupt is activated by calling sei
*	@post	If sei has not been called, or if cli has been called afterwards, then the correct execution is not guaranteed
*	@post	If there is no tachometer reading before the last call, or if the measured speed is lower than the minimum one, then the speed is not saved
*	@post	If an internal error occurs the speed is not measured
*	@post	On success the speed is measured, clamped within the valid range and returned
*
*/
//uint16_t fan_driver_speed_test(void);

/**
*
*	@brief	Test the delay to update the speed from two specific values of the duty-cycle register
*
*	@pre	The scheduler is booted and the global interrupt is activated by calling scheduler_timer_boot and sei
*	@post	If scheduler_timer_boot or sei have not been called, or if schedulert_timer_stop or cli have been called afterwards, then the correct execution is not guaranteed
*	@post	If there is no tachometer reading before the last call, or if the measured speed is lower than the minimum one, then the speed is not saved
*	@post	If an internal error occurs the speed is not measured
*	@post	On success the delay is measured and returned
*
*/
//uint16_t fan_driver_update_delay_test(void);

#endif
