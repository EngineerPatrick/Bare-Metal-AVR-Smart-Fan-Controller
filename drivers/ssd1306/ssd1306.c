/**
*   
*   @file ssd1306.c
*
*   @brief Implementation for the ssd1306 module.
*
*   @details Contains the configuration commands and 
*	the 8x8 pixels character logic.
*
*   Handles the transmission using the 2-wire interface.
*
*/

#include <stdint.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include "ssd1306.h"
#include "twi.h"
#include "config.h"

#define DATA_BYTE 0x40
#define COMMAND_BYTE 0x80
#define MEMORY_ADDRESSING_MODE 0x20
#define HORIZONTAL_ADDRESSING_MODE 0X00
#define COLUMN_START_END 0x21
#define PAGE_START_END 0x22
#define CHARGE_PUMP_SETTING 0x8D
#define CHARGE_PUMP_ENABLE 0x14
#define DISPLAY_ON 0xAF

#define CONFIG_LENGTH 16
#define TURNON_LENGTH 6

static const uint8_t CONFIG[CONFIG_LENGTH] PROGMEM = {COMMAND_BYTE, MEMORY_ADDRESSING_MODE, COMMAND_BYTE, HORIZONTAL_ADDRESSING_MODE, COMMAND_BYTE, COLUMN_START_END, COMMAND_BYTE, 0x00, COMMAND_BYTE, 0x00, COMMAND_BYTE, PAGE_START_END, COMMAND_BYTE, 0x00, COMMAND_BYTE, 0x00};

static const uint8_t TURNON[TURNON_LENGTH] PROGMEM = {COMMAND_BYTE, CHARGE_PUMP_SETTING, COMMAND_BYTE, CHARGE_PUMP_ENABLE, COMMAND_BYTE, DISPLAY_ON};

static uint8_t init_flag = 0;

ssd_errors ssd1306_data_reset(void) {
	twi_errors error_code = 0;
	uint8_t commands[CONFIG_LENGTH];
	uint8_t data[9] = {DATA_BYTE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	for (uint8_t i = 0; i < CONFIG_LENGTH; i++) {
		commands[i] = pgm_read_byte(CONFIG + i);
	}
	
	for (uint8_t i = 0; i < 8; i++) {
		commands[13] = i;
		commands[15] = i;
			
		for (uint8_t j = 0; j < 16; j++) {
			commands[7] = 8 * j;
			commands[9] = 7 + 8 * j;			
			error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, CONFIG_LENGTH, SSD1306_SLA_ADD, commands);
			
			if (error_code != TWI_ERR_OK) {
				return SSD_ERR_TWI;
			}
			
			init_flag = 1;
			
			error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 9, SSD1306_SLA_ADD, data);
			
			if (error_code != TWI_ERR_OK) {
				return SSD_ERR_TWI;
			}
		}
	}
	
	return SSD_ERR_OK;
}

ssd_errors ssd1306_display_on(void) {
	twi_errors error_code = 0;
	uint8_t commands[TURNON_LENGTH];
	
	for (uint8_t i = 0; i < TURNON_LENGTH; i++) {
		commands[i] = pgm_read_byte(TURNON + i);
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, TURNON_LENGTH, SSD1306_SLA_ADD, commands);
			
	if (error_code != TWI_ERR_OK) {
		return SSD_ERR_TWI;
	}
	
	return SSD_ERR_OK;
}

ssd_errors ssd1306_8x8char_write(const uint8_t* char_bytes, uint8_t row, uint8_t column) {
	twi_errors error_code = 0;
	ssd_errors ssd_error_code = 0;
	uint8_t commands[CONFIG_LENGTH - 4];
	uint8_t data[9];
	
	if (char_bytes == NULL || (row > 8 || row == 0) || (column > 16 || column == 0)) {
		return SSD_ERR_PARAM;
	}
	
	if (!init_flag) {
		ssd_error_code = ssd1306_data_reset();
		
		if (ssd_error_code != SSD_ERR_OK) {
			return ssd_error_code;
		}
	}
	
	for (uint8_t i = 0; i < CONFIG_LENGTH - 4; i++) {
		commands[i] = pgm_read_byte(CONFIG + 4 + i);
	}
	
	data[0] = DATA_BYTE;
	
	for (uint8_t i = 0; i < 8; i++) {
		data[i + 1] = *(char_bytes + i);
	}
	
	commands[3] = 8 * (column - 1);
	commands[5] = 7 + (8 * (column - 1));
	commands[9] = (row - 1);
	commands[11] = (row - 1);
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, CONFIG_LENGTH - 4, SSD1306_SLA_ADD, commands);
			
	if (error_code != TWI_ERR_OK) {
		return SSD_ERR_TWI;
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 9, SSD1306_SLA_ADD, data);
			
	if (error_code != TWI_ERR_OK) {
		return SSD_ERR_TWI;
	}
	
	return SSD_ERR_OK;
}
