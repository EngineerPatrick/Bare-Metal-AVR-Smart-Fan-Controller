/**
*   
*   @file twi.c
*
*   @brief Implementation for the twi module for the ATmega328P.
*
*   @details Contains the interrupt-based finite state machine, which could be
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

#define TWI_BITRATE_REGISTER TWBR

#define TWI_CONTROL_REGISTER TWCR
#define TWI_INT_FLAG (1 << TWINT)
#define TWI_START_CONDITION (1 << TWSTA)
#define TWI_ACK_CONDITION (1 << TWEA)
#define TWI_STOP_CONDITION (1 << TWSTO)
#define TWI_ENABLE (1 << TWEN)
#define TWI_INT_ENABLE (1 << TWIE)

#define TWI_STATUS_REGISTER TWSR
#define TWI_DATA_REGISTER TWDR

#define TWI_STATES_NUMBER 7U

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
	twi_state_branch branches[TWI_STATES_NUMBER];
} twi_state;

typedef struct {
	uint8_t status_code;
	twi_errors error;
	twi_state states[TWI_STATES_NUMBER];
	twi_state current_state;
	void (*next_op_ptr)(void);
	uint16_t current_input;
	uint8_t sla_add;
	const uint8_t* tx_buff;
	uint8_t* rx_buff;
	uint8_t done;
} twi_state_machine;

static volatile twi_state_machine tsm = {0};

inline static void twi_operation_start(void) {
	TWI_CONTROL_REGISTER = TWI_ENABLE | TWI_INT_ENABLE | TWI_INT_FLAG;
}

inline static void twi_start_send(void) {
	TWI_CONTROL_REGISTER = TWI_START_CONDITION | TWI_ENABLE | TWI_INT_ENABLE | TWI_INT_FLAG;
}

inline static void twi_slaw_send(void) {
	TWI_DATA_REGISTER = SLA_W(tsm.sla_add);
	twi_operation_start();
}

inline static void twi_slar_send(void) {
	TWI_DATA_REGISTER = SLA_R(tsm.sla_add);
	twi_operation_start();
}

inline static void twi_data_send(void) {
	TWI_DATA_REGISTER = *(tsm.tx_buff);
	twi_operation_start();
}

inline static void twi_ack_send(void) {
	TWI_CONTROL_REGISTER = TWI_ACK_CONDITION | TWI_ENABLE | TWI_INT_ENABLE | TWI_INT_FLAG;
}

inline static void twi_stop_send(void) {
	TWI_CONTROL_REGISTER = TWI_STOP_CONDITION | TWI_ENABLE | TWI_INT_FLAG;
	tsm.done = 1;
}

inline static uint8_t twi_transmission_finished(void) {
	return (!(TWI_CONTROL_REGISTER & TWI_STOP_CONDITION)) ? 1 : 0;
}

/**
*
*	@brief	Advances the state machine to the next state.
*
*	@pre 	Every used state has at least one branch pointing to the next state within an input range
*	@pre 	All input ranges of a used states are non-intersecting intervals with each others
*	@post	The state machine is advanced to the state pointed by the branch with an input range which includes the current input
*
*/
static void twi_state_advance(void) {

	for (size_t i = 0; i < TWI_STATES_NUMBER - 1; i++) {
	
		if (tsm.current_input >= tsm.current_state.branches[i].input[0] && tsm.current_input <= tsm.current_state.branches[i].input[1]) {
			tsm.next_op_ptr = tsm.current_state.branches[i].next_op_ptr;
			tsm.current_state = tsm.states[tsm.current_state.branches[i].next_op];
			tsm.current_input++;
			break;
		}
		
		else if (i == TWI_STATES_NUMBER - 2 && tsm.current_input == tsm.current_state.branches[6].input[0]) {
			tsm.next_op_ptr = tsm.current_state.branches[6].next_op_ptr;
			break;
		}
	}
}

ISR(TWI_vect) {

	switch (tsm.current_state.current_op) {
	
		case START:
			
			if (TW_STATUS != TW_START) {
				tsm.status_code = TW_STATUS;
				tsm.error = TWI_ERR_START;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case REP_START:
			
			if (TW_STATUS != TW_REP_START) {
				tsm.status_code = TW_STATUS;
				tsm.error = TWI_ERR_REP_START;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case SLAW_SEND:
			
			if (TW_STATUS != TW_MT_SLA_ACK) {
				tsm.status_code = TW_STATUS;
				tsm.error = TWI_ERR_SLAW_SEND;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case SLAR_SEND:
			
			if (TW_STATUS != TW_MR_SLA_ACK) {
				tsm.status_code = TW_STATUS;
				tsm.error = TWI_ERR_SLAR_SEND;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			break;
			
		case DATA_SEND:
			
			if (TW_STATUS != TW_MT_DATA_ACK) {
				tsm.status_code = TW_STATUS;
				tsm.error = TWI_ERR_DATA_SEND;
				twi_stop_send();
				break;
			}
			
			twi_state_advance();
			tsm.tx_buff++;
			break;
			
		case ACK_SEND:
			
			if (TW_STATUS != TW_MR_DATA_ACK) {
				tsm.status_code = TW_STATUS;
				tsm.error = TWI_ERR_ACK_SEND;
				twi_stop_send();
				break;
			}
			
			*(tsm.rx_buff) = TWI_DATA_REGISTER;
			tsm.rx_buff++;
			twi_state_advance();
			break;
			
		case NACK_SEND:
			
			if (TW_STATUS != TW_MR_DATA_NACK) {
				tsm.status_code = TW_STATUS;
				tsm.error = TWI_ERR_NACK_SEND;
				twi_stop_send();
				break;
			}
			
			*(tsm.rx_buff) = TWI_DATA_REGISTER;
			tsm.rx_buff++;
			twi_state_advance();
			break;
		
		default:
			tsm.status_code = TW_STATUS;
			twi_stop_send();
	}
	
	if (tsm.error == TWI_ERR_OK && tsm.next_op_ptr != NULL) {
		tsm.next_op_ptr();
	}
}

inline static void twi_start_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {
	tsm.states[START].branches[next_op - 1].input[0] = from_input;
	tsm.states[START].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_rep_start_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {
	tsm.states[REP_START].branches[next_op - 1].input[0] = from_input;
	tsm.states[REP_START].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_slaw_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {	
	tsm.states[SLAW_SEND].branches[next_op - 1].input[0] = from_input;
	tsm.states[SLAW_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_slar_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {
	tsm.states[SLAR_SEND].branches[next_op - 1].input[0] = from_input;
	tsm.states[SLAR_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_data_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {	
	tsm.states[DATA_SEND].branches[next_op - 1].input[0] = from_input;
	tsm.states[DATA_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_ack_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {		
	tsm.states[ACK_SEND].branches[next_op - 1].input[0] = from_input;
	tsm.states[ACK_SEND].branches[next_op - 1].input[1] = to_input;
}

inline static void twi_nack_send_state_config(twi_operations next_op, uint16_t from_input, uint16_t to_input) {		
	tsm.states[NACK_SEND].branches[next_op - 1].input[0] = from_input;
	tsm.states[NACK_SEND].branches[next_op - 1].input[1] = to_input;
}

static void twi_state_machine_restore(void) {
	tsm.status_code = TW_NO_INFO;
	tsm.error = TWI_ERR_OK;
	
	tsm.states[START].current_op = START;
	tsm.states[REP_START].current_op = REP_START;
	tsm.states[SLAW_SEND].current_op = SLAW_SEND;
	tsm.states[SLAR_SEND].current_op = SLAR_SEND;
	tsm.states[DATA_SEND].current_op = DATA_SEND;
	tsm.states[ACK_SEND].current_op = ACK_SEND;
	tsm.states[NACK_SEND].current_op = NACK_SEND;
	
	for (size_t i = 0; i < TWI_STATES_NUMBER; i++) {
		tsm.states[i].branches[REP_START - 1].next_op = REP_START;
		tsm.states[i].branches[REP_START - 1].next_op_ptr = twi_start_send;
		tsm.states[i].branches[REP_START - 1].input[0] = 0;
		tsm.states[i].branches[REP_START - 1].input[1] = 0;
		
		tsm.states[i].branches[SLAW_SEND - 1].next_op = SLAW_SEND;
		tsm.states[i].branches[SLAW_SEND - 1].next_op_ptr = twi_slaw_send;
		tsm.states[i].branches[SLAW_SEND - 1].input[0] = 0;
		tsm.states[i].branches[SLAW_SEND - 1].input[1] = 0;
		
		tsm.states[i].branches[SLAR_SEND - 1].next_op = SLAR_SEND;
		tsm.states[i].branches[SLAR_SEND - 1].next_op_ptr = twi_slar_send;
		tsm.states[i].branches[SLAR_SEND - 1].input[0] = 0;
		tsm.states[i].branches[SLAR_SEND - 1].input[1] = 0;
		
		tsm.states[i].branches[DATA_SEND - 1].next_op = DATA_SEND;
		tsm.states[i].branches[DATA_SEND - 1].next_op_ptr = twi_data_send;
		tsm.states[i].branches[DATA_SEND - 1].input[0] = 0;
		tsm.states[i].branches[DATA_SEND - 1].input[1] = 0;
		
		tsm.states[i].branches[ACK_SEND - 1].next_op = ACK_SEND;
		tsm.states[i].branches[ACK_SEND - 1].next_op_ptr = twi_ack_send;
		tsm.states[i].branches[ACK_SEND - 1].input[0] = 0;
		tsm.states[i].branches[ACK_SEND - 1].input[1] = 0;
		
		tsm.states[i].branches[NACK_SEND - 1].next_op = NACK_SEND;
		tsm.states[i].branches[NACK_SEND - 1].next_op_ptr = twi_operation_start;
		tsm.states[i].branches[NACK_SEND - 1].input[0] = 0;
		tsm.states[i].branches[NACK_SEND - 1].input[1] = 0;
		
		tsm.states[i].branches[STOP - 1].next_op = STOP;
		tsm.states[i].branches[STOP - 1].next_op_ptr = twi_stop_send;
		tsm.states[i].branches[STOP - 1].input[0] = 0;
		tsm.states[i].branches[STOP - 1].input[1] = 0;
	}
	
	tsm.next_op_ptr = NULL;
	tsm.current_state = tsm.states[START];
	tsm.current_input = 1;
	tsm.sla_add = 0;
	tsm.tx_buff = NULL;
	tsm.rx_buff = NULL;
	tsm.done = 0;
}

static void twi_state_machine_boot(uint8_t bitrate, uint8_t bitrate_prsc, uint8_t sla_add, const uint8_t* tx_buff, uint8_t* rx_buff){
	
	TWI_BITRATE_REGISTER = bitrate;
	TWI_STATUS_REGISTER = bitrate_prsc;
	TWI_CONTROL_REGISTER = TWI_ENABLE;
	
	tsm.sla_add = sla_add;
	tsm.tx_buff = tx_buff;
	tsm.rx_buff = rx_buff;
	tsm.current_state = tsm.states[START];
	
	sei();
	twi_start_send();
}

twi_errors twi_master_transmitter(uint8_t bitrate, uint8_t bitrate_prsc, uint16_t bytes_num, uint8_t sla_add, const uint8_t* tx_buff) {
	uint8_t done = 0;
	uint16_t t_zero_ms = 0;

	if (tx_buff == NULL || bitrate_prsc > 3 || bytes_num == 0) {
		return TWI_ERR_PARAM;
	}

	twi_state_machine_restore();	
	twi_start_state_config(SLAW_SEND, 1, 1);	
	twi_slaw_send_state_config(DATA_SEND, 2, 2);	
	twi_data_send_state_config(DATA_SEND, 3, (bytes_num + 1));	
	twi_data_send_state_config(STOP, (bytes_num + 2), (bytes_num + 2));
	twi_state_machine_boot(bitrate, bitrate_prsc, sla_add, tx_buff, NULL);
	t_zero_ms = scheduler_timer_get_timestamp();
	
	while (1) {
		
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			done = tsm.done;
		}
		
		if (done && twi_transmission_finished()) {
		    break;
		}
		
		if (scheduler_timer_poll(&t_zero_ms, TWI_TIMEOUT_MS)) {
			twi_stop_send();
			return TWI_ERR_TIMEOUT;
		}
	}
	
	return tsm.error;
}

twi_errors twi_master_receiver(uint8_t bitrate, uint8_t bitrate_prsc, uint16_t bytes_num, uint8_t sla_add, uint8_t* rx_buff) {
	uint8_t done = 0;
	uint16_t t_zero_ms = 0;

	if (rx_buff == NULL || bitrate_prsc > 3 || bytes_num == 0) {
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
	t_zero_ms = scheduler_timer_get_timestamp();
	
	while (1) {
		
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			done = tsm.done;
		}
		
		if (done && twi_transmission_finished()) {
		    break;
		}
		
		if (scheduler_timer_poll(&t_zero_ms, TWI_TIMEOUT_MS)) {
			twi_stop_send();
			return TWI_ERR_TIMEOUT;
		}
	}
	
	return tsm.error;
}
