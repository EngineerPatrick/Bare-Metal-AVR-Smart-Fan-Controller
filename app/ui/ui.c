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
*	1 display for the output.
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

#define FAN_CURVE_MAX_SIZE 10U

#define TEMP_MIN_RATE 1U
#define TEMP_MAX_RATE 100U
#define SPEED_MIN_RATE 10U
#define SPEED_MAX_RATE 1000U

#define STANDARD_MODE 1
#define ADVANCED_MODE 2

static inline void rate_pin_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ((uint8_t) ~RATE_BUTTON_DIRECTION);
	D0_D7_DATA_REG |= RATE_BUTTON_PULLUP_R;
}

static inline void select_pin_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ((uint8_t) ~SELECT_BUTTON_DIRECTION);
	D0_D7_DATA_REG |= SELECT_BUTTON_PULLUP_R;
}

static inline void confirm_pin_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ((uint8_t) ~CONFIRM_BUTTON_DIRECTION);
	D0_D7_DATA_REG |= CONFIRM_BUTTON_PULLUP_R;
}

static inline void re_clk_pin_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ((uint8_t) ~ROT_ENC_CLK_DIRECTION);
	D0_D7_DATA_REG |= ROT_ENC_CLK_PULLUP_R;
}

static inline void re_dt_pin_init(void) {
	D0_D7_DATA_DIRECTION_REG &= ((uint8_t) ~ROT_ENC_DT_DIRECTION);
	D0_D7_DATA_REG |= ROT_ENC_DT_PULLUP_R;
}

static inline uint8_t rate_pin_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & RATE_BUTTON_INPUT)) ? 1 : 0;
}

static inline uint8_t select_pin_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & SELECT_BUTTON_INPUT)) ? 1 : 0;
}

static inline uint8_t confirm_pin_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & CONFIRM_BUTTON_INPUT)) ? 1 : 0;
}

static inline uint8_t re_clk_pin_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & ROT_ENC_CLK_INPUT)) ? 1 : 0;
}

static inline uint8_t re_dt_pin_state(void) {
	return (!(D0_D7_INPUT_PINS_REG & ROT_ENC_DT_INPUT)) ? 1 : 0;
}

static uint8_t pins_init_flag = 0;

static void pins_init(void) {
	
	if (pins_init_flag) {
		return;
	}

	rate_pin_init();
	select_pin_init();
	confirm_pin_init();
	re_clk_pin_init();
	re_dt_pin_init();
	
	pins_init_flag = 1;
}

static inline void rotary_enc_read(uint16_t* value, uint16_t rate, uint16_t min_value, uint16_t max_value) {
	
	if (re_dt_pin_state() != re_clk_pin_state()) {
		*(value) = (*(value) <= max_value - rate) ? *(value) + rate : *(value);
	}
	
	else {
		*(value) = (*(value) >= min_value + rate) ? *(value) - rate : *(value);
	}
}

static ui_errors standard_mode_config(void) {
	display_errors display_error_code = 0;
	fan_curve_errors fan_curve_error_code = 0;
	const uint16_t node_temp[FAN_CURVE_MAX_SIZE] = {UI_STD_TEMP_1, UI_STD_TEMP_2, UI_STD_TEMP_3, UI_STD_TEMP_4, UI_STD_TEMP_5, UI_STD_TEMP_6, UI_STD_TEMP_7, UI_STD_TEMP_8, UI_STD_TEMP_9, UI_STD_TEMP_10};
	uint16_t node_speed[FAN_CURVE_MAX_SIZE] = {FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM};
	size_t i = 0;
	uint8_t prev_re_value = 0;
	uint16_t prev_speed_rpm = FAN_DRIVER_MIN_SPEED_RPM;
	uint16_t rate = 0;
	blink_options option = SPEED_THIRD_DIGIT;
	uint8_t exit_flag = 0;
	
	while (!exit_flag) {
		
		if (i > 0) {
			node_speed[i] = (node_speed[i] < node_speed[i - 1]) ? node_speed[i - 1] : node_speed[i];
			prev_speed_rpm = node_speed[i - 1];
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
		display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_speed + i));
		
		while (1) {
			display_error_code = display_blink_state_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (re_clk_pin_state() != prev_re_value) {
				rotary_enc_read(node_speed + i, rate, prev_speed_rpm, FAN_DRIVER_MAX_SPEED_RPM);
				prev_re_value = re_clk_pin_state();
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			if (rate_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				option = (rate < SPEED_MAX_RATE) ? option - 1 : SPEED_THIRD_DIGIT;
				rate = (rate < SPEED_MAX_RATE) ? rate * 10 : SPEED_MIN_RATE;
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			if (confirm_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = 1;
				break;
			}
			
			if (select_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				
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
	
	fan_curve_error_code = fan_curve_eeprom_update(UI_STD_CURVE_SIZE, node_temp, node_speed);
	
	if (fan_curve_error_code != FAN_CURVE_ERR_OK) {
		return UI_ERR_FAN_CURVE;
	}
	
	return UI_ERR_OK;	
}

static ui_errors advanced_mode_config(void) {
	display_errors display_error_code = 0;
	fan_curve_errors fan_curve_error_code = 0;
	uint16_t node_temp[FAN_CURVE_MAX_SIZE] = {BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10, BME280_MIN_TEMP_C_X10};
	uint16_t node_speed[FAN_CURVE_MAX_SIZE] = {FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM, FAN_DRIVER_MIN_SPEED_RPM};
	size_t i = 0;
	uint8_t prev_re_value = 0;
	uint16_t prev_temp_c_x10 = BME280_MIN_TEMP_C_X10;
	uint16_t prev_speed_rpm = FAN_DRIVER_MIN_SPEED_RPM;
	uint16_t rate = 0;
	blink_options option = TEMP_THIRD_DIGIT;
	uint8_t exit_flag = 0;
	
	while (1) {
		
		if (i > 0) {
			node_temp[i] = (node_temp[i] <= node_temp[i - 1]) ? node_temp[i - 1] + 1 : node_temp[i];
			node_speed[i] = (node_speed[i] < node_speed[i - 1]) ? node_speed[i - 1] : node_speed[i];
			prev_temp_c_x10 = node_temp[i - 1] + 1;
			prev_speed_rpm = node_speed[i - 1];
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
		display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_temp + i));
		
		while (1) {
			display_error_code = display_blink_state_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (re_clk_pin_state() != prev_re_value) {
				rotary_enc_read(node_temp + i, rate, prev_temp_c_x10, BME280_MAX_TEMP_C_X10);
				prev_re_value = re_clk_pin_state();
				display_error_code = display_temp_write(*(node_temp + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_temp + i));
			}
			
			if (rate_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				option = (rate < TEMP_MAX_RATE) ? option - 1 : TEMP_THIRD_DIGIT;
				rate = (rate < TEMP_MAX_RATE) ? rate * 10 : TEMP_MIN_RATE;
				display_error_code = display_temp_write(*(node_temp + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_temp + i));
			}
			
			
			if (confirm_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = 1;
				break;
			}
			
			if (select_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				display_error_code = display_temp_write(*(node_temp + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}

				break;
			}
		}
		
		if (exit_flag) {
			break;
		}
		
		rate = SPEED_MIN_RATE;
		option = SPEED_THIRD_DIGIT;
		display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_speed + i));
		
		while (1) {
			display_error_code = display_blink_state_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (re_clk_pin_state() != prev_re_value) {
				rotary_enc_read(node_speed + i, rate, prev_speed_rpm, FAN_DRIVER_MAX_SPEED_RPM);
				prev_re_value = re_clk_pin_state();
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			if (rate_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				option = (rate < SPEED_MAX_RATE) ? option - 1 : SPEED_THIRD_DIGIT;
				rate = (rate < SPEED_MAX_RATE) ? rate * 10 : SPEED_MIN_RATE;
				display_error_code = display_speed_write(*(node_speed + i));
			
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
				
				display_digit_blink(DISPLAY_BLINK_TIME_MS, option, *(node_speed + i));
			}
			
			
			if (confirm_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = 1;
				break;
			}
			
			if (select_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				
				if (i < (FAN_CURVE_MAX_SIZE - 1)) {
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
		
		if (exit_flag) {
			break;
		}
	}
	
	for (i = 0; i < (FAN_CURVE_MAX_SIZE - 1); i++) {
		node_temp[i + 1] = (node_temp[i] >= node_temp[i + 1]) ? node_temp[i] + 1 : node_temp[i + 1];
		node_speed[i + 1] = (node_speed[i] > node_speed[i + 1]) ? node_speed[i] : node_speed[i + 1];
	}
		
	fan_curve_error_code = fan_curve_eeprom_update(FAN_CURVE_MAX_SIZE, node_temp, node_speed);
	
	if (fan_curve_error_code != FAN_CURVE_ERR_OK) {
		return UI_ERR_FAN_CURVE;
	}
	
	return UI_ERR_OK;		
}

ui_errors ui_system_config(void) {
	display_errors display_error_code = 0;
	ui_errors ui_error_code = 0;
	uint8_t config_mode = STANDARD_MODE;
	uint8_t exit_flag = 0;
	
	fan_driver_controller_stop();
	
	if (!pins_init_flag) {
		pins_init();
	}
	
	display_error_code = display_intro_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	display_error_code = display_standard_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	display_error_code = display_advanced_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	while (1) {
	
		display_word_blink(DISPLAY_BLINK_TIME_MS, STANDARD_WORD);
		
		while (config_mode == STANDARD_MODE) {
			display_error_code = display_blink_state_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (confirm_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = 1;
				break;
			}
			
			if (select_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				config_mode = ADVANCED_MODE;
				display_error_code = display_standard_write();
				
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
			}
			
			if (rate_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				return UI_ERR_OK;
			}
		}
			
		if (exit_flag) {
			break;
		}
			
		display_word_blink(DISPLAY_BLINK_TIME_MS, ADVANCED_WORD);
		
		while (config_mode == ADVANCED_MODE) {
			display_error_code = display_blink_state_switch();
			
			if (display_error_code != DISPLAY_ERR_OK) {
				return UI_ERR_DISPLAY;
			}
			
			if (confirm_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				exit_flag = 1;
				break;
			}
			
			if (select_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				config_mode = STANDARD_MODE;
				display_error_code = display_advanced_write();
				
				if (display_error_code != DISPLAY_ERR_OK) {
					return UI_ERR_DISPLAY;
				}
			}
			
			if (rate_pin_state()) {
				scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
				return UI_ERR_OK;
			}
		}
			
		if (exit_flag) {
			break;
		}
	}
	
	display_error_code = display_units_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	if (config_mode == STANDARD_MODE) {
		ui_error_code = standard_mode_config();
		
		if (ui_error_code != UI_ERR_OK) {
			return ui_error_code;
		}
	}
	
	else {
		ui_error_code = advanced_mode_config();
		
		if (ui_error_code != UI_ERR_OK) {
			return ui_error_code;
		}
	}
	
	return UI_ERR_OK;
}

ui_errors ui_system_runtime_loop(void) {
	display_errors display_error_code = 0;
	bme_errors bme_error_code = 0;
	fan_curve_errors fan_curve_error_code = 0;
	fan_driver_errors fan_driver_error_code = 0;
	int16_t measured_temp_c_x10 = 0;
	uint16_t measured_speed_rpm = 0;
	uint16_t target_speed_rpm = 0;
	uint16_t fan_boot_t_0_ms = 0;
	uint16_t speed_update_t_0_ms = 0;
	uint16_t temp_update_t_0_ms = 0;
	uint16_t display_update_t_0_ms = 0;
	uint8_t fan_boot_flag = 0;
	
	display_error_code = display_units_write();
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return UI_ERR_DISPLAY;
	}
	
	temp_update_t_0_ms = scheduler_timestamp_capture();
	speed_update_t_0_ms = scheduler_timestamp_capture();
	display_update_t_0_ms = scheduler_timestamp_capture();
	fan_driver_controller_boot();
	fan_boot_t_0_ms = scheduler_timestamp_capture();
	
	while(1) {
		
		if (scheduler_timer_elapsed(temp_update_t_0_ms, BME280_TEMP_UPDATE_TIME_MS)) {
			bme_error_code = bme280_temp_capture(&measured_temp_c_x10);
			
			if (bme_error_code != BME_ERR_OK) {
				bme280_stop();
				fan_driver_controller_stop();
				return UI_ERR_BME;
			}
				
			fan_curve_error_code = fan_curve_target_speed_compute((uint16_t) measured_temp_c_x10, &target_speed_rpm);
				
			if (fan_curve_error_code != FAN_CURVE_ERR_OK) {
				bme280_stop();
				fan_driver_controller_stop();
				return UI_ERR_FAN_CURVE;
			}
			
			temp_update_t_0_ms = scheduler_timestamp_capture();
		}
		
		if (!fan_boot_flag) {
			
			if (scheduler_timer_elapsed(fan_boot_t_0_ms, FAN_DRIVER_BOOT_DELAY_MS)) {
				fan_boot_flag = 1;
			}
		}
		
		else {
		
			if (scheduler_timer_elapsed(speed_update_t_0_ms, FAN_DRIVER_SPEED_UPDATE_TIME_MS)) {
				fan_driver_error_code = fan_driver_speed_measure(&measured_speed_rpm);
					
				if (fan_driver_error_code != FAN_DRIVER_ERR_OK) {
					bme280_stop();
					fan_driver_controller_stop();
					return UI_ERR_FAN_DRIVER;
				}
				
				speed_update_t_0_ms = scheduler_timestamp_capture();
			}
		
			fan_driver_controller_update(target_speed_rpm);
		}
		
		if (scheduler_timer_elapsed(display_update_t_0_ms, DISPLAY_UPDATE_TIME_MS)) {
		
			display_error_code = display_temp_write((uint16_t) measured_temp_c_x10);
		
			if (display_error_code != DISPLAY_ERR_OK) {
				bme280_stop();
				fan_driver_controller_stop();
				return UI_ERR_DISPLAY;
			}
			
			display_error_code = display_speed_write(measured_speed_rpm);
		
			if (display_error_code != DISPLAY_ERR_OK) {
				bme280_stop();
				fan_driver_controller_stop();
				return UI_ERR_DISPLAY;
			}
			
			display_update_t_0_ms = scheduler_timestamp_capture();
		}
			
		if (rate_pin_state()) {
			scheduler_timer_delay(UI_BUTTONS_DEBOUNCE_TIME_MS);
			bme_error_code = bme280_stop();
			fan_driver_controller_stop();
			
			if (bme_error_code != BME_ERR_OK) {
				return UI_ERR_BME;
			}
			
			break;
		}
	}
	
	return UI_ERR_OK;
}
