/**
*   
*   @file fan_curve.c
*
*   @brief Implementation for the fan_curve module.
*
*   @details Contains EEPROM and data validation logic.
*
*	Implements linear interpolation to compute the target speed.
*
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <avr/eeprom.h>
#include "fan_curve.h"
#include "crc.h"
#include "config.h"

#define MAX_SIZE 10U
#define MIN_TEMP_C 0
#define MAX_TEMP_C 999U
#define MIN_SPEED_RPM 0
#define MAX_SPEED_RPM 9999U

typedef struct {
    uint16_t temp_c;
    uint16_t speed_rpm;
} fan_curve_node;

typedef struct {
    uint8_t size;
    fan_curve_node nodes[MAX_SIZE];
    uint16_t crc;
} fan_curve;

static fan_curve ee_fan_curve EEMEM = {0};

static uint16_t crc_calculate(const fan_curve* curve_ptr) {
    return crc16_ccitt(offsetof(fan_curve, crc), (const uint8_t*) curve_ptr);
}

static inline void ee_curve_save(fan_curve* curve_ptr) {
	eeprom_update_block((const void*) curve_ptr, (void*) &(ee_fan_curve), sizeof(ee_fan_curve));
}

static inline void ee_curve_load(fan_curve* curve_ptr) {
	eeprom_read_block((void*) curve_ptr, (const void*) &(ee_fan_curve), sizeof(ee_fan_curve));
}

fan_curve_errors fan_curve_eeprom_update(uint8_t curve_size, const uint16_t* node_temp, const uint16_t* node_speed) {
	fan_curve loaded_curve = {0};
	fan_curve check_curve = {0};
	
	if (curve_size > MAX_SIZE || !curve_size || node_temp == NULL || node_speed == NULL) {
		return FAN_CURVE_ERR_PARAM;
	}
	
	check_curve.size = curve_size;
	
	for (size_t i = 0; i < curve_size; i++) {
		
		if (*(node_temp + i) > MAX_TEMP_C || *(node_speed + i) > MAX_SPEED_RPM) {
			return FAN_CURVE_ERR_PARAM;
		}
		
		if (i > 0) {
			
			if (*(node_temp + i) <= *(node_temp + i - 1) || *(node_speed + i) < *(node_speed + i - 1)) {
				return FAN_CURVE_ERR_PARAM;
			}
		}
		
		check_curve.nodes[i].temp_c = *(node_temp + i);
		check_curve.nodes[i].speed_rpm = *(node_speed + i);
	}
		
	check_curve.crc = crc_calculate(&check_curve);
	
	ee_curve_save(&check_curve);
	ee_curve_load(&loaded_curve);
	
	if (loaded_curve.crc != check_curve.crc || loaded_curve.crc != crc_calculate(&loaded_curve)) {
		return FAN_CURVE_ERR_EEPROM;
	}
	
	return FAN_CURVE_ERR_OK;
}

fan_curve_errors fan_curve_target_speed_compute(uint16_t temp_c, uint16_t* target_speed_rpm) {
	fan_curve loaded_curve = {0};
	uint32_t dx = 0;
	uint32_t dy = 0;
	uint32_t dT = 0;
	
	if (target_speed_rpm == NULL) {
		return FAN_CURVE_ERR_PARAM;
	}
	
	ee_curve_load(&loaded_curve);
	
	if (loaded_curve.crc != crc_calculate(&loaded_curve) || !(loaded_curve.size) || loaded_curve.size > MAX_SIZE) {
		return FAN_CURVE_ERR_EEPROM;
	}
	
	for (size_t i = 0; i < loaded_curve.size; i++) {
		
		if (loaded_curve.nodes[i].temp_c > MAX_TEMP_C || loaded_curve.nodes[i].speed_rpm > MAX_SPEED_RPM) {
			return FAN_CURVE_ERR_EEPROM;
		}
		
		if (i > 0) {
			
			if (loaded_curve.nodes[i].temp_c < loaded_curve.nodes[i - 1].temp_c || loaded_curve.nodes[i].speed_rpm < loaded_curve.nodes[i - 1].speed_rpm) {
				return FAN_CURVE_ERR_EEPROM;
			}
		}
	}
	
	if (loaded_curve.size == 1) {		
		*target_speed_rpm = loaded_curve.nodes[0].speed_rpm;
		return FAN_CURVE_ERR_OK;
	}
	
	for (size_t i = 0; i < loaded_curve.size - 1U; i++) {
		
		if (loaded_curve.nodes[i].temp_c == temp_c || (!i && temp_c <= loaded_curve.nodes[i].temp_c)) {
			*target_speed_rpm = loaded_curve.nodes[i].speed_rpm;
			break;
		}
		
		else if ((loaded_curve.nodes[i + 1].temp_c == temp_c) || (i == loaded_curve.size - 2U && temp_c >= loaded_curve.nodes[i + 1].temp_c)) {
			*target_speed_rpm = loaded_curve.nodes[i + 1].speed_rpm;
			break;
		}
		
		else if ((loaded_curve.nodes[i].temp_c < temp_c) && (loaded_curve.nodes[i + 1].temp_c > temp_c)) {
			dx = (loaded_curve.nodes[i + 1].temp_c - loaded_curve.nodes[i].temp_c);
			dy = (loaded_curve.nodes[i + 1].speed_rpm - loaded_curve.nodes[i].speed_rpm);
			dT = (temp_c - loaded_curve.nodes[i].temp_c);
			*target_speed_rpm = loaded_curve.nodes[i].speed_rpm + (uint16_t) ((dT * dy) / dx);
			break;
		}
	}
	
	return FAN_CURVE_ERR_OK;
}
