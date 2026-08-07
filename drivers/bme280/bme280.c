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

#define ID_ADD 0xD0

#define CTRL_HUM_ADD 0xF2
#define CTRL_HUM_SETTINGS 0

#define CTRL_MEAS_ADD 0xF4
#define CTRL_MEAS_SETTINGS (1 << 7) | (1 << 5) | (1 << 1) | 1

#define CONFIG_ADD 0xF5
#define CONFIG_SETTINGS 0

#define TEMP_MSB_ADD 0xFA

#define DIG_T1_ADD 0x88

#define VALUE_BYTES 3U
#define COMPENSATION_PARAMS_BYTES 6U

#define CONFIG_LENGTH 6U

static const uint8_t CONFIG[CONFIG_LENGTH] PROGMEM = {CTRL_HUM_ADD, CTRL_HUM_SETTINGS, CONFIG_ADD, CONFIG_SETTINGS, CTRL_MEAS_ADD, CTRL_MEAS_SETTINGS};

static uint8_t init_flag = 0;

typedef struct {
	uint16_t dig_t1;
	int16_t dig_t2;
	int16_t dig_t3;
	uint8_t init_flag;
} compensation_params;

static compensation_params comp_params = {0};

static twi_errors bme280_temp_compensate(int32_t temp_adc_value, int16_t* temp_c_x100) {
	twi_errors error_code = 0;
	int32_t var1 = 0;
	int32_t var2 = 0;
	uint8_t tx_buff[1] = {DIG_T1_ADD};
	uint8_t rx_buff[COMPENSATION_PARAMS_BYTES] = {0, 0, 0, 0, 0, 0};
	
	if (!comp_params.init_flag) {
		error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 1, BME280_SLA_ADD, tx_buff);
		
		if (error_code != TWI_ERR_OK) {
			return error_code;
		}
		
		error_code = twi_master_receiver(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, COMPENSATION_PARAMS_BYTES, BME280_SLA_ADD, rx_buff);
		
		if (error_code != TWI_ERR_OK) {
			return error_code;
		}

		comp_params.dig_t1 = (rx_buff[1] << 8) | rx_buff[0];
		comp_params.dig_t2 = (rx_buff[3] << 8) | rx_buff[2];
		comp_params.dig_t3 = (rx_buff[5] << 8) | rx_buff[4];
		comp_params.init_flag = 1;
	}
	
	var1 = (((temp_adc_value >> 3) - ((int32_t) comp_params.dig_t1 << 1)) * comp_params.dig_t2) >> 11;
	var2 = (((((temp_adc_value >> 4) - ((int32_t) comp_params.dig_t1)) * ((temp_adc_value >> 4) - ((int32_t) comp_params.dig_t1))) >> 12) * (comp_params.dig_t3)) >> 14;
	
	/*
	*
	*		The expected value from the datasheet is in hundredth of Celsius degrees within the range [-4000, 8500]
	*
	*/
	*temp_c_x100 = (int16_t) (((var1 + var2) * 5 + 128) >> 8);
		
	return TWI_ERR_OK;
}

bme_errors bme280_chip_id_read(uint8_t* chip_id) {
	twi_errors error_code = 0;
	uint8_t tx_buff[1] = {ID_ADD};
	uint8_t rx_buff[1] = {0};
	
	if (chip_id == NULL) {
		return BME_ERR_PARAM;
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 1, BME280_SLA_ADD, tx_buff);
	
	if (error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	error_code = twi_master_receiver(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 1, BME280_SLA_ADD, rx_buff);
	
	if (error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	*chip_id = rx_buff[0];	
	return BME_ERR_OK;
}

bme_errors bme280_temp_init(void) {
	twi_errors error_code = 0;
	uint8_t tx_buff[CONFIG_LENGTH];
	
	for (uint8_t i = 0; i < CONFIG_LENGTH; i++) {
		tx_buff[i] = pgm_read_byte(CONFIG + i);
	}
	
	error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, CONFIG_LENGTH, BME280_SLA_ADD, tx_buff);
	
	if (error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	init_flag = 1;	
	return BME_ERR_OK;
}

bme_errors bme280_temp_read(int16_t* temp_c_x10) {
	twi_errors twi_error_code = 0;
	bme_errors bme_error_code = 0;
	uint8_t tx_buff[1] = {TEMP_MSB_ADD};
	uint8_t rx_buff[VALUE_BYTES] = {0, 0, 0};
	int32_t temp_adc_value = 0;
	int16_t temp_c_x100 = 0;
	
	if (temp_c_x10 == NULL) {
		return BME_ERR_PARAM;
	}
	
	if (!init_flag) {
		bme_error_code = bme280_temp_init();
		
		if (bme_error_code != BME_ERR_OK) {
			return bme_error_code;
		}
	}
	
	twi_error_code = twi_master_transmitter(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, 1, BME280_SLA_ADD, tx_buff);
	
	if (twi_error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	twi_error_code = twi_master_receiver(TWI_BITRATE_REG, TWI_BITRATE_PRESCALER, VALUE_BYTES, BME280_SLA_ADD, rx_buff);
	
	if (twi_error_code != TWI_ERR_OK) {
		return BME_ERR_TWI;
	}
	
	temp_adc_value = ((int32_t) rx_buff[0] << 12) | ((int32_t) rx_buff[1] << 4) | (rx_buff[2] >> 4);
	twi_error_code = bme280_temp_compensate(temp_adc_value, &temp_c_x100);
	
	if (twi_error_code != TWI_ERR_OK) {
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
