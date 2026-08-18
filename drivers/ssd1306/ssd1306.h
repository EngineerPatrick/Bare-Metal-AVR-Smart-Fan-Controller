/**
*
*	@file ssd1306.h
*
*	@brief Public API for the ssd1306 module.
*
*	@details Module to show 8x8-pixel patterns on a 128x64 screen
*	integrating the 2-wire interface for transmission.
*
*	Resets the value of pixels, writes new ones and turns on the display.
*
*/

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>

#define SSD1306_SLA_ADD (0x3C << 1)

typedef enum {
	SSD_ERR_OK = 0,
	SSD_ERR_PARAM,
	SSD_ERR_TWI
} ssd_errors;

/**
*
*	@brief	Turns on the screen.
*
*	@retval	SSD_ERR_OK					If no error occurs
*	@retval	SSD_ERR_TWI					If an error with the twi driver occurs
*
*	@post   If an error with the twi driver occurs the correct execution is not guaranteed
*	@post 	On success the screen turns on and shows all the pixels with value of 1
*	@post 	On success the transmission is stopped and the screen is left on
*
*/
ssd_errors ssd1306_screen_turn_on(void);

/**
*
*	@brief	Writes an 8x8-pixel pattern at a specific position of an 8x16 grid.
*
*	@param	pattern_bytes					Pointer to the first element of the pattern array
*	@param	row						Index of the row of an 8x16 grid
*	@param	column						Index of the column of an 8x16 grid
*
*	@retval	SSD_ERR_OK					If no error occurs
*	@retval	SSD_ERR_PARAM					If pattern_bytes == NULL || !(1 <= row <= 8) || !(1 <= column <= 16)
*	@retval	SSD_ERR_TWI					If an error with the twi driver occurs
*
*	@pre   	pattern_bytes points to the first element of an array of size 8 containing the 8-byte pattern
*	@pre   	1 <= row <= 8
*	@pre   	1 <= column <= 16
*	@post   If the buffer array has a size smaller than 8 the correct execution is not guaranteed
*	@post   If passed parameters are invalid the transmission is not started
*	@post   If an error with the twi driver occurs the correct execution is not guaranteed
*	@post 	On success the value of the desired pixels is set to 1
*	@post 	On success the transmission is stopped and the display is left configured in Horizontal Addressing Mode
*
*/
ssd_errors ssd1306_data_write(const uint8_t* pattern_bytes, size_t row, size_t column);

/**
*
*	@brief	Clears all pixels.
*
*	@retval	SSD_ERR_OK					If no error occurs
*	@retval	SSD_ERR_TWI					If an error with the twi driver occurs
*
*	@post   If an error with the twi driver occurs the correct execution is not guaranteed
*	@post 	On success the value of all pixels is changed to 0
*	@post 	On success the transmission is stopped and the display is left configured in Horizontal Addressing Mode
*
*/
ssd_errors ssd1306_data_reset(void);

#endif
