/**
*   
*   @file scheduler.c
*
*   @brief Implementation for the scheduler module.
*
*   @details Configures Timer0 registers and handles delay and
*	polling logic.
*
*   Integrates the util/atomic library of Avr-libC for both
*	polling of volatile variable and delay creation.
*
*/

/*
*
*	For the ATmega328P using Timer0 in CTC mode:
*
*	f = (CPU Clock Frequency) / (prescaler * (1 + top))
*
*	0 <= top value <= 255 (8-bit register)
*
*	prescaler register value -> prescaler value: 1 -> 1, 2 -> 8, 3 -> 64, 4 -> 256, 5 -> 1024
*
*	For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz
*
*	*Tick period*
*	prescaler value = 64, top value = 249 -> 1 ms
*
*/

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "scheduler.h"
#include "config.h"

#define TIM0_COUNT_REG TCNT0

#define TIM0_CTRL_REG_A TCCR0A
#define TIM0_CLEAR_ON_COMP (1 << WGM01)

#define TIM0_CTRL_REG_B TCCR0B
#define TIM0_PRSC 3U

#define TIM0_INT_MASK_REG TIMSK0
#define TIM0_OUT_COMP_INT (1 << OCIE0A)

#define TIM0_INT_FLAG_REG TIFR0
#define TIM0_OUT_COMP_FLAG (1 << OCF0A)

#define TIM0_TOP_REG OCR0A
#define TIM0_TOPVALUE 249U

#define TIMER_RANGE_MS 65536UL

typedef struct {
	uint8_t init_flag;
	uint8_t boot_flag;
} timer_state;

typedef struct {
	timer_state state;
	volatile uint16_t current_ms;
} timer_controller;

static timer_controller scheduler_controller = {0};

ISR(TIMER0_COMPA_vect) {
	scheduler_controller.current_ms++;
}

static inline void tim0_mode_config(void) {
	TIM0_CTRL_REG_A = TIM0_CLEAR_ON_COMP;
	TIM0_TOP_REG = TIM0_TOPVALUE;
	TIM0_INT_FLAG_REG = 0xFF;
	TIM0_COUNT_REG = 0;
	TIM0_INT_MASK_REG = TIM0_OUT_COMP_INT;
}

static inline void tim0_boot(void) {
	TIM0_CTRL_REG_B = TIM0_PRSC;
}

static inline void tim0_stop(void) {
	TIM0_INT_MASK_REG = 0;
	TIM0_INT_FLAG_REG = TIM0_OUT_COMP_FLAG;
	TIM0_CTRL_REG_B = 0;
	TIM0_CTRL_REG_A = 0;
	TIM0_TOP_REG = 0;
	TIM0_COUNT_REG = 0;
}

static void timer_init(void) {

	if (scheduler_controller.state.init_flag) {
		return;
	}
	
	tim0_mode_config();	
	scheduler_controller.state.init_flag = 1;
}

void scheduler_timer_boot(void) {
	
	if (scheduler_controller.state.boot_flag) {
		return;
	}
	
	if (!scheduler_controller.state.init_flag) {
		timer_init();
	}
	
	tim0_boot();
	scheduler_controller.state.boot_flag = 1;
}

void scheduler_timer_stop(void) {

	if (!scheduler_controller.state.boot_flag) {
		return;
	}
	
	tim0_stop();
	scheduler_controller.current_ms = 0;
	scheduler_controller.state.init_flag = 0;
	scheduler_controller.state.boot_flag = 0;
}

uint16_t scheduler_timestamp_capture(void) {
	uint16_t captured_timestamp_ms = 0;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		captured_timestamp_ms = scheduler_controller.current_ms;
	}
	
	return captured_timestamp_ms;
}

uint8_t scheduler_timer_elapsed(uint16_t t_0_ms, uint16_t target_duration_ms) {
	uint16_t captured_timestamp_ms = 0;
	
	if (!target_duration_ms || !scheduler_controller.state.boot_flag) {
		return 0;
	}
	
	captured_timestamp_ms = scheduler_timestamp_capture();
	
	if (captured_timestamp_ms > t_0_ms) {
	
		if (captured_timestamp_ms - t_0_ms >= target_duration_ms) {
			return 1;
		}
	}
	
	else if (captured_timestamp_ms < t_0_ms) {
	
		if ((TIMER_RANGE_MS - t_0_ms) + captured_timestamp_ms >= target_duration_ms) {
			return 1;
		}
	}
	
	return 0;
}

void scheduler_timer_delay(uint16_t target_duration_ms) {
	uint16_t captured_timestamp_ms = scheduler_timestamp_capture();
	
	if (target_duration_ms && scheduler_controller.state.boot_flag) {
		
		while (!scheduler_timer_elapsed(captured_timestamp_ms, target_duration_ms)) {}
	}
}
