/**
*   
*   @file ssd1306.c
*
*   @brief Implementation for the ssd1306 module.
*
*   @details Contains the configuration commands and 
*	the logic to write 8x8-pixel patterns.
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

#define DATA_BYTE (1 << 6)
#define COMMAND_BYTE (1 << 7)

#define MEMORY_ADDRESSING_MODE 0x20
#define HORIZONTAL_ADDRESSING_MODE 0

#define COLUMN_START_END 0x21
#define PAGE_START_END 0x22

#define CHARGE_PUMP_SETTING 0x8D
#define CHARGE_PUMP_ENABLE 0x14
#define DISPLAY_ON 0xAF

#define MODE_CONFIG_LENGTH 4U
#define POSITION_CONFIG_LENGTH 12U
#define TURNON_LENGTH 6U
#define BUFF_LENGTH 9U

#define DISPLAY_MAX_ROW 8U
#define DISPLAY_MAX_COLUMN 16U

static const uint8_t MODE_CONFIG[MODE_CONFIG_LENGTH] PROGMEM = {COMMAND_BYTE, MEMORY_ADDRESSING_MODE, COMMAND_BYTE, HORIZONTAL_ADDRESSING_MODE};

static const uint8_t POSITION_CONFIG[POSITION_CONFIG_LENGTH] PROGMEM = {COMMAND_BYTE, COLUMN_START_END, COMMAND_BYTE, 0x00, COMMAND_BYTE, 0x00, COMMAND_BYTE, PAGE_START_END, COMMAND_BYTE, 0x00, COMMAND_BYTE, 0x00};

static const uint8_t TURNON[TURNON_LENGTH] PROGMEM = {COMMAND_BYTE, CHARGE_PUMP_SETTING, COMMAND_BYTE, CHARGE_PUMP_ENABLE, COMMAND_BYTE, DISPLAY_ON};

typedef struct {
	uint8_t init_flag;
	uint8_t turn_on_flag;
	uint8_t data_reset_flag;
} display_state;

typedef struct {
	display_state state;
	uint8_t pattern_buff[BUFF_LENGTH];
} display_controller;

static display_controller ssd1306_controller = {0};

static twi_errors mode_config(void) {
	twi_errors error_code = 0;
	uint8_t commands[MODE_CONFIG_LENGTH];
	
	for (size_t i = 0; i < MODE_CONFIG_LENGTH; i++) {
		commands[i] = pgm_read_byte(MODE_CONFIG + i);
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, MODE_CONFIG_LENGTH, SSD1306_SLA_ADD, commands);
	
	if (error_code != TWI_ERR_OK) {
		return error_code;
	}
			
	ssd1306_controller.state.init_flag = 1;
	return TWI_ERR_OK;
}

ssd_errors ssd1306_screen_turn_on(void) {
	twi_errors error_code = 0;
	uint8_t commands[TURNON_LENGTH];
	
	if (ssd1306_controller.state.turn_on_flag) {
		return SSD_ERR_OK;
	}
	
	for (size_t i = 0; i < TURNON_LENGTH; i++) {
		commands[i] = pgm_read_byte(TURNON + i);
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, TURNON_LENGTH, SSD1306_SLA_ADD, commands);
			
	if (error_code != TWI_ERR_OK) {
		return SSD_ERR_TWI;
	}
	
	ssd1306_controller.state.turn_on_flag = 1;
	return SSD_ERR_OK;
}

ssd_errors ssd1306_data_write(const uint8_t* pattern_bytes, size_t row, size_t column) {
	twi_errors error_code = 0;
	uint8_t commands[POSITION_CONFIG_LENGTH];
	
	if (pattern_bytes == NULL || row > DISPLAY_MAX_ROW || !row || column > DISPLAY_MAX_COLUMN || !column) {
		return SSD_ERR_PARAM;
	}
	
	if (!ssd1306_controller.state.init_flag) {
		error_code = mode_config();
		
		if (error_code != TWI_ERR_OK) {
			return SSD_ERR_TWI;
		}
	}
	
	for (size_t i = 0; i < POSITION_CONFIG_LENGTH; i++) {
		commands[i] = pgm_read_byte(POSITION_CONFIG + i);
	}
	
	ssd1306_controller.pattern_buff[0] = DATA_BYTE;
	
	for (size_t i = 0; i < BUFF_LENGTH - 1; i++) {
		ssd1306_controller.pattern_buff[i + 1] = *(pattern_bytes + i);
	}
	
	commands[3] = (uint8_t) (8 * (column - 1));
	commands[5] = (uint8_t) (7 + (8 * (column - 1)));
	commands[9] = (uint8_t) (row - 1);
	commands[11] = (uint8_t) (row - 1);
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, POSITION_CONFIG_LENGTH, SSD1306_SLA_ADD, commands);
			
	if (error_code != TWI_ERR_OK) {
		return SSD_ERR_TWI;
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, BUFF_LENGTH, SSD1306_SLA_ADD, ssd1306_controller.pattern_buff);
			
	if (error_code != TWI_ERR_OK) {
		return SSD_ERR_TWI;
	}
	
	ssd1306_controller.state.data_reset_flag = 0;	
	return SSD_ERR_OK;
}

ssd_errors ssd1306_data_reset(void) {
	ssd_errors error_code = 0;
	uint8_t blank[BUFF_LENGTH - 1] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	if (ssd1306_controller.state.data_reset_flag) {
		return SSD_ERR_OK;
	}
	
	for (size_t i = 0; i < DISPLAY_MAX_ROW; i++) {
			
		for (size_t j = 0; j < DISPLAY_MAX_COLUMN; j++) {	
			error_code = ssd1306_data_write(blank, i + 1, j + 1);
			
			if (error_code != SSD_ERR_OK) {
				return error_code;
			}
		}
	}
	
	ssd1306_controller.state.data_reset_flag = 1;	
	return SSD_ERR_OK;
}
