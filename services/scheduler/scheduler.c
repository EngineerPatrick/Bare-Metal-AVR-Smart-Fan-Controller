/**
*   
*   @file scheduler.c
*
*   @brief Implementation for the scheduler module.
*
*   @details Configures the timer registers and handles delay and
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
*	*Period*
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
#define TIM0_PRSC 3

#define TIM0_INT_MASK_REG TIMSK0
#define TIM0_OUT_COMP_INT (1 << OCIE0A)

#define TIM0_INT_FLAG_REG TIFR0
#define TIM0_OUT_COMP_FLAG (1 << OCF0A)

#define TIM0_TOP_REG OCR0A
#define TIM0_TOPVALUE 249

#define TIMER_RANGE_MS 65536

typedef struct {
	uint16_t current_ms;
	uint8_t init_flag;
} scheduler_timer;

static volatile scheduler_timer timer = {0};

ISR(TIMER0_COMPA_vect) {
	timer.current_ms++;
}

static inline void scheduler_tim0_ctc_init(void) {
	TIM0_CTRL_REG_A = TIM0_CLEAR_ON_COMP;
	TIM0_INT_MASK_REG = TIM0_OUT_COMP_INT;
	TIM0_TOP_REG = TIM0_TOPVALUE;
}

static inline void scheduler_tim0_prsc_init(void) {
	TIM0_CTRL_REG_B = TIM0_PRSC;
}

static inline void scheduler_tim0_stop(void) {
	TIM0_INT_MASK_REG = 0;
	TIM0_INT_FLAG_REG = TIM0_OUT_COMP_FLAG;
	TIM0_CTRL_REG_B = 0;
	TIM0_CTRL_REG_A = 0;
	TIM0_COUNT_REG = 0;
}

static void scheduler_timer_1ms_init(void) {
	scheduler_tim0_ctc_init();
	sei();
	timer.init_flag = 1;
}

void scheduler_timer_start(void) {

	if (!timer.init_flag) {
		scheduler_timer_1ms_init();
	}
	
	scheduler_tim0_prsc_init();
	sei();
}

void scheduler_timer_stop(void) {
	scheduler_tim0_stop();
	timer.current_ms = 0;
	timer.init_flag = 0;
}

uint16_t scheduler_timer_get_timestamp(void) {
	uint16_t timer_capture_ms = 0;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		timer_capture_ms = timer.current_ms;
	}
	
	return timer_capture_ms;
}

uint8_t scheduler_timer_poll(uint16_t* t_zero_ms, uint16_t target_duration_ms) {
	uint16_t timer_capture_ms = 0;
	
	if (!target_duration_ms || t_zero_ms == NULL || !timer.init_flag) {
		return 0;
	}
	
	timer_capture_ms = scheduler_timer_get_timestamp();
	
	if (timer_capture_ms > *t_zero_ms) {
	
		if (timer_capture_ms - *t_zero_ms >= target_duration_ms) {
			*t_zero_ms = timer_capture_ms;
			return 1;
		}
	}
	
	else if (timer_capture_ms < *t_zero_ms) {
	
		if ((TIMER_RANGE_MS - *t_zero_ms) + timer_capture_ms >= target_duration_ms) {
			*t_zero_ms = timer_capture_ms;
			return 1;
		}
	}
	
	return 0;
}

void scheduler_timer_delay(uint16_t target_duration_ms) {
	uint16_t timer_capture_ms = scheduler_timer_get_timestamp();
	
	if (target_duration_ms && timer.init_flag) {
		
		while (!scheduler_timer_poll(&timer_capture_ms, target_duration_ms)) {}
	}
}
