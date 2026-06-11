/**
*
*	@file ssd1306.h
*
*	@brief Public API for the ssd1306 module.
*
*	@details Module to print 8x8 pixels characters on the display
*	integrating the 2-wire interface for transmission.
*
*	Resets the value of pixels, writes new ones and turns on the display.
*
*/

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SSD1306_SLA_ADD (0x3C << 1)

typedef enum {
	SSD_ERR_OK = 0,
	SSD_ERR_PARAM,
	SSD_ERR_TWI
} ssd_errors;

/**
*
*	@brief	Configures the display with the Horizontal Addressing Mode and clears all pixels.
*
*	@retval	SSD_ERR_OK				If no error occurs
*	@retval	SSD_ERR_TWI				If an error with the 2-wire interface occurs
*
*   @post   If an error with the 2-wire interface occurs the correct execution is not guaranteed
*	@post 	On success the value of all pixels is 0
*	@post 	On success the transmission is stopped and the display is left configured
*
*/
ssd_errors ssd1306_data_reset(void);

/**
*
*	@brief	Turns on the display.
*
*	@retval	SSD_ERR_OK				If no error occurs
*	@retval	SSD_ERR_TWI				If an error with the 2-wire interface occurs
*
*   @post   If an error with the 2-wire interface occurs the correct execution is not guaranteed
*	@post 	On success the display turns on and prints all pixels having value set to 1
*	@post 	On success the transmission is stopped and the display is left configured
*
*/
ssd_errors ssd1306_display_on(void);

/**
*
*	@brief	Writes an 8x8 pixels character at a specific position of a 8x16 grid.
*
*	@param	char_bytes				Pointer to the first element of the character array
*	@param	row						Index of the row of a 8x16 grid
*	@param	column					Index of the column of a 8x16 grid
*
*	@retval	SSD_ERR_OK				If no error occurs
*	@retval	SSD_ERR_PARAM			If char_bytes == NULL || !(1 <= row <= 8) || !(1 <= column <= 16)
*	@retval	SSD_ERR_TWI				If an error with the 2-wire interface occurs
*
*   @pre   	char_bytes points to the first element of an array containing the character in 8 bytes
*   @pre   	1 <= row <= 8
*   @pre   	1 <= column <= 16
*   @post   If passed parameters are invalid the transmission is not started
*   @post   If an error with the 2-wire interface occurs the correct execution is not guaranteed
*	@post	ssd1306_data_reset is called if it has never been called
*	@post 	On success the value of the desired pixels is set to 1
*	@post 	On success the transmission is stopped and the display is left configured
*
*/
ssd_errors ssd1306_8x8char_write(const uint8_t* char_bytes, uint8_t row, uint8_t column);

#endif
