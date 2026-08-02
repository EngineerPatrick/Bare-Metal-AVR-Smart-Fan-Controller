/**
*
*	@file bme280.h
*
*	@brief Public API for the bme280 module.
*
*	@details Module to read the temperature and the chip ID from
*	the sensor integrating the 2-wire interface for transmission.
*
*	Implements compensation formulas provided by the manufacturer.
*
*/

#ifndef BME280_H
#define BME280_H

#include <stdint.h>

#define BME280_SLA_ADD (0x76 << 1)

typedef enum {
	BME_ERR_OK = 0,
	BME_ERR_PARAM,
	BME_ERR_TWI
} bme_errors;

/**
*
*	@brief	Reads the chip ID from the sensor.
*
*	@param	chip_id					Pointer to the variable where to save the chip ID
*
*	@retval	BME_ERR_OK				If no error occurs
*	@retval	BME_ERR_PARAM			If chip_id == NULL
*	@retval	BME_ERR_TWI				If an error with the 2-wire interface occurs
*
*   @pre   	chip_id != NULL
*   @post   If passed parameter is invalid the transmission is not started and chip_id is left unchanged
*   @post   If an error with the 2-wire interface occurs the correct execution is not guaranteed
*	@post 	On success the chip ID is saved in chip_id
*	@post 	On success the transmission is stopped and the sensor is left configured
*
*/
bme_errors bme280_chip_id_read(uint8_t* chip_id);

/**
*
*	@brief	Configures the sensor to perform temperature live readings.
*
*	@retval	BME_ERR_OK				If no error occurs
*	@retval	BME_ERR_TWI				If an error with the 2-wire interface occurs
*
*   @post   If an error with the 2-wire interface occurs the correct execution is not guaranteed
*	@post 	On success the sensor is configured to perform temperature live readings
*	@post 	On success the transmission is stopped and the sensor is left configured
*
*/
bme_errors bme280_temp_init(void);

/**
*
*	@brief	Reads the temperature from the sensor and saves it.
*
*	@param	temp_c_x10				Pointer to the variable where to save the temperature
*
*	@retval	BME_ERR_OK				If no error occurs
*	@retval	BME_ERR_PARAM			If temp_c_x10 == NULL
*	@retval	BME_ERR_TWI				If an error with the 2-wire interface occurs
*
*   @pre   	temp_c_x10 != NULL
*   @post   If passed parameter is invalid the transmission is not started and the variable temp_c_x10 is left unchanged
*   @post   If an error with the 2-wire interface occurs the correct execution is not guaranteed
*	@post 	On success the measured value is compensated using the datasheet formulas and clamped within the valid boundaries
*	@post 	On success the temperature is saved in the variable temp_c_x10
*	@post 	On success the transmission is stopped and the sensor is left configured
*
*/
bme_errors bme280_temp_read(int16_t* temp_c_x10);

#endif
