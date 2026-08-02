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
*	@brief	Starts the UI to create a new fan curve.
*
*	@retval	UI_ERR_OK				If no error occurs
*	@retval	UI_ERR_FAN_CURVE 		If the curve is not correctly saved in EEPROM or the standard curve fixed values are out of range
*	@retval	UI_ERR_DISPLAY			If an error with the SSD1306 display occurs
*
*	@post 	If the standard curve fixed values are out of range the fan curve saved in EEPROM is left unchanged
*	@post 	If an error with EEPROM or the SSD1306 display occurs the correct execution is not guaranteed
*	@post 	On success the new curve is created and saved in EEPROM
*
*/
ui_errors ui_system_configure(void);

/**
*
*	@brief	Updates the entire system with new readings.
*
*	@retval	UI_ERR_OK				If no error occurs
*	@retval	UI_ERR_BME				If an error with the BME280 sensor occurs
*	@retval	UI_ERR_DISPLAY			If an error with the SSD1306 display occurs
*	@retval	UI_ERR_FAN_CURVE 		If the curve is not correctly saved in EEPROM
*	@retval	UI_ERR_FAN_DRIVER		If an error with the PWM occurs
*
*	@post 	If an error with the BME280, the SSD1306 display, EEPROM or the PWM occurs the correct execution is not guaranteed
*	@post 	On success the PWM duty-cycle is set to 50% for FAN_DRIVER_BOOT_DELAY_MS to overcome inertia before using the tachometer hardware timeout
*	@post 	On success the BME280 sensor performs temperature live readings and the SSD1306 display shows them
*	@post 	On success the real-time target speed is calculated from the fan curve
*	@post 	On success the PWM duty-cycle is updated every FAN_DRIVER_UPDATE_TIME_MS to approximately match the fan speed with the new target speed
*	@post 	On success the display shows the real-time measured speed
*
*/
ui_errors ui_system_update(void);

#endif
