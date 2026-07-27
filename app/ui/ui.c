/**
*   
*   @file ui.c
*
*   @brief Implementation for the ui module.
*
*   @details Contains UI logic for the configuration of the system
*	and the runtime loop.
*
*	Handles 3 buttons and 1 rotary encoder for the input and
*	a display for the output.
*
*/

#include <stdint.h>
#include "ui.h"
#include "display.h"
#include "fan_curve.h"
#include "fan_driver.h"
#include "bme280.h"
#include "scheduler.h"
#include "config.h"
#include "pin_map.h"
#include "board.h"

#define STD_FAN_CURVE_MAX_SIZE 10
#define ADV_FAN_CURVE_MAX_SIZE 10

#define TEMP_MIN_RATE 1
#define TEMP_MAX_RATE 100
#define SPEED_MIN_RATE 10
#define SPEED_MAX_RATE 1000
#define EXIT 99
#define STANDARD 1
#define ADVANCED 2

static inline void ui_rate_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ~RATE_BUTTON_DIRECTION;
	D0_D7_DATA_REG |= RATE_BUTTON_PULLUP_R;
}

static inline void ui_select_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ~SELECT_BUTTON_DIRECTION;
	D0_D7_DATA_REG |= SELECT_BUTTON_PULLUP_R;
}

static inline void ui_confirm_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ~CONFIRM_BUTTON_DIRECTION;
	D0_D7_DATA_REG |= CONFIRM_BUTTON_PULLUP_R;
}

static inline void ui_re_clk_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ~ROT_ENC_CLK_DIRECTION;
	D0_D7_DATA_REG |= ROT_ENC_CLK_PULLUP_R;
}

static inline void ui_re_dt_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ~ROT_ENC_DT_DIRECTION;
	D0_D7_DATA_REG |= ROT_ENC_DT_PULLUP_R;
}

static inline uint8_t ui_rate_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & RATE_BUTTON_INPUT)) ? 1 : 0;
}

static inline uint8_t ui_select_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & SELECT_BUTTON_INPUT)) ? 1 : 0;
}

static inline uint8_t ui_confirm_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & CONFIRM_BUTTON_INPUT)) ? 1 : 0;
}

static inline uint8_t ui_re_clk_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & ROT_ENC_CLK_INPUT)) ? 1 : 0;
}

static inline uint8_t ui_re_dt_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & ROT_ENC_DT_INPUT)) ? 1 : 0;
}

static inline void ui_rotary_enc_handler(uint16_t* value, uint16_t rate, uint16_t min_value, uint16_t max_value) {
	
	if (ui_re_dt_state() != ui_re_clk_state()) {
		*(value) = (*(value) <= max_value - rate) ? *(value) + rate : *(value);
	}
	
	else {
		*(value) = (*(value) >= min_value + rate) ? *(value) - rate : *(value);
	}
}

static ui_errors ui_system_configure_std(void) {
	display_errors display_error_code = 0;
	fan_curve_errors fan_curve_error_code = 0;
	uint16_t node_temp[STD_FAN_CURVE_MAX_SIZE] = {UI_STD_TEMP_1, UI_STD_TEMP_2, UI_STD_TEMP_3, UI_STD_TEMP_4, UI_STD_TEMP_5, UI_STD_TEMP_6, UI_STD_TEMP_7, UI_STD_TEMP_8, UI_STD_TEMP_9, UI_STD_TEMP_10};
	uint16_t node_speed[STD_FAN_CURVE_MAX_SIZE] = {FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM};
	uint8_t i = 0;
	uint8_t prev_re_value = 0;
	uint16_t prev_speed_rpm = FAN_DRIVER_MIN_SPEED_RPM;
	uint16_t rate = 0;
	blink_options option = SPEED_THIRD_DIGIT;
	uint8_t exit_flag = 0;
	
	while (exit_flag != EXIT) {
		
		if (i > 0) {
			prev_speed_rpm = node_speed[i - 1];
			node_speed[i] = (node_speed[i] < node_speed[i - 1]) ? node_speed[i - 1] : node_speed[i];
		}
	
		display_error_code = display_index_write(i + 1);
	
		if (display_error_code != DISPLAY_ERR_OK) {
			return UI_ERR_DISPLAY;
		}
	
		display_error_code = display_temp_write(*(node_temp + i));
	
		if (display_error_code != DISPLAY_ERR_OK) {
			return UI_ERR_DISPLAY;
		}
		
		display_error_code = display_speed_write(*(node_speed + i));
			
		if (display_error_code != DISPLAY_ERR_OK) {
			return UI_ERR_DISPLAY;
		}
		
		rate = SPEED_MIN_RATE;
		option = SPEED_THIRD_DIGIT;
		display_digit_blink(BLINK_TIME_MS, option, *(node_speed + i));
		
		while (1) {
			display_error_code = display_blink_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (ui_re_clk_state() != prev_re_value) {
				ui_rotary_enc_handler(node_speed + i, rate, prev_speed_rpm, FAN_DRIVER_MAX_SPEED_RPM);
				prev_re_value = ui_re_clk_state();
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			if (ui_rate_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				option = (rate < SPEED_MAX_RATE) ? option - 1 : SPEED_THIRD_DIGIT;
				rate = (rate < SPEED_MAX_RATE) ? rate * 10 : SPEED_MIN_RATE;
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			if (ui_confirm_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = EXIT;
				break;
			}
			
			if (ui_select_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				
				if (i < (UI_STD_CURVE_SIZE - 1)) {
					i++;
					break;
				}
				
				else {
					prev_speed_rpm = FAN_DRIVER_MIN_SPEED_RPM;
					i = 0;
					break;
				}
			}
		}
	}
	
	for (i = 0; i < (UI_STD_CURVE_SIZE - 1); i++) {
		node_speed[i + 1] = (node_speed[i] > node_speed[i + 1]) ? node_speed[i] : node_speed[i + 1];
	}
	
	fan_curve_error_code = fan_curve_create(UI_STD_CURVE_SIZE, node_temp, node_speed);
	
	if (fan_curve_error_code != FAN_CURVE_ERR_OK) {
		return UI_ERR_FAN_CURVE;
	}
	
	return UI_ERR_OK;	
}

static ui_errors ui_system_configure_adv(void) {
	display_errors display_error_code = 0;
	fan_curve_errors fan_curve_error_code = 0;
	uint16_t node_temp[ADV_FAN_CURVE_MAX_SIZE] = {BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10};
	uint16_t node_speed[ADV_FAN_CURVE_MAX_SIZE] = {FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM};
	uint8_t i = 0;
	uint8_t prev_re_value = 0;
	uint16_t prev_temp_c_x10 = BME280_MIN_TEMP_C_X10;
	uint16_t prev_speed_rpm = FAN_DRIVER_MIN_SPEED_RPM;
	uint16_t rate = 0;
	blink_options option = TEMP_THIRD_DIGIT;
	uint8_t exit_flag = 0;
	
	while (1) {
		
		if (i > 0) {
			prev_temp_c_x10 = node_temp[i - 1];
			prev_speed_rpm = node_speed[i - 1];
			node_speed[i] = (node_speed[i] <= node_speed[i - 1]) ? node_speed[i - 1] : node_speed[i];
			node_temp[i] = (node_temp[i] <= node_temp[i - 1]) ? node_temp[i - 1] : node_temp[i];
		}
		
		display_error_code = display_index_write(i + 1);
			
		if (display_error_code != DISPLAY_ERR_OK) {
			return UI_ERR_DISPLAY;
		}
		
		display_error_code = display_temp_write(*(node_temp + i));
			
		if (display_error_code != DISPLAY_ERR_OK) {
			return UI_ERR_DISPLAY;
		}
		
		display_error_code = display_speed_write(*(node_speed + i));
			
		if (display_error_code != DISPLAY_ERR_OK) {
			return UI_ERR_DISPLAY;
		}
		
		rate = TEMP_MIN_RATE;
		option = TEMP_THIRD_DIGIT;
		display_digit_blink(BLINK_TIME_MS, option, *(node_temp + i));
		
		while (1) {
			display_error_code = display_blink_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (ui_re_clk_state() != prev_re_value) {
				ui_rotary_enc_handler(node_temp + i, rate, prev_temp_c_x10, BME280_MAX_TEMP_C_X10);
				prev_re_value = ui_re_clk_state();
				display_error_code = display_temp_write(*(node_temp + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(BLINK_TIME_MS, option, *(node_temp + i));
			}
			
			if (ui_rate_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				option = (rate < TEMP_MAX_RATE) ? option - 1 : TEMP_THIRD_DIGIT;
				rate = (rate < TEMP_MAX_RATE) ? rate * 10 : TEMP_MIN_RATE;
				display_error_code = display_temp_write(*(node_temp + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(BLINK_TIME_MS, option, *(node_temp + i));
			}
			
			
			if (ui_confirm_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = EXIT;
				break;
			}
			
			if (ui_select_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				display_error_code = display_temp_write(*(node_temp + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}

				break;
			}
		}
		
		if (exit_flag == EXIT) {
			break;
		}
		
		rate = SPEED_MIN_RATE;
		option = SPEED_THIRD_DIGIT;
		display_digit_blink(BLINK_TIME_MS, option, *(node_speed + i));
		
		while (1) {
			display_error_code = display_blink_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (ui_re_clk_state() != prev_re_value) {
				ui_rotary_enc_handler(node_speed + i, rate, prev_speed_rpm, FAN_DRIVER_MAX_SPEED_RPM);
				prev_re_value = ui_re_clk_state();
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			if (ui_rate_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				option = (rate < SPEED_MAX_RATE) ? option - 1 : SPEED_THIRD_DIGIT;
				rate = (rate < SPEED_MAX_RATE) ? rate * 10 : SPEED_MIN_RATE;
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			
			if (ui_confirm_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = EXIT;
				break;
			}
			
			if (ui_select_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				
				if (i < (ADV_FAN_CURVE_MAX_SIZE - 1)) {
					i++;
					break;
				}
				
				else {
					prev_speed_rpm = FAN_DRIVER_MIN_SPEED_RPM;
					prev_temp_c_x10 = BME280_MIN_TEMP_C_X10;
					i = 0;
					break;
				}
			}
		}
		
		if (exit_flag == EXIT) {
			break;
		}
	}
	
	for (i = 0; i < (ADV_FAN_CURVE_MAX_SIZE - 1); i++) {
		node_temp[i + 1] = (node_temp[i] > node_temp[i + 1]) ? node_temp[i] : node_temp[i + 1];
		node_speed[i + 1] = (node_speed[i] > node_speed[i + 1]) ? node_speed[i] : node_speed[i + 1];
	}
		
	fan_curve_error_code = fan_curve_create(ADV_FAN_CURVE_MAX_SIZE, node_temp, node_speed);
	
	if (fan_curve_error_code != FAN_CURVE_ERR_OK) {
		return UI_ERR_FAN_CURVE;
	}
	
	return UI_ERR_OK;		
}

ui_errors ui_system_configure(void) {
	display_errors display_error_code = 0;
	ui_errors ui_error_code = 0;
	uint8_t choice = STANDARD;
	uint8_t exit_flag = 0;
	
	fan_driver_boot();
	fan_driver_stop();
	
	ui_rate_init();
	ui_select_init();
	ui_confirm_init();
	ui_re_clk_init();
	ui_re_dt_init();
	
	display_error_code = display_intro_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	display_error_code = display_std_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	display_error_code = display_adv_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	while (1) {
	
		display_word_blink(BLINK_TIME_MS, STANDARD_WORD);
		
		while (choice == STANDARD) {
			display_error_code = display_blink_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (ui_confirm_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = EXIT;
				break;
			}
			
			if (ui_select_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				choice = ADVANCED;
				display_error_code = display_std_write();
				
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
			}
			
			if (ui_rate_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				return UI_ERR_OK;
			}
		}
			
		if (exit_flag == EXIT) {
			break;
		}
			
		display_word_blink(BLINK_TIME_MS, ADVANCED_WORD);
		
		while (choice == ADVANCED) {
			display_error_code = display_blink_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (ui_confirm_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = EXIT;
				break;
			}
			
			if (ui_select_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				choice = STANDARD;
				display_error_code = display_adv_write();
				
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
			}
			
			if (ui_rate_state()) {
				scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
				return UI_ERR_OK;
			}
		}
			
		if (exit_flag == EXIT) {
			break;
		}
	}
	
	display_error_code = display_units_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	if (choice == STANDARD) {
		ui_error_code = ui_system_configure_std();
		
		if (ui_error_code != UI_ERR_OK) {
			return ui_error_code;
		}
	}
	
	else {
		ui_error_code = ui_system_configure_adv();
		
		if (ui_error_code != UI_ERR_OK) {
			return ui_error_code;
		}
	}
	
	return UI_ERR_OK;
}

ui_errors ui_system_update(void) {
	display_errors display_error_code = 0;
	bme_errors bme_error_code = 0;
	fan_curve_errors fan_curve_error_code = 0;
	fan_driver_errors fan_driver_error_code = 0;
	int16_t temp_c_x10 = 0;
	uint16_t speed_rpm = 0;
	uint16_t t_zero_ms = 0;
	
	display_error_code = display_units_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	bme_error_code = bme280_temp_init();
	
	if (bme_error_code != BME_ERR_OK) {
		return UI_ERR_BME;
	}
	
	fan_driver_boot();
	t_zero_ms = scheduler_timer_get_timestamp();
	
	while(1) {
		bme_error_code = bme280_temp_read(&temp_c_x10);
	
		if (bme_error_code != BME_ERR_OK) {
			fan_driver_stop();
			return UI_ERR_BME;
		}
				
		fan_curve_error_code = fan_curve_get_speed((uint16_t) temp_c_x10, &speed_rpm);
	
		if (fan_curve_error_code != FAN_CURVE_ERR_OK) {
			fan_driver_stop();
			return UI_ERR_FAN_CURVE;
		}
		
		if (scheduler_timer_poll(&t_zero_ms, FAN_DRIVER_UPDATE_TIME_MS)) {
			fan_driver_error_code = fan_driver_update(speed_rpm);
			
			if (fan_driver_error_code != FAN_DRIVER_ERR_OK) {
				fan_driver_stop();
				return UI_ERR_FAN_DRIVER;
			}
			
			t_zero_ms = scheduler_timer_get_timestamp();
		}
		
		display_error_code = display_temp_write((uint16_t) temp_c_x10);
	
		if (display_error_code != DISPLAY_ERR_OK) {
			fan_driver_stop();
			return UI_ERR_DISPLAY;
		}
		
		display_error_code = display_speed_write(speed_rpm);
	
		if (display_error_code != DISPLAY_ERR_OK) {
			fan_driver_stop();
			return UI_ERR_DISPLAY;
		}
			
		if (ui_rate_state()) {
			scheduler_timer_delay(BUTTONS_DEBOUNCE_TIME_MS);
			fan_driver_stop();
			break;
		}
	}
	
	return UI_ERR_OK;
}
