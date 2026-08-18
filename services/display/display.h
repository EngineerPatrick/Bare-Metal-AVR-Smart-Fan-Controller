/**
*
*	@file display.h
*
*	@brief Public API for the display module.
*
*	@details Module to configure a display to show 8x8-pixel characters and digits.
*
*	All the pattern bitmaps are saved in program memory to restrain RAM usage for MCU applications.
*
*/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
	DISPLAY_ERR_OK = 0,
	DISPLAY_ERR_PARAM,
	DISPLAY_ERR_SSD
} display_errors;

typedef enum {
	SYS_ERR_OK = 0,
	SYS_ERR_DISPLAY,
	SYS_ERR_FAN_CURVE,
	SYS_ERR_BME,
	SYS_ERR_FAN_DRIVER
} system_errors;

typedef enum {
	STANDARD_WORD = 20,
	ADVANCED_WORD,
	TEMP_FIRST_DIGIT,
	TEMP_SECOND_DIGIT,
	TEMP_THIRD_DIGIT,
	SPEED_FIRST_DIGIT,
	SPEED_SECOND_DIGIT,
	SPEED_THIRD_DIGIT
} blink_options;
	

/**
*
*	@brief	Resets the display, writes the introduction and turns it on.
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the words "SELECT MODE" at the selected position of an 8x16 grid
*
*/
display_errors display_intro_write(void);

/**
*
*	@brief	Writes the standard option word on the display.
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the word "STANDARD" at the selected position of an 8x16 grid
*
*/
display_errors display_standard_write(void);

/**
*
*	@brief	Writes the advanced option word on the display.
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the word "ADVANCED" at the selected position of an 8x16 grid
*
*/
display_errors display_advanced_write(void);

/**
*
*	@brief	Writes the index of the current fan curve node on the display.
*
*	@param	inde						Index of the current fan curve node
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_PARAM				If !(0 < index <= 10)
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@pre 	0 < index <= 10
*	@post   If passed parameter is invalid the transmission is not started
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the index as "NODE XX" at the selected position of an 8x16 grid
*
*/
display_errors display_index_write(size_t index);

/**
*
*	@brief	Resets the display and writes the units of temperature and speed.
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the words "TEMP      . °C" at the selected position of an 8x16 grid
*	@post 	On success the display shows the words "SPEED      RPM" at the selected position of an 8x16 grid
*
*/
display_errors display_units_write(void);

/**
*
*	@brief	Writes the temperature digits on the display.
*
*	@param	temp_c_x10					Temperature value of 3 digits with 1 decimal digit "12.3" = "123"
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_PARAM				If !(0 <= temp_c_x10 <= 999)
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@pre 	0 <= temp_c_x10 <= 999
*	@post   If passed parameter is invalid the transmission is not started
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the digits "XX X" at the selected position of an 8x16 grid
*
*	@note	The space between the last digit and the first two should be filled with a dot by calling display_units_write.
*
*/
display_errors display_temp_write(uint16_t temp_c_x10);

/**
*
*	@brief	Writes the speed digits on the display.
*
*	@param	speed_rpm					Speed value of 4 digits with no decimal digit
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_PARAM				If !(0 <= speed_rpm <= 9999)
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@pre 	0 <= speed_rpm <= 9999
*	@post   If passed parameter is invalid the transmission is not started
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the digits "XXXX" at the selected position of an 8x16 grid
*
*/
display_errors display_speed_write(uint16_t speed_rpm);

/**
*
*	@brief	Resets the display and writes the passed error code.
*
*	@param	error_code					Error code
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_PARAM				If error_code > SYS_ERR_FAN_DRIVER
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@pre 	error_code <= SYS_ERR_FAN_DRIVER
*	@post	If passed parameter is invalid the display is left unchanged
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the display shows the error code as "ERR X" in the selected position of an 8x16 grid
*
*/
display_errors display_error_write(system_errors error_code);

/**
*
*	@brief	Configures the blink parameters for the word STANDARD or ADVANCED.
*
*	@param	target_duration					Target timer
*	@param	option						Option to choose which word to blink
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_PARAM				If !target_duration || !(STANDARD_WORD <= option <= ADVANCED_WORD)
*
*	@pre   	target_duration > 0
*	@pre   	STANDARD_WORD <= option <= ADVANCED_WORD
*	@post   If passed parameters are invalid the blink parameters are left unchanged
*	@post 	On success the blink parameters are configured and display_blink_state_switch is required to be polled
*
*/
display_errors display_word_blink(uint16_t target_duration, blink_options option);

/**
*
*	@brief	Configures the blink parameters for a digit from 0 to 9.
*
*	@param	target_duration					Target timer
*	@param	option						Option to choose which digit to blink for temperature or speed
*	@param	full_value					Full value of temperature or speed from where to extract the digit to blink
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_PARAM				If !target_duration || !(TEMP_FIRST_DIGIT <= option <= SPEED_THIRD_DIGIT)
*	@retval	DISPLAY_ERR_PARAM				If option is a temperature digit and full_value is outside of valid boundaries
*	@retval	DISPLAY_ERR_PARAM				If option is a speed digit and full_value is outside of valid boundaries
*
*	@pre   	target_duration > 0
*	@pre   	TEMP_FIRST_DIGIT <= option <= SPEED_THIRD_DIGIT
*	@pre   	full_value is within valid boundaries
*	@post   If passed parameters are invalid the blink parameters are left unchanged
*	@post 	On success the blink parameters are configured and display_blink_state_switch is required to be polled
*
*/
display_errors display_digit_blink(uint16_t target_duration, blink_options option, uint16_t full_value);

/**
*
*	@brief	Switches on or off the pixels to blink.
*
*	@retval	DISPLAY_ERR_OK					If no error occurs
*	@retval	DISPLAY_ERR_PARAM				If display_word_blink or display_digit_blink have never been called
*	@retval	DISPLAY_ERR_SSD					If an error with the ssd1306 driver occurs
*
*	@pre   	display_word_blink or display_digit_blink has been called to configure and start the timer
*	@pre    This function is being polled at a frequency higher than 16 mHz
*	@pre 	The scheduler must be activated by calling scheduler_timer_boot
*	@post 	If scheduler_timer_boot has not been called the correct execution is not guaranteed
*	@post   If this function is being polled at a frequency lower than 16 mHz the correct execution is not guaranteed
*	@post   If display_word_blink and display_digit_blink have never been called the display is left unchanged
*	@post   If an error with the ssd1306 driver occurs the correct execution is not guaranteed
*	@post 	On success the selected pixels on the display switch on or off
*
*/
display_errors display_blink_state_switch(void);

#endif
