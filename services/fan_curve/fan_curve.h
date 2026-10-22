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
*	@brief	Updates a pre-existing zero-curve saved in EEPROM.
*
*	@param	curve_size				Number of nodes
*	@param	node_temp				Pointer to the first element of the temperature array
*	@param	node_speed				Pointer to the first element of the speed array
*
*	@retval	FAN_CURVE_ERR_OK 		If no error occurs
*	@retval	FAN_CURVE_ERR_EEPROM 	If the curve is not correctly saved in EEPROM
*	@retval	FAN_CURVE_ERR_PARAM		If !(0 < curve_size <= 10) || node_temp == NULL || node_speed == NULL
*	@retval	FAN_CURVE_ERR_PARAM		If !(0 <= node_temp[i] <= 999) || (0 <= node_speed[i] <= 9999)
*	@retval	FAN_CURVE_ERR_PARAM		If passed temperatures or speeds are not increasingly higher or constant
*
*	@pre 	0 < curve_size <= 10
*	@pre 	node_temp points to the first element of an array of size curve_size containing the temperatures of all nodes
*	@pre 	node_speed points to the first element of an array of size curve_size containing the speeds of all nodes
*	@pre 	Passed temperatures or speeds are within valid boundaries and are increasingly higher or constant
*   @post   If the buffer arrays have a size smaller than curve_size the correct execution is not guaranteed
*   @post   If invalid parameters are passed the curve saved in EEPROM is left unchanged
*   @post   If the curve is not correctly saved in EEPROM changes to the curve saved in EEPROM are left
*	@post 	On success the curve saved in EEPROM is updated with the new size, temperature and speed values
*
*	@par 	Invariants:
*
*			-The size of the curve saved in EEPROM is always 10
*
*			-The number of updated nodes matches curve_size and is always bigger than 0
*
*			-The sequence of the nodes matches that of the temperature array and of the speed array
*
*			-The sequence of temperatures and speeds is ordered from lowest to highest
*
*			-All temperatures and speeds are within valid boundaries
*
*/
fan_curve_errors fan_curve_create(uint8_t curve_size, const uint16_t* node_temp, const uint16_t* node_speed);

/**
*
*	@brief	Calculates the speed value for the current temperature from the curve saved in EEPROM.
*
*	@param	temp_c					Measured current temperature
*	@param	speed_rpm				Pointer to the variable where to save the speed value
*
*	@retval	FAN_CURVE_ERR_OK 		If no error occurs
*	@retval	FAN_CURVE_ERR_EEPROM 	If the curve is not correctly loaded from EEPROM or if fan_curve_create has never been called
*	@retval	FAN_CURVE_ERR_PARAM		If speed_rpm == NULL
*
*	@pre 	fan_curve_create has been called
*	@pre 	speed_rpm != NULL
*   @post   If invalid parameters are passed speed_rpm is left unchanged
*   @post   If the curve is not correctly loaded from EEPROM or fan_curve_create has never been called speed_rpm is left unchanged
*	@post 	On success if temp_c is smaller than the lowest temperature the speed value matches the lowest speed
*	@post 	On success if temp_c is bigger than the highest temperature the speed value matches the highest speed
*	@post 	On success if temp_c falls between two nodes a linear interpolation is performed to evaluate the speed value
*	@post 	On success the speed value from the curve is saved in the variable speed_rpm
*
*	@par 	Invariants:
*
*			-The number of nodes of the curve saved in EEPROM used to calculate the speed value matches the curve_size field of the
*			curve saved in EEPROM
*
*/
fan_curve_errors fan_curve_get_speed(uint16_t temp_c, uint16_t* speed_rpm);

#endif
