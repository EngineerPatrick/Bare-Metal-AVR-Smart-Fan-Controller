/**
*
*	@file crc.h
*
*	@brief Public API for the crc module.
*
*	@details Module to implement the CRC16-CCITT algorithm.
*
*/

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/**
*
*	@brief	Calculates the CRC16-CCITT of a given bytes sequence.
*
*	@param	length			    		Number of bytes
*	@param	data						Address of the first byte
*
*	@retval	0   						If data == NULL || !length
*
*	@pre 	data != NULL && length > 0
*	@post 	On success the correct CRC is calculated and returned
*
*/
uint16_t crc16_ccitt(uint16_t length, const uint8_t* data);

#endif
