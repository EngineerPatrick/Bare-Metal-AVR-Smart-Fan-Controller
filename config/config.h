/**
*
*	@file config.h
*
*	@brief System configuration storage.
*
*	@details Configuration values for all modules of the system.
*
*	If incorrect changes are made the system will return a
*	specific error code.
*
*/

#ifndef CONFIG_H
#define CONFIG_H

/**TWI*/
/*
*
*		Bitrate and prescaler values for 100 kHz for the ATmega328P/Arduino UNO R3 (see the frequency formula in twi.h)
*
*/
#define TWI_BITRATE_REG 72
#define TWI_BITRATE_PRESCALER 0
#define TWI_TIMEOUT_MS 400

/**UI*/
#define UI_STD_CURVE_SIZE 8
#define UI_STD_TEMP_1 200
#define UI_STD_TEMP_2 300
#define UI_STD_TEMP_3 400
#define UI_STD_TEMP_4 500
#define UI_STD_TEMP_5 600
#define UI_STD_TEMP_6 700
#define UI_STD_TEMP_7 800
#define UI_STD_TEMP_8 900
#define UI_STD_TEMP_9 000
#define UI_STD_TEMP_10 000

#define BUTTONS_DEBOUNCE_TIME_MS 250
#define BLINK_TIME_MS 400

/**DISPLAY*/
#define FIRST_ROW 3
#define FIRST_COLUMN 3

#define SELECT_WORD_ROW FIRST_ROW
#define SELECT_WORD_COLUMN FIRST_COLUMN
#define MODE_WORD_ROW SELECT_WORD_ROW
#define MODE_WORD_COLUMN SELECT_WORD_COLUMN + 7
#define STANDARD_WORD_ROW FIRST_ROW + 3
#define STANDARD_WORD_COLUMN FIRST_COLUMN
#define ADVANCED_WORD_ROW FIRST_ROW + 5
#define ADVANCED_WORD_COLUMN FIRST_COLUMN

#define NODE_WORD_ROW FIRST_ROW - 2
#define NODE_WORD_COLUMN FIRST_COLUMN
#define NODE_DIGITS_ROW NODE_WORD_ROW
#define NODE_DIGITS_COLUMN NODE_WORD_COLUMN + 6

#define TEMP_WORD_ROW FIRST_ROW
#define TEMP_WORD_COLUMN FIRST_COLUMN
#define TEMP_DIGITS_ROW TEMP_WORD_ROW
#define TEMP_DIGITS_COLUMN TEMP_WORD_COLUMN + 8

#define SPEED_WORD_ROW FIRST_ROW + 3
#define SPEED_WORD_COLUMN FIRST_COLUMN
#define SPEED_DIGITS_ROW SPEED_WORD_ROW
#define SPEED_DIGITS_COLUMN SPEED_WORD_COLUMN + 7

#define ERR_WORD_ROW FIRST_ROW + 1
#define ERR_WORD_COLUMN FIRST_COLUMN
#define ERR_DIGITS_ROW ERR_WORD_ROW
#define ERR_DIGITS_COLUMN ERR_WORD_COLUMN + 5

/**BME280*/
/*
*
*		The manufacturer provides the maximum range [-40, 85]°C and the ideal range [0, 65]°C
*
*		-The value format here is tenth of Celsius degrees multiplied by 10: 12.3°C = 123°C
*
*/
#define BME280_MIN_TEMP_C_X10 0
#define BME280_MAX_TEMP_C_X10 650

/**FAN_DRIVER*/
/*
*
*		Minimum duty-cycle register step
*
*		-Increase MIN_DC_REG_STEP to increase the speed variation during the feedback adjustment, which will finish faster but will be less smooth
*
*		-Decrease MIN_DC_REG_STEP to improve the responsiveness and the approximation of the target speed during the feedback adjustment, which will be slower
*
*		-The valid range is [1, 10]
*
*/
#define MIN_DC_REG_STEP 1
/*
*
*		Hysteresis correction factor to account for rounded decimals and non-liner speed
*
*		-The valid range is [15, 50]
*
*/
#define HYST_CORR_FACT_X1000 15
/*
*
*		Minimum duty-cycle register value to spin the fan tested with a 9 V DC supply
*
*		-The valid range is [0, 40] (half range: the reg top value is 79)
*
*/
#define MIN_DC_REG_VALUE 2
/*
*	
*	  	Approximate MIN-MAX fan speed values for a 9 V DC supply calculated from the tachometer
*	  	pulses measured using the input capture feature
*
*	 	-Tested duty-cycle range is [3.75%, 100%] (2 to 79 duty_cycle reg)
*
*		-The valid range must be bigger than 80
*
*		-Keep this range smaller than the real measured one to guarantee the reaching of the target speed
*
*/
#define FAN_DRIVER_MAX_SPEED_RPM 2000
#define FAN_DRIVER_MIN_SPEED_RPM 350

#define FAN_DRIVER_BOOT_DELAY_MS 1000
#define FAN_DRIVER_TIMEOUT_MS 400
#define FAN_DRIVER_UPDATE_TIME_MS 80

#define TACH_PULSES_PER_REV 2

/*FAULT_MANAGER*/
#define BUZZER_DURATION_S 20

#endif
