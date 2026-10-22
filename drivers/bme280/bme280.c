/**
*   
*   @file bme280.c
*
*   @brief Implementation for the bme280 module.
*
*   @details Contains the configuration commands and the temperature
*	compensation logic.
*
*   Handles the transmission using the 2-wire interface.
*
*/

#include <stdint.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include "bme280.h"
#include "twi.h"
#include "config.h"

#define CTRL_HUM_ADD 0xF2
#define CTRL_HUM_SETTINGS 0

#define TEMP_X16_NORMAL_MODE (1 << 7) | (1 << 5) | (1 << 1) | 1
#define CTRL_MEAS_ADD 0xF4
#define CTRL_MEAS_SETTINGS TEMP_X16_NORMAL_MODE

#define CONFIG_ADD 0xF5
#define CONFIG_SETTINGS 0

#define TEMP_MSB_ADD 0xFA
#define DIG_T1_ADD 0x88

#define RESET_ADD 0xE0
#define RESET_VALUE 0xB6

#define VALUE_BYTES 3U
#define COMPENSATION_PARAMS_BYTES 6U

#define MODE_CONFIG_LENGTH 6U
#define SOFT_RESET_LENGTH 2

static const uint8_t MODE_CONFIG[MODE_CONFIG_LENGTH] PROGMEM = {CTRL_HUM_ADD, CTRL_HUM_SETTINGS, CONFIG_ADD, CONFIG_SETTINGS, CTRL_MEAS_ADD, CTRL_MEAS_SETTINGS};

static const uint8_t SOFT_RESET[SOFT_RESET_LENGTH] PROGMEM = {RESET_ADD, RESET_VALUE};

typedef struct {
	uint8_t config_init_flag;
	uint8_t params_init_flag;
} sensor_state;

typedef struct {
	uint16_t dig_t1;
	int16_t dig_t2;
	int16_t dig_t3;
} compensation_params;

typedef struct {
	sensor_state state;
	compensation_params params;
} sensor_controller;

static sensor_controller bme280_controller = {0};

static twi_errors params_init(void) {
	twi_errors error_code = 0;
	uint8_t commands[1] = {DIG_T1_ADD};
	uint8_t data[COMPENSATION_PARAMS_BYTES] = {0, 0, 0, 0, 0, 0};
	
	if (bme280_controller.state.params_init_flag) {
		return TWI_ERR_OK;
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 1, BME280_SLA_ADD, commands);
		
	if (error_code != TWI_ERR_OK) {
		return error_code;
	}
		
	error_code = twi_master_receiver(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, COMPENSATION_PARAMS_BYTES, BME280_SLA_ADD, data);
		
	if (error_code != TWI_ERR_OK) {
		return error_code;
	}

/*
*
*		In the C11 standard the bit-wise left shifting is defined as E1 << E2 = E1 x 2^(E2)
*		
*		-Both the operands undergo integer promotion: if they fit in the int type they get converted into it, otherwise if
*		they fit into the unsigned int type they get converted into it, if they don't fit in any of these they are left unchanged
*
*		-If E1 has a signed type and non-negative value, then if E1 x 2^(E2) fits in the type that is the result, otherwise
*		the expression results in UB
*
*		-If E1 has an unsigned type then the result is E1 x 2^(E2) mod (MAX + 1), where MAX is the maximum value supported by that type
*
*		-In this case data[n] is uint8_t so it would become int16_t (defined in Avr-LibC stdint.h as signed int)
*
*		-Since uint8_t has a range of [0, 255] and int16_t has a range of [-32768, 32767], then the positive value of data[n]
*		is guaranteed to be represented as a positive value after the conversion in int16_t
*		
*		-Since the values of the compensation parameters are unknown, the expression could result in UB:
*		data[n] > 127 -> data[n] x 2^(8) = data[n] x 256 > 32767
*
*		-The left operand is therefore converted to uint16_t (defined in Avr-LibC stdint.h as unsigned int) to guarantee safety from UB
*		
*/
	bme280_controller.params.dig_t1 = ((uint16_t) data[1] << 8) | data[0];
	bme280_controller.params.dig_t2 = (int16_t) (((uint16_t) data[3] << 8) | data[2]);
	bme280_controller.params.dig_t3 = (int16_t) (((uint16_t) data[5] << 8) | data[4]);
	bme280_controller.state.params_init_flag = 1;
	return TWI_ERR_OK;
}

static twi_errors temp_compensate(int32_t captured_temp, int16_t* temp_c_x100) {
	twi_errors error_code = 0;
	int32_t var1 = 0;
	int32_t var2 = 0;
	
	if (!bme280_controller.state.params_init_flag) {
		error_code = params_init();
		
		if (error_code != TWI_ERR_OK) {
			return error_code;
		}
	}
	
	var1 = (((captured_temp >> 3) - ((int32_t) bme280_controller.params.dig_t1 << 1)) * bme280_controller.params.dig_t2) >> 11;
	var2 = (((((captured_temp >> 4) - ((int32_t) bme280_controller.params.dig_t1)) * ((captured_temp >> 4) - ((int32_t) bme280_controller.params.dig_t1))) >> 12) * (bme280_controller.params.dig_t3)) >> 14;
	
	/*
	*
	*		The expected value from the datasheet is in hundredth of Celsius degrees within the range [-4000, 8500]
	*
	*/
	*temp_c_x100 = (int16_t) (((var1 + var2) * 5 + 128) >> 8);
	return TWI_ERR_OK;
}

static twi_errors mode_config(void) {
	twi_errors error_code = 0;
	uint8_t commands[MODE_CONFIG_LENGTH];
	
	if (bme280_controller.state.config_init_flag) {
		return TWI_ERR_OK;
	}
	
	for (size_t i = 0; i < MODE_CONFIG_LENGTH; i++) {
		commands[i] = pgm_read_byte(MODE_CONFIG + i);
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, MODE_CONFIG_LENGTH, BME280_SLA_ADD, commands);
	
	if (error_code != TWI_ERR_OK) {
		return error_code;
	}
	
	bme280_controller.state.config_init_flag = 1;	
	return TWI_ERR_OK;
}

bme_errors bme280_temp_capture(int16_t* temp_c_x10) {
	twi_errors error_code = 0;
	uint8_t commands[1] = {TEMP_MSB_ADD};
	uint8_t data[VALUE_BYTES] = {0, 0, 0};
	int16_t temp_c_x100 = 0;
	int32_t captured_temp = 0;
	
	if (temp_c_x10 == NULL) {
		return BME_ERR_PARAM;
	}
	
	if (!bme280_controller.state.config_init_flag) {
		error_code = mode_config();
		
		if (error_code != TWI_ERR_OK) {
			return BME_ERR_TWI;
		}
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 1, BME280_SLA_ADD, commands);
	
	if (error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	error_code = twi_master_receiver(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, VALUE_BYTES, BME280_SLA_ADD, data);
	
	if (error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	captured_temp = ((int32_t) data[0] << 12) | ((int32_t) data[1] << 4) | (data[2] >> 4);
	error_code = temp_compensate(captured_temp, &temp_c_x100);
	
	if (error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	if (temp_c_x100 / 10 < BME280_MIN_TEMP_C_X10) {
		*temp_c_x10 = BME280_MIN_TEMP_C_X10;
	}
	
	else if (temp_c_x100 / 10 > BME280_MAX_TEMP_C_X10) {
		*temp_c_x10 = BME280_MAX_TEMP_C_X10;
	}
	
	else {
		*temp_c_x10 = temp_c_x100 / 10;
	}
	
	return BME_ERR_OK;
}

bme_errors bme280_stop(void) {
	twi_errors error_code = 0;
	uint8_t commands[SOFT_RESET_LENGTH];
	
	if (!bme280_controller.state.config_init_flag) {
		return BME_ERR_OK;
	}
	
	for (size_t i = 0; i < SOFT_RESET_LENGTH; i++) {
		commands[i] = pgm_read_byte(SOFT_RESET + i);
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, SOFT_RESET_LENGTH, BME280_SLA_ADD, commands);
	
	if (error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	bme280_controller.state.config_init_flag = 0;
	bme280_controller.state.params_init_flag = 0;
	bme280_controller.params.dig_t1 = 0;
	bme280_controller.params.dig_t2 = 0;
	bme280_controller.params.dig_t3 = 0;
	return BME_ERR_OK;
}
