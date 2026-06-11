/**
*
*	@file twi.h
*
*	@brief Public API for the twi module.
*
*	@details Module to transmit and receive bytes through the 2-wire interface of
*	an ATmega328P implementing an interrupt-based finite state machine (Mealy).
*
*	All data structures and ISR logic are hidden from the caller to enforce
*	low coupling.
*
*/

/*
*
*	For the ATmega328P:
*
*	SCL = (CPU Clock Frequency) / (16 + (2 * bitrate * bitrate prescaler))
*
*	0 <= bitrate register <= 255 (8-bit register)
*
*	bitrate prescaler register value -> bitrate prescaler value: 0 -> 1, 1 -> 4, 2 -> 16, 3 -> 64
*
*	For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz
*
*/

#ifndef TWI_H
#define TWI_H

#include <stdint.h>
#include <util/twi.h>

#define SLA_W(sla_add) (sla_add | TW_WRITE)
#define SLA_R(sla_add) (sla_add | TW_READ)

typedef enum {
	TWI_ERR_OK = 0,
	TWI_ERR_START,
	TWI_ERR_REP_START,
	TWI_ERR_SLAW_SEND,
	TWI_ERR_SLAR_SEND,
	TWI_ERR_DATA_SEND,
	TWI_ERR_ACK_SEND,
	TWI_ERR_NACK_SEND,
	TWI_ERR_PARAM,
	TWI_ERR_TIMEOUT
} twi_errors;

/**
*
*	@brief	Transmits bytes from a buffer to a slave device by configuring and booting the state machine.
*
*	@param	bitrate					Value for the 8-bit valued register to configure the bitrate
*	@param	bitrate_prsc 			Value for the 8-bit valued register to configure the bitrate prescaler
*	@param	bytes_num				Number of bytes to be transmitted
*	@param	sla_add					Address of the slave device
*	@param	tx_buff					Pointer to the first element of the buffer array
*
*	@retval	TWI_ERR_OK				If no error occurs
*	@retval	TWI_ERR_START			If TW_STATUS != TW_START after sending the start condition
*	@retval	TWI_ERR_SLAW_SEND		If TW_STATUS != TW_MT_SLA_ACK after sending the SLA + W
*	@retval	TWI_ERR_DATA_SEND		If TW_STATUS != TW_MT_DATA_ACK after sending a byte
*	@retval	TWI_ERR_PARAM			If tx_buff == NULL || bitrate_prsc > 3 || bytes_num == 0
*	@retval	TWI_ERR_TIMEOUT			If the transmission lasts longer than TWI_TIMEOUT_MS
*
*	@pre 	0 <= bitrate <= 255
*	@pre 	0 <= bitrate_prsc <= 3
*	@pre 	bytes_num > 0
*	@pre 	tx_buff points to the first element of an array containing the bytes to be transmitted
*   @post   If passed parameters are invalid the state machine is not configured and the transmission is not started
*   @post   If an error with TW_STATUS occurs the transmission is stopped, changes to the state machine and the slave device are left and the error is returned
*   @post   If the transmission lasts longer than TWI_TIMEOUT_MS the transmission is stopped and changes to the state machine and the slave device are left
*	@post 	On success all the bytes are transmitted from the buffer array to the slave device
*	@post	On success the executed progressive states are: START SENT, SLAW SENT, DATA SENT and STOP SENT
*	@post 	On success the transmission is stopped and the state machine is left configured
*
*	@par 	Invariants:
*
*			-The state DATA SENT is executed a number of times equal to bytes_num
*
*			-The input of the state machine is the progressive index of the current state
*
*/
twi_errors twi_master_transmitter(uint8_t bitrate, uint8_t bitrate_prsc, uint16_t bytes_num, uint8_t sla_add, const uint8_t* tx_buff);

/**
*
*	@brief	Receives bytes from a slave device and saves them in a buffer by configuring and booting the state machine.
*
*	@param	bitrate					Value for the 8-bit valued register to configure the bitrate
*	@param	bitrate_prsc 			Value for the 8-bit valued register to configure the bitrate prescaler
*	@param	bytes_num				Number of bytes to be received
*	@param	sla_add					Address of the slave device
*	@param	rx_buff					Pointer to the first element of the buffer array
*
*	@retval	TWI_ERR_OK				If no error occurs
*	@retval	TWI_ERR_START			If TW_STATUS != TW_START after sending the start condition
*	@retval	TWI_ERR_SLAR_SEND		If TW_STATUS != TW_MR_SLA_ACK after sending the SLA + R
*	@retval	TWI_ERR_ACK_SEND 		If TW_STATUS != TW_MR_DATA_ACK after receiving a byte
*	@retval	TWI_ERR_NACK_SEND		If TW_STATUS != TW_MR_DATA_NACK after receiving the last byte
*	@retval	TWI_ERR_PARAM			If rx_buff == NULL || bitrate_prsc > 3 || bytes_num == 0
*	@retval	TWI_ERR_TIMEOUT			If the transmission lasts longer than TWI_TIMEOUT_MS
*
*	@pre 	0 <= bitrate <= 255
*	@pre 	0 <= bitrate_prsc <= 3
*	@pre 	bytes_num > 0
*	@pre 	rx_buff points to the first element of an array containing the bytes to be received
*   @post   If passed parameters are invalid the state machine is not configured and the transmission is not started
*   @post   If an error with TW_STATUS occurs the transmission is stopped, changes to the state machine and the buffer array are left and the error is returned
*   @post   If the transmission lasts longer than TWI_TIMEOUT_MS the transmission is stopped and changes to the state machine and the buffer array are left
*	@post 	On success all the bytes are received from the slave device and saved in the buffer array
*	@post	On success the executed progressive states are: START SENT, SLAR SENT, ACK SENT, NACK SENT and STOP SENT
*	@post 	On success the transmission is stopped and the state machine is left configured
*
*	@par 	Invariants:
*
*			-The state ACK SENT is executed a number of times equal to bytes_num - 1
*
*			-The input of the state machine is the progressive index of the current state
*
*/
twi_errors twi_master_receiver(uint8_t bitrate, uint8_t bitrate_prsc, uint16_t bytes_num, uint8_t sla_add, uint8_t* rx_buff);

#endif
