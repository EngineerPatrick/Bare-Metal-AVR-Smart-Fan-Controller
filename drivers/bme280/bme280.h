/**
*
*	@file bme280.h
*
*	@brief Public API for the bme280 module.
*
*	@details Module to read the temperature from the sensor
*	integrating the 2-wire interface for transmission.
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
*	@brief	Captures the temperature from the sensor and saves it.
*
*	@param	temp_c_x10[out]						Temperature
*
*	@retval	BME_ERR_OK							If no error occurs
*	@retval	BME_ERR_PARAM						If temp_c_x10 == NULL
*	@retval	BME_ERR_TWI							If an error with the twi driver occurs
*
*	@pre	This function is polled to capture the sensor temperature live readings
*	@post	If the passed parameter is invalid then the transmission is not started and the temperature is not saved
*	@post	If an error with the twi driver occurs then the correct execution is not guaranteed
*	@post	On success the captured temperature is compensated using the datasheet formulas and clamped within the valid boundaries
*	@post	On success the temperature is saved
*	@post	On success the transmission is stopped and the sensor is left configured for temperature live readings (~26 Hz)
*
*/
bme_errors bme280_temp_capture(int16_t* temp_c_x10);

/**
*
*	@brief	Stops the sensor from performing temperature live readings and resets it.
*
*	@retval	BME_ERR_OK							If no error occurs
*	@retval	BME_ERR_TWI							If an error with the twi driver occurs
*
*	@post	If an error with the twi driver occurs then the correct execution is not guaranteed
*	@post	If bme280_temp_capture has not been called then the sensor is left unchanged
*	@post	On success the transmission is stopped and the sensor is reset
*
*/
bme_errors bme280_stop(void);

#endif
