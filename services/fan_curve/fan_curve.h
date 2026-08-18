/**
*
*	@file fan_curve.h
*
*	@brief Public API for the fan_curve module.
*
*	@details Module to create a fan curve of max 10 nodes with
*	temperature and speed values.
*
*	The curve is saved in EEPROM and validated with CRC to restrain
*	RAM usage and to be reset-persistent for MCU applications.
*
*/

#ifndef FAN_CURVE_H
#define FAN_CURVE_H

#include <stdint.h>

typedef enum {
	FAN_CURVE_ERR_OK = 0,
	FAN_CURVE_ERR_EEPROM,
	FAN_CURVE_ERR_PARAM
} fan_curve_errors;

/**
*
*	@brief	Updates EEPROM with a new curve.
*
*	@param	curve_size					Number of nodes
*	@param	node_temp					Pointer to the first element of the temperature array
*	@param	node_speed					Pointer to the first element of the speed array
*
*	@retval	FAN_CURVE_ERR_OK 				If no error occurs
*	@retval	FAN_CURVE_ERR_EEPROM 				If the curve is not correctly saved in EEPROM
*	@retval	FAN_CURVE_ERR_PARAM				If !(0 < curve_size <= 10) || node_temp == NULL || node_speed == NULL
*	@retval	FAN_CURVE_ERR_PARAM				If !(0 <= node_temp[i] <= 999) || (0 <= node_speed[i] <= 9999)
*	@retval	FAN_CURVE_ERR_PARAM				If passed temperatures or speeds are not increasingly higher or constant
*
*	@pre 	0 < curve_size <= 10
*	@pre 	node_temp points to the first element of an array of size curve_size containing the temperatures of all nodes
*	@pre 	node_speed points to the first element of an array of size curve_size containing the speeds of all nodes
*	@pre 	Passed temperatures or speeds are within valid boundaries, speeds are increasingly higher or constant and temperatures are increasingly higher
*	@post   If the buffer arrays have a size smaller than curve_size the correct execution is not guaranteed
*	@post   If invalid parameters are passed EEPROM is left unchanged
*	@post   If the curve is not correctly saved in EEPROM changes to EEPROM are left
*	@post 	On success EEPROM is updated with the new size, temperature, speed values and CRC-16
*
*	@par 	Invariants:
*
*			-The total number of nodes updated in EEPROM is always 10
*
*			-The number of active nodes updated matches curve_size and is always bigger than 0
*
*			-The order of the nodes matches that of the temperature array and of the speed array
*
*			-The sequence of temperatures and speeds is ordered from lowest to highest
*
*			-All temperatures and speeds are within valid boundaries
*
*/
fan_curve_errors fan_curve_eeprom_update(uint8_t curve_size, const uint16_t* node_temp, const uint16_t* node_speed);

/**
*
*	@brief	Calculates the target speed from the curve saved in EEPROM for the current temperature.
*
*	@param	temp_c						Current temperature
*	@param	target_speed_rpm				Pointer to the variable where to save the target speed
*
*	@retval	FAN_CURVE_ERR_OK 				If no error occurs
*	@retval	FAN_CURVE_ERR_EEPROM 				If the curve is not correctly loaded from EEPROM or if fan_curve_eeprom_update has never been called
*	@retval	FAN_CURVE_ERR_PARAM				If target_speed_rpm == NULL
*
*	@pre 	fan_curve_eeprom_update has been called
*	@pre 	target_speed_rpm != NULL
*	@post   If invalid parameter is passed the target speed is not saved
*	@post   If the curve is not correctly loaded from EEPROM or fan_curve_eeprom_update has never been called the target speed is not saved
*	@post 	On success if temp_c is smaller than the lowest temperature the target speed matches the lowest speed
*	@post 	On success if temp_c is bigger than the highest temperature the target speed matches the highest speed
*	@post 	On success if temp_c falls between two nodes a linear interpolation is performed to evaluate the target speed
*	@post 	On success the target speed is saved in the pointed variable
*
*	@par 	Invariants:
*
*			-The number of nodes of the curve saved in EEPROM used to calculate the target speed matches the curve_size field of the
*			curve saved in EEPROM
*
*/
fan_curve_errors fan_curve_target_speed_compute(uint16_t temp_c, uint16_t* target_speed_rpm);

#endif
