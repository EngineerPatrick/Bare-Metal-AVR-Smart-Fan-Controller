/**
*   
*   @file twi.c
*
*   @brief Implementation for the twi module for the ATmega328P.
*
*   @details Contains the interrupt finite state-machine, which could be
*	reused with inputs different from the progressive index.
*
*   Validates transmission completion and handles hardware faults with
*	scheduler timeout.
*
*/

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/twi.h>
#include "twi.h"
#include "scheduler.h"
#include "config.h"

#define BITRATE_REGISTER TWBR

#define CONTROL_REGISTER TWCR
#define TWI_INT_FLAG (1 << TWINT)
#define START_CONDITION (1 << TWSTA)
#define ACK_CONDITION (1 << TWEA)
#define STOP_CONDITION (1 << TWSTO)
#define TWI_ENABLE (1 << TWEN)
#define TWI_INT_ENABLE (1 << TWIE)

#define STATUS_REGISTER TWSR
#define DATA_REGISTER TWDR

#define STATES_NUMBER 7U

#define MAX_BYTE_NUM 250U

typedef enum {
	START = 0,
	REP_START,
	SLAW_SEND,
	SLAR_SEND,
	DATA_SEND,
	ACK_SEND,
	NACK_SEND,
	STOP
} twi_operations;

typedef struct {
	twi_operations next_op;
	void (*next_op_ptr)(void);
	uint16_t input[2];
} twi_state_branch;

typedef struct {
	twi_operations current_op;
	twi_state_branch branches[STATES_NUMBER];
} twi_state;

typedef 	void (*function_ptr)(void);

typedef struct {
	volatile uint8_t status_code;
	volatile twi_errors error;
	twi_state states[STATES_NUMBER];
	volatile twi_state current_state;
	volatile function_ptr next_op_ptr;
	volatile uint16_t current_input;
	uint8_t sla_add;
	volatile const uint8_t* tx_buff;
	volatile uint8_t* rx_buff;
	volatile uint8_t stop_sent;
} twi_controller;

static twi_controller twi_state_machine = {0};

inline static void twi_operation_start(void) {
	CONTROL_REGISTER = TWI_ENABLE | TWI_INT_ENABLE | TWI_INT_FLAG;
}

inline static void twi_start_send(void) {
	CONTROL_REGISTER = START_CONDITION | TWI_ENABLE | TWI_INT_ENABLE | TWI_INT_FLAG;
}

inline static void twi_slaw_send(void) {
	DATA_REGISTER = SLA_W(twi_state_machine.sla_add);
	twi_operation_start();
}

inline static void twi_slar_send(void) {
	DATA_REGISTER = SLA_R(twi_state_machine.sla_add);
	twi_operation_start();
}

inline static void twi_data_send(void) {
	DATA_REGISTER = *(twi_state_machine.tx_buff);
	twi_operation_start();
}

inline static void twi_ack_send(void) {
	CONTROL_REGISTER = ACK_CONDITION | TWI_ENABLE | TWI_INT_ENABLE | TWI_INT_FLAG;
}

inline static void twi_stop_send(void) {
	CONTROL_REGISTER = STOP_CONDITION | TWI_ENABLE | TWI_INT_FLAG;
	twi_state_machine.stop_sent = 1;
}

inline static uint8_t twi_transmission_finished(void) {
	return (!(CONTROL_REGISTER & STOP_CONDITION)) ? 1 : 0;
}

inline static void twi_hardware_reset(void) {
	CONTROL_REGISTER = 0;
	DATA_REGISTER = 0;
}

/**
*
*	@brief	Advances the state-machine to the next state.
*
*	@pre 	Every used state has at least one branch pointing to the next state within an input range
*	@pre 	All input ranges of a used states are non-intersecting intervals with each others
*	@post	The state-machine is advanced to the state pointed by the branch with an input range which includes the current input
*
*/
static void twi_state_advance(void) {

	for (size_t i = 0; i < STATES_NUMBER - 1; i++) {
	
		if (twi_state_machine.current_input >= twi_state_machine.current_state.branches[i].input[0] && twi_state_machine.current_input <= twi_state_machine.current_state.branches[i].input[1]) {
			twi_state_machine.next_op_ptr = twi_state_machine.current_state.branches[i].next_op_ptr;
			twi_state_machine.current_state = twi_state_machine.states[twi_state_machine.current_state.branches[i].next_op];
			twi_state_machine.current_input++;
			break;
		}
		
		else if (i == STATES_NUMBER - 2 && twi_state_machine.current_input == twi_state_machine.current_state.branches[6].input[0]) {
			twi_state_machine.next_op_ptr = twi_state_machine.current_state.branches[6].next_op_ptr;
			break;
		}
	}
}

ISR(TWI_vect) {

	switch (twi_state_machine.current_state.current_op) {
	
		case START:
			
			if (TW_STATUS != TW_START) {
				twi_state_machine.status_code = TW_STATUS;
				twi_state_machine.error = TWI_ERR_START;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case REP_START:
			
			if (TW_STATUS != TW_REP_START) {
				twi_state_machine.status_code = TW_STATUS;
				twi_state_machine.error = TWI_ERR_REP_START;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case SLAW_SEND:
			
			if (TW_STATUS != TW_MT_SLA_ACK) {
				twi_state_machine.status_code = TW_STATUS;
				twi_state_machine.error = TWI_ERR_SLAW_SEND;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case SLAR_SEND:
			
			if (TW_STATUS != TW_MR_SLA_ACK) {
				twi_state_machine.status_code = TW_STATUS;
				twi_state_machine.error = TWI_ERR_SLAR_SEND;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case DATA_SEND:
			
			if (TW_STATUS != TW_MT_DATA_ACK) {
				twi_state_machine.status_code = TW_STATUS;
				twi_state_machine.error = TWI_ERR_DATA_SEND;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			twi_state_machine.tx_buff++;
			break;
			
		case ACK_SEND:
			
			if (TW_STATUS != TW_MR_DATA_ACK) {
				twi_state_machine.status_code = TW_STATUS;
				twi_state_machine.error = TWI_ERR_ACK_SEND;
				twi_stop_send();
				break;
			}
			
			*(twi_state_machine.rx_buff) = DATA_REGISTER;
			twi_state_machine.rx_buff++;
			twi_state_advance();
			break;
			
		case NACK_SEND:
			
			if (TW_STATUS != TW_MR_DATA_NACK) {
				twi_state_machine.status_code = TW_STATUS;
				twi_state_machine.error = TWI_ERR_NACK_SEND;
				twi_stop_send();
				break;
			}
			
			*(twi_state_machine.rx_buff) = DATA_REGISTER;
			twi_state_machine.rx_buff++;
			twi_state_advance();
			break;
		
		default:
			twi_state_machine.status_code = TW_STATUS;
			twi_state_machine.error = TWI_ERR_PARAM;
			twi_stop_send();
	}
	
	if (twi_state_machine.error == TWI_ERR_OK && twi_state_machine.next_op_ptr != NULL) {
		twi_state_machine.next_op_ptr();
	}
}

inline static void twi_start_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {
	twi_state_machine.states[START].branches[next_op - 1].input[0] = from_input;
	twi_state_machine.states[START].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_rep_start_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {
	twi_state_machine.states[REP_START].branches[next_op - 1].input[0] = from_input;
	twi_state_machine.states[REP_START].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_slaw_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {	
	twi_state_machine.states[SLAW_SEND].branches[next_op - 1].input[0] = from_input;
	twi_state_machine.states[SLAW_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_slar_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {
	twi_state_machine.states[SLAR_SEND].branches[next_op - 1].input[0] = from_input;
	twi_state_machine.states[SLAR_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_data_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {	
	twi_state_machine.states[DATA_SEND].branches[next_op - 1].input[0] = from_input;
	twi_state_machine.states[DATA_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_ack_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {		
	twi_state_machine.states[ACK_SEND].branches[next_op - 1].input[0] = from_input;
	twi_state_machine.states[ACK_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_nack_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {		
	twi_state_machine.states[NACK_SEND].branches[next_op - 1].input[0] = from_input;
	twi_state_machine.states[NACK_SEND].branches[next_op - 1].input[1] = to_input;
}

static void twi_state_machine_restore(void) {
	twi_state_machine.status_code = TW_NO_INFO;
	twi_state_machine.error = TWI_ERR_OK;
	
	twi_state_machine.states[START].current_op = START;
	twi_state_machine.states[REP_START].current_op = REP_START;
	twi_state_machine.states[SLAW_SEND].current_op = SLAW_SEND;
	twi_state_machine.states[SLAR_SEND].current_op = SLAR_SEND;
	twi_state_machine.states[DATA_SEND].current_op = DATA_SEND;
	twi_state_machine.states[ACK_SEND].current_op = ACK_SEND;
	twi_state_machine.states[NACK_SEND].current_op = NACK_SEND;
	
	for (size_t i = 0; i < STATES_NUMBER; i++) {
		twi_state_machine.states[i].branches[REP_START - 1].next_op = REP_START;
		twi_state_machine.states[i].branches[REP_START - 1].next_op_ptr = twi_start_send;
		twi_state_machine.states[i].branches[REP_START - 1].input[0] = 0;
		twi_state_machine.states[i].branches[REP_START - 1].input[1] = 0;
		
		twi_state_machine.states[i].branches[SLAW_SEND - 1].next_op = SLAW_SEND;
		twi_state_machine.states[i].branches[SLAW_SEND - 1].next_op_ptr = twi_slaw_send;
		twi_state_machine.states[i].branches[SLAW_SEND - 1].input[0] = 0;
		twi_state_machine.states[i].branches[SLAW_SEND - 1].input[1] = 0;
		
		twi_state_machine.states[i].branches[SLAR_SEND - 1].next_op = SLAR_SEND;
		twi_state_machine.states[i].branches[SLAR_SEND - 1].next_op_ptr = twi_slar_send;
		twi_state_machine.states[i].branches[SLAR_SEND - 1].input[0] = 0;
		twi_state_machine.states[i].branches[SLAR_SEND - 1].input[1] = 0;
		
		twi_state_machine.states[i].branches[DATA_SEND - 1].next_op = DATA_SEND;
		twi_state_machine.states[i].branches[DATA_SEND - 1].next_op_ptr = twi_data_send;
		twi_state_machine.states[i].branches[DATA_SEND - 1].input[0] = 0;
		twi_state_machine.states[i].branches[DATA_SEND - 1].input[1] = 0;
		
		twi_state_machine.states[i].branches[ACK_SEND - 1].next_op = ACK_SEND;
		twi_state_machine.states[i].branches[ACK_SEND - 1].next_op_ptr = twi_ack_send;
		twi_state_machine.states[i].branches[ACK_SEND - 1].input[0] = 0;
		twi_state_machine.states[i].branches[ACK_SEND - 1].input[1] = 0;
		
		twi_state_machine.states[i].branches[NACK_SEND - 1].next_op = NACK_SEND;
		twi_state_machine.states[i].branches[NACK_SEND - 1].next_op_ptr = twi_operation_start;
		twi_state_machine.states[i].branches[NACK_SEND - 1].input[0] = 0;
		twi_state_machine.states[i].branches[NACK_SEND - 1].input[1] = 0;
		
		twi_state_machine.states[i].branches[STOP - 1].next_op = STOP;
		twi_state_machine.states[i].branches[STOP - 1].next_op_ptr = twi_stop_send;
		twi_state_machine.states[i].branches[STOP - 1].input[0] = 0;
		twi_state_machine.states[i].branches[STOP - 1].input[1] = 0;
	}
	
	twi_state_machine.next_op_ptr = NULL;
	twi_state_machine.current_state = twi_state_machine.states[START];
	twi_state_machine.current_input = 1;
	twi_state_machine.sla_add = 0;
	twi_state_machine.tx_buff = NULL;
	twi_state_machine.rx_buff = NULL;
	twi_state_machine.stop_sent = 0;
}

static void twi_state_machine_boot(uint8_t bitrate, uint8_t bitrate_prsc, uint8_t sla_add, const uint8_t* tx_buff, uint8_t* rx_buff){
	
	BITRATE_REGISTER = bitrate;
	STATUS_REGISTER = bitrate_prsc;
	CONTROL_REGISTER = TWI_ENABLE;
	
	twi_state_machine.sla_add = sla_add;
	twi_state_machine.tx_buff = tx_buff;
	twi_state_machine.rx_buff = rx_buff;
	twi_state_machine.current_state = twi_state_machine.states[START];
	
	twi_start_send();
}

twi_errors twi_master_transmitter(uint8_t bitrate, uint8_t bitrate_prsc, uint16_t bytes_num, uint8_t sla_add, const uint8_t* tx_buff) {
	uint8_t stop_sent = 0;
	uint16_t timeout_t_0_ms = 0;
	twi_errors error = 0;

	if (tx_buff == NULL || bitrate_prsc > 3 || bytes_num == 0 || bytes_num > MAX_BYTE_NUM) {
		return TWI_ERR_PARAM;
	}

	twi_state_machine_restore();	
	twi_start_state_config(SLAW_SEND, 1, 1);	
	twi_slaw_send_state_config(DATA_SEND, 2, 2);	
	twi_data_send_state_config(DATA_SEND, 3, (bytes_num + 1));	
	twi_data_send_state_config(STOP, (bytes_num + 2), (bytes_num + 2));
	twi_state_machine_boot(bitrate, bitrate_prsc, sla_add, tx_buff, NULL);
	timeout_t_0_ms = scheduler_timestamp_capture();
	
	while (1) {
		
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			stop_sent = twi_state_machine.stop_sent;
		}
		
		if (stop_sent && twi_transmission_finished()) {
			twi_hardware_reset();
			error = twi_state_machine.error;
			twi_state_machine_restore();
		    break;
		}
		
		if (scheduler_timer_elapsed(timeout_t_0_ms, TWI_TIMEOUT_MS)) {
			twi_hardware_reset();
			error = twi_state_machine.error;
			twi_state_machine_restore();
			return TWI_ERR_TIMEOUT;
		}
	}
	
	return error;
}

twi_errors twi_master_receiver(uint8_t bitrate, uint8_t bitrate_prsc, uint16_t bytes_num, uint8_t sla_add, uint8_t* rx_buff) {
	uint8_t stop_sent = 0;
	uint16_t timeout_t_0_ms = 0;
	twi_errors error = 0;

	if (rx_buff == NULL || bitrate_prsc > 3 || bytes_num == 0 || bytes_num > MAX_BYTE_NUM) {
		return TWI_ERR_PARAM;
	}

	twi_state_machine_restore();	
	twi_start_state_config(SLAR_SEND, 1, 1);
	
	switch (bytes_num) {
		
		case 1:
		
			twi_slar_send_state_config(NACK_SEND, 2, 2);
			twi_nack_send_state_config(STOP, 3, 3);
			break;
		
		case 2:
		
			twi_slar_send_state_config(ACK_SEND, 2, 2);		
			twi_ack_send_state_config(NACK_SEND, 3, 3);
			twi_nack_send_state_config(STOP, 4, 4);
			break;
		
		default:
			
			twi_slar_send_state_config(ACK_SEND, 2, 2);		
			twi_ack_send_state_config(ACK_SEND, 3, (bytes_num));		
			twi_ack_send_state_config(NACK_SEND, (bytes_num + 1), (bytes_num + 1));
			twi_nack_send_state_config(STOP, (bytes_num + 2), (bytes_num + 2));
	}
	
	twi_state_machine_boot(bitrate, bitrate_prsc, sla_add, NULL, rx_buff);
	timeout_t_0_ms = scheduler_timestamp_capture();
	
	while (1) {
		
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			stop_sent = twi_state_machine.stop_sent;
		}
		
		if (stop_sent && twi_transmission_finished()) {
			twi_hardware_reset();
			error = twi_state_machine.error;
			twi_state_machine_restore();
		    break;
		}
		
		if (scheduler_timer_elapsed(timeout_t_0_ms, TWI_TIMEOUT_MS)) {
			twi_hardware_reset();
			error = twi_state_machine.error;
			twi_state_machine_restore();
			return TWI_ERR_TIMEOUT;
		}
	}
	
	return error;
}
