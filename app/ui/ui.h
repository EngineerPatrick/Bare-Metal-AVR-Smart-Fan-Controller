/**
*
*	@file ui.h
*
*	@brief Public API for the ui module.
*
*	@details Module to manage the UI using a standard and
*	an advanced mode to create a fan curve.
*
*	The CONFIRM button saves the setting, the SELECT button changes the option
*	and the RATE button changes the digit or toggles the curve creation.
*
*	A rotary encoder is implemented to handle the change of numbers.
*
*/

#ifndef UI_H
#define UI_H

typedef enum {
	UI_ERR_OK = 0,
	UI_ERR_DISPLAY,
	UI_ERR_FAN_CURVE,
	UI_ERR_BME,
	UI_ERR_FAN_DRIVER
} ui_errors;

/**
*
*	@brief	Starts the UI to create a new fan curve and to save it in EEPROM.
*
*	@retval	UI_ERR_OK					If no error occurs
*	@retval	UI_ERR_FAN_CURVE				If an error with the fan_curve service occurs
*	@retval	UI_ERR_DISPLAY					If an error with the display service occurs
*
*	@post 	If an error with the fan_curve service or the display service occurs the correct execution is not guaranteed
*	@post 	On success the new curve is created and saved in EEPROM
*
*/
ui_errors ui_system_config(void);

/**
*
*	@brief	Handles the system runtime loop.
*
*	@retval	UI_ERR_OK					If no error occurs
*	@retval	UI_ERR_BME					If an error with the bme280 driver occurs
*	@retval	UI_ERR_DISPLAY					If an error with the display service occurs
*	@retval	UI_ERR_FAN_CURVE 				If an error with the fan_curve service occurs
*	@retval	UI_ERR_FAN_DRIVER				If an error with the fan driver occurs
*
*	@pre 	The scheduler must be activated by calling scheduler_timer_boot
*	@post 	If scheduler_timer_boot has not been called the correct execution is not guaranteed
*	@post 	If an error with the bme280 driver, the display service, the fan_curve service or the fan driver occurs the correct execution is not guaranteed
*	@post 	On success the PWM is booted at 50% duty cycle for an initial delay to overcome the inertia before checking for missed tachometer readings
*	@post 	On success the BME280 sensor performs temperature live readings
*	@post 	On success the target speed is calculated from the fan curve saved in EEPROM
*	@post 	On success the PWM duty-cycle is updated to match the fan speed with the closest possible value of the new target speed
*	@post 	On success the SSD1306 display shows the real-time measured values
*	@post 	If the function returns the fan and the BME280 are stopped and reset
*
*/
ui_errors ui_system_runtime_loop(void);

#endif
