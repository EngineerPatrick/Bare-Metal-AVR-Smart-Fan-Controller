/**
*
*	@file config.h
*
*	@brief System configuration storage.
*
*	@details Configuration values for all modules of the system.
*
*	If parameters are changed to incorrect values a specific error
*	will be generated during compile-time.
*
*/

#ifndef CONFIG_H
#define CONFIG_H

/**TWI*/
/*
*
*		Bitrate and prescaler values for 100 kHz TWI/I2C communication for the ATmega328P/Arduino UNO R3 (see twi.h)
*
*/
#define TWI_BITRATE_REG 72U
#define TWI_BITRATE_PRESCALER 0

/*
*
*		Hardware timeout for the 2-wire interface
*
*		-With a 100 kHz communication it takes 10 us to send 1 bit
*
*		-The longest I2C communication of the firmware (ssd1306.c) requires the transmission of 12 bytes + 1 SLA+W byte, which lasts ~1040 us
*
*		-The longest possible I2C communication with using the twi.h API is 250 bytes + 1 SLA+W byte, which lasts ~20 ms
*
*		-The valid range is [10, 60]
*
*/
#define TWI_TIMEOUT_MS 10U

/**UI*/
/*
*
*		Fixed temperature values for the fan curve in the standard option
*
*		-The valid range for the curve size is [1, 10]
*
*		-The valid range for the temperatures is [BME280_MIN_TEMP_C_X10, BME280_MAX_TEMP_C_X10] (see below)
*
*/
#define UI_STD_CURVE_SIZE 8U
#define UI_STD_TEMP_1 200U
#define UI_STD_TEMP_2 250U
#define UI_STD_TEMP_3 300U
#define UI_STD_TEMP_4 350U
#define UI_STD_TEMP_5 400U
#define UI_STD_TEMP_6 500U
#define UI_STD_TEMP_7 600U
#define UI_STD_TEMP_8 650U
#define UI_STD_TEMP_9 0
#define UI_STD_TEMP_10 0


/*
*
*		Software-debounce for the input buttons
*
*		-The valid range is [100, 500]
*
*/
#define UI_BUTTONS_DEBOUNCE_TIME_MS 250U

/**DISPLAY*/
/*
*
*		Position of every word and digit	 on the display
*
*		-Every position must be set so that every character fits within an 8x16 grid
*
*/
#define DISPLAY_FIRST_ROW 3U
#define DISPLAY_FIRST_COLUMN 3U

#define DISPLAY_SELECT_WORD_ROW DISPLAY_FIRST_ROW
#define DISPLAY_SELECT_WORD_COLUMN DISPLAY_FIRST_COLUMN
#define DISPLAY_MODE_WORD_ROW DISPLAY_SELECT_WORD_ROW
#define DISPLAY_MODE_WORD_COLUMN DISPLAY_SELECT_WORD_COLUMN + 7
#define DISPLAY_STANDARD_WORD_ROW DISPLAY_FIRST_ROW + 3
#define DISPLAY_STANDARD_WORD_COLUMN DISPLAY_FIRST_COLUMN
#define DISPLAY_ADVANCED_WORD_ROW DISPLAY_FIRST_ROW + 5
#define DISPLAY_ADVANCED_WORD_COLUMN DISPLAY_FIRST_COLUMN

#define DISPLAY_NODE_WORD_ROW DISPLAY_FIRST_ROW - 2
#define DISPLAY_NODE_WORD_COLUMN DISPLAY_FIRST_COLUMN
#define DISPLAY_NODE_DIGITS_ROW DISPLAY_NODE_WORD_ROW
#define DISPLAY_NODE_DIGITS_COLUMN DISPLAY_NODE_WORD_COLUMN + 6

#define DISPLAY_TEMP_WORD_ROW DISPLAY_FIRST_ROW
#define DISPLAY_TEMP_WORD_COLUMN DISPLAY_FIRST_COLUMN
#define DISPLAY_TEMP_DIGITS_ROW DISPLAY_TEMP_WORD_ROW
#define DISPLAY_TEMP_DIGITS_COLUMN DISPLAY_TEMP_WORD_COLUMN + 8

#define DISPLAY_SPEED_WORD_ROW DISPLAY_FIRST_ROW + 3
#define DISPLAY_SPEED_WORD_COLUMN DISPLAY_FIRST_COLUMN
#define DISPLAY_SPEED_DIGITS_ROW DISPLAY_SPEED_WORD_ROW
#define DISPLAY_SPEED_DIGITS_COLUMN DISPLAY_SPEED_WORD_COLUMN + 7

#define DISPLAY_ERR_WORD_ROW DISPLAY_FIRST_ROW + 1
#define DISPLAY_ERR_WORD_COLUMN DISPLAY_FIRST_COLUMN
#define DISPLAY_ERR_DIGITS_ROW DISPLAY_ERR_WORD_ROW
#define DISPLAY_ERR_DIGITS_COLUMN DISPLAY_ERR_WORD_COLUMN + 5

/*
*
*		Blinking time for the UI
*
*		-The valid range is [400, 500]
*
*/
#define DISPLAY_BLINK_TIME_MS 400U

/*
*
*		Update time for the display
*
*		-The valid range is [15, 50]
*
*/
#define DISPLAY_UPDATE_TIME_MS 30U

/**BME280*/
/*
*
*		Maximum and minimum temperature values
*
*		-The manufacturer provides the maximum range [-40, 85] and the ideal range [0, 65] of degrees Celsius
*
*		-The value format here is tenth of Celsius degrees multiplied by 10: 12.3°C = 123°C
*
*		-The valid range is [0, 850] and must be bigger than 100
*
*/
#define BME280_MIN_TEMP_C_X10 0
#define BME280_MAX_TEMP_C_X10 650

/*
*
*		Update time for the temperature reading from the sensor and the target speed computation
*
*		-The sensor is configured to perform ~26 Hz temperature live readings
*
*		-The valid range is [10, 35]
*
*/
#define BME280_TEMP_UPDATE_TIME_MS 10U

/**FAN_DRIVER*/
/*
*
*		Minimum duty-cycle register value to spin the fan
*
*		-This value should be measured with fan_driver_speed_test
*
*		-The valid range is [0, 40] (half range: the reg top value is 79)
*
*/
#define FAN_DRIVER_MIN_DC_REG_VALUE 2U
/*
*	
*	  	Approximate MIN-MAX fan speed values calculated from the tachometer 	pulses measured using the input capture feature of Timer1
*
*		-Should be measured by using fan_driver_speed_test
*
*		-Keep this range slightly tighter than the measured one
*
*		-The valid range is [200, 4000] and must be bigger than 1000
*		
*/
#define FAN_DRIVER_MIN_SPEED_RPM 290U
#define FAN_DRIVER_MAX_SPEED_RPM 2000U

/*
*
*		Initial delay with the PWM at 50% duty-cycle to overcome inertia before using the tachometer hardware timeout
*
*		-The valid range is [500, 1500]
*
*/
#define FAN_DRIVER_BOOT_DELAY_MS 1500U

/*
*
*		Update time of the speed readings
*
*		-Should be higher than the maximum possible time between pulses calculated with the slowest speed: t = 1000 / (RPM * 2 / 60)
*
*		-The valid range is [300, 600]
*
*/
#define FAN_DRIVER_SPEED_UPDATE_TIME_MS 300U

/*
*
*		Approximate time to update the speed from its minimum to its maximum value and vice versa
*
*		-Should be measured by using fan_driver_update_delay_test
*
*		-The valid range is [1000, 15000]
*
*/
#define FAN_DRIVER_MAX_SPEED_UPDATE_DELAY_MS 10000

/*
*
*		Time in which the speed must be within a tight range for it to be considered stable
*
*		-This value must be at least 6 times the value of FAN_DRIVER_SPEED_UPDATE_TIME_MS to guarantee 5 temperature readings within this interval
*
*		-The valid range is [2000, 4000]
*
*/
#define FAN_DRIVER_SPEED_STABILITY_TIME_MS 2000

/*FAULT_MANAGER*/
/*
*
*		Duration of the buzzer sound
*
*		-The valid range is [1, 65]
*
*/
#define FAULT_MANAGER_BUZZER_DURATION_S 20U


/*
*
*		Control logic to generate compile-time errors for incorrect values
*
*/	

/*TWI*/
#if (TWI_BITRATE_REG < 0 || TWI_BITRATE_REG > 255)

	#error "The configured value for the bitrate register is out of range"
	
#elif (TWI_BITRATE_PRESCALER < 0 || TWI_BITRATE_PRESCALER > 3)

	#error "The configured value for the bitrate prescaler register is out of range"
	
#elif (TWI_TIMEOUT_MS < 10 || TWI_TIMEOUT_MS > 60)

	#error "The configured value for the hardware timeout of the TWI is out of range"
	
#endif

/*UI*/
#if (UI_STD_CURVE_SIZE < 1 || UI_STD_CURVE_SIZE > 10)

	#error "The configured size for the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE >= 1 && (UI_STD_TEMP_1 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_1 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 1 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 1 && (UI_STD_TEMP_1 > UI_STD_TEMP_2 || UI_STD_TEMP_2 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 2 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 2 && (UI_STD_TEMP_2 > UI_STD_TEMP_3 || UI_STD_TEMP_3 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 3 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 3 && (UI_STD_TEMP_3 > UI_STD_TEMP_4 || UI_STD_TEMP_4 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 4 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 4 && (UI_STD_TEMP_4 > UI_STD_TEMP_5 || UI_STD_TEMP_5 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 5 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 5 && (UI_STD_TEMP_5 > UI_STD_TEMP_6 || UI_STD_TEMP_6 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 6 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 6 && (UI_STD_TEMP_6 > UI_STD_TEMP_7 || UI_STD_TEMP_7 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 7 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 7 && (UI_STD_TEMP_7 > UI_STD_TEMP_8 || UI_STD_TEMP_8 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 8 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 8 && (UI_STD_TEMP_8 > UI_STD_TEMP_9 || UI_STD_TEMP_9 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 9 of the standard curve is out of range"
	
#elif (UI_STD_CURVE_SIZE > 9 && (UI_STD_TEMP_9 > UI_STD_TEMP_10 || UI_STD_TEMP_10 > BME280_MAX_TEMP_C_X10))

	#error "The configured temperature for node 10 of the standard curve is out of range"
	
#elif (UI_BUTTONS_DEBOUNCE_TIME_MS < 100 || UI_BUTTONS_DEBOUNCE_TIME_MS > 500)

	#error "The configured value for the buttons software-debounce is out of range"

#endif

/*DISPLAY*/
#if (DISPLAY_SELECT_WORD_ROW < 1 || DISPLAY_SELECT_WORD_ROW > 8)

	#error "The configured row for the SELECT word is out of range"
	
#elif (DISPLAY_SELECT_WORD_COLUMN < 1 || DISPLAY_SELECT_WORD_COLUMN > 11)

	#error "The configured column for the SELECT word is out of range"

#elif (DISPLAY_MODE_WORD_ROW < 1 || DISPLAY_MODE_WORD_ROW > 8)

	#error "The configured row for the MODE word is out of range"
	
#elif (DISPLAY_MODE_WORD_COLUMN < 1 || DISPLAY_MODE_WORD_COLUMN > 13)

	#error "The configured column for the MODE word is out of range"

#elif (DISPLAY_STANDARD_WORD_ROW < 1 || DISPLAY_STANDARD_WORD_ROW > 8)

	#error "The configured row for the STANDARD word is out of range"
	
#elif (DISPLAY_STANDARD_WORD_COLUMN < 1 || DISPLAY_STANDARD_WORD_COLUMN > 9)

	#error "The configured column for the STANDARD word is out of range"

#elif (DISPLAY_ADVANCED_WORD_ROW < 1 || DISPLAY_ADVANCED_WORD_ROW > 8)

	#error "The configured row for the ADVANCED word is out of range"
	
#elif (DISPLAY_ADVANCED_WORD_COLUMN < 1 || DISPLAY_ADVANCED_WORD_COLUMN > 9)

	#error "The configured column for the ADVANCED word is out of range"

#elif (DISPLAY_NODE_WORD_ROW < 1 || DISPLAY_NODE_WORD_ROW > 8)

	#error "The configured row for the NODE word is out of range"
	
#elif (DISPLAY_NODE_WORD_COLUMN < 1 || DISPLAY_NODE_WORD_COLUMN > 13)

	#error "The configured column for the NODE word is out of range"

#elif (DISPLAY_NODE_DIGITS_ROW < 1 || DISPLAY_NODE_DIGITS_ROW > 8)

	#error "The configured row for the node digits is out of range"
	
#elif (DISPLAY_NODE_DIGITS_COLUMN < 1 || DISPLAY_NODE_DIGITS_COLUMN > 15)

	#error "The configured column for the node digits is out of range"

#elif (DISPLAY_TEMP_WORD_ROW < 1 || DISPLAY_TEMP_WORD_ROW > 8)

	#error "The configured row for the TEMP word is out of range"
	
#elif (DISPLAY_TEMP_WORD_COLUMN < 1 || DISPLAY_TEMP_WORD_COLUMN > 3)

	#error "The configured column for the TEMP word is out of range"

#elif (DISPLAY_TEMP_DIGITS_ROW < 1 || DISPLAY_TEMP_DIGITS_ROW > 8)

	#error "The configured row for the temperature digits is out of range"
	
#elif (DISPLAY_TEMP_DIGITS_COLUMN < 1 || DISPLAY_TEMP_DIGITS_COLUMN > 11)

	#error "The configured column for the temperature digits is out of range"

#elif (DISPLAY_SPEED_WORD_ROW < 1 || DISPLAY_SPEED_WORD_ROW > 8)

	#error "The configured row for the SPEED word is out of range"
	
#elif (DISPLAY_SPEED_WORD_COLUMN < 1 || DISPLAY_SPEED_WORD_COLUMN > 3)

	#error "The configured column for the SPEED word is out of range"

#elif (DISPLAY_SPEED_DIGITS_ROW < 1 || DISPLAY_SPEED_DIGITS_ROW > 8)

	#error "The configured row for the speed digits is out of range"
	
#elif (DISPLAY_SPEED_DIGITS_COLUMN < 1 || DISPLAY_SPEED_DIGITS_COLUMN > 10)

	#error "The configured column for the speed digits is out of range"

#elif (DISPLAY_ERR_WORD_ROW < 1 || DISPLAY_ERR_WORD_ROW > 8)

	#error "The configured row for the ERR word is out of range"
	
#elif (DISPLAY_ERR_WORD_COLUMN < 1 || DISPLAY_ERR_WORD_COLUMN > 14)

	#error "The configured column for the ERR word is out of range"

#elif (DISPLAY_ERR_DIGITS_ROW < 1 || DISPLAY_ERR_DIGITS_ROW > 8)

	#error "The configured row for the error digits is out of range"
	
#elif (DISPLAY_ERR_DIGITS_COLUMN < 1 || DISPLAY_ERR_DIGITS_COLUMN > 16)

	#error "The configured column for the error digits is out of range"
	
#elif (DISPLAY_BLINK_TIME_MS < 400 || DISPLAY_BLINK_TIME_MS > 500)

	#error "The configured value for the blinking time is out of range"
	
#elif (DISPLAY_UPDATE_TIME_MS < 15 || DISPLAY_UPDATE_TIME_MS > 50)

	#error "The configured value for the display update time is out of range"

#endif

/*BME280*/
#if (BME280_MIN_TEMP_C_X10 < 0 || BME280_MIN_TEMP_C_X10 > 840)

	#error "The configured minimum value for the temperature is out of range"
	
#elif (BME280_MAX_TEMP_C_X10 < 10 || BME280_MAX_TEMP_C_X10 > 850)

	#error "The configured maximum value for the temperature is out of range"
	
#elif BME280_MIN_TEMP_C_X10 > BME280_MAX_TEMP_C_X10

	#error "The configured minimum value for the temperature is higher than the configured maximum value"
	
#elif (BME280_MAX_TEMP_C_X10 - BME280_MIN_TEMP_C_X10) < 100

	#error "The configured temperature range is smaller than 100"
	
#elif (BME280_TEMP_UPDATE_TIME_MS < 10 || BME280_TEMP_UPDATE_TIME_MS > 35)

	#error "The configured value for the sensor update time is out of range"

#endif

/*FAN_DRIVER*/

#if (FAN_DRIVER_MIN_DC_REG_VALUE < 0 || FAN_DRIVER_MIN_DC_REG_VALUE > 40)

	#error "The configured minimum value of the duty-cycle register is out of range"
	
#elif (FAN_DRIVER_MIN_SPEED_RPM < 200 || FAN_DRIVER_MIN_SPEED_RPM > 3000)

	#error "The configured minimum value of the speed is out of range"
	
#elif (FAN_DRIVER_MAX_SPEED_RPM < 1200 || FAN_DRIVER_MAX_SPEED_RPM > 4000)

	#error "The configured maximum value of the speed is out of range"
	
#elif FAN_DRIVER_MIN_SPEED_RPM > FAN_DRIVER_MAX_SPEED_RPM

	#error "The configured minimum value for the speed is higher than the configured maximum value"
	
#elif (FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM) < 1000

	#error "The configured speed range is smaller than 1000"
	
#elif (FAN_DRIVER_BOOT_DELAY_MS < 500 || FAN_DRIVER_BOOT_DELAY_MS > 1500)

	#error "The configured value for the fan boot delay is out of range"
	
#elif (FAN_DRIVER_SPEED_UPDATE_TIME_MS < 300 || FAN_DRIVER_SPEED_UPDATE_TIME_MS > 600 || FAN_DRIVER_SPEED_UPDATE_TIME_MS < (1000 * 60) / (FAN_DRIVER_MIN_SPEED_RPM * 2))

	#error "The configured value for the update time of the speed readings is out of range"
	
#elif (FAN_DRIVER_MAX_SPEED_UPDATE_DELAY_MS < 1000 || FAN_DRIVER_MAX_SPEED_UPDATE_DELAY_MS > 15000)

	#error "The configured value for the maximum speed update time is out of range"
	
#elif (FAN_DRIVER_SPEED_STABILITY_TIME_MS < 2000 || FAN_DRIVER_SPEED_STABILITY_TIME_MS > 4000)

	#error "The configured value for the stability time is out of range"
	
#elif FAN_DRIVER_SPEED_STABILITY_TIME_MS < FAN_DRIVER_SPEED_UPDATE_TIME_MS * 6

	#error "The configured value for the stability time is less than 6 times the value of the time for the speed reading update"
	
#endif

/*FAULT_MANAGER*/
#if (FAULT_MANAGER_BUZZER_DURATION_S < 1 || FAULT_MANAGER_BUZZER_DURATION_S > 65)

	#error "The configured duration for the buzzer sound is out of range"
	
#endif


#endif
