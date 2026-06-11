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
*	@retval	UI_ERR_FAN_CURVE		If the curve is not correctly saved in the EEPROM
*	@retval	UI_ERR_DISPLAY			If an error with the SSD1306 display occurs
*
*	@post 	If an error with the EEPROM or the SSD1306 display occurs the correct execution is not guaranteed
*	@post 	On success the new curve is created and saved in the EEPROM
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
*	@retval	UI_ERR_FAN_CURVE		If the curve is not correctly saved in EEPROM
*	@retval	UI_ERR_FAN_DRIVER		If an error with the PWM occurs
*
*	@post 	If an error with the BME280, the SSD1306 display, the EEPROM or the PWM occurs the correct execution is not guaranteed
*	@post 	On success the new temperature is measured and printed on the display
*	@post 	On success the new target speed is calculated from the fan curve and printed on display
*	@post 	On success the PWM duty-cycle is updated to match the new target speed
*
*/
ui_errors ui_system_update(void);

#endif
