/**
*   
*   @file display.c
*
*   @brief Implementation for the display module.
*
*   @details Contains the pattern bitmaps generated using
*	the 8x8_bin_to_hex tool.
*
*   Handles digit extraction, character positioning and blinking.
*
*/

#include <stdint.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include "display.h"
#include "ssd1306.h"
#include "scheduler.h"
#include "config.h"

#define CHAR_PATTERN_BYTES 8U
#define SELECT_WORD_LENGTH 6U
#define MODE_WORD_LENGTH 4U
#define STANDARD_WORD_LENGTH 8U
#define ADVANCED_WORD_LENGTH 8U
#define TEMP_WORD_LENGTH 14U
#define SPEED_WORD_LENGTH 14U
#define NODE_WORD_LENGTH 4U
#define ERR_WORD_LENGTH 3U

#define MIN_INDEX 1U
#define MAX_INDEX 10U

/*
*
*	The value format is in tenth of degrees Celsius : 12.3°C = 123
*
*/
#define MIN_TEMP_C_X10 0
#define MAX_TEMP_C_X10 999U
#define TEMP_DIGITS 3U

#define MIN_SPEED_RPM 0
#define MAX_SPEED_RPM 9999U
#define SPEED_DIGITS 4U

/*
*
*	{
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
static const uint8_t A_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x60, 0x18, 0x16, 0x11, 0x16, 0x18, 0x60, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 1, 1, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{0, 0, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
static const uint8_t C_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x1C, 0x22, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 0, 0, 0},
*		{1, 0, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 1, 0, 0},
*		{1, 1, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t D_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x41, 0x41, 0x41, 0x41, 0x22, 0x1C, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t E_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t L_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 0, 0, 0, 1, 1, 0},
*		{1, 0, 1, 0, 1, 0, 1, 0},
*		{1, 0, 0, 1, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t M_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x02, 0x04, 0x08, 0x04, 0x02, 0x7F, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 0, 0, 0, 0, 1, 0},
*		{1, 0, 1, 0, 0, 0, 1, 0},
*		{1, 0, 0, 1, 0, 0, 1, 0},
*		{1, 0, 0, 0, 1, 0, 1, 0},
*		{1, 0, 0, 0, 0, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t N_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7F, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t O_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x3E, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t P_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x09, 0x09, 0x09, 0x09, 0x09, 0x06, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 1, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 1, 0, 0, 0},
*		{1, 0, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t R_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x09, 0x09, 0x09, 0x1B, 0x29, 0x46, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t S_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x46, 0x49, 0x49, 0x49, 0x49, 0x49, 0x31, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t T_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x01, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x01, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t V_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x03, 0x0C, 0x30, 0x40, 0x30, 0x0C, 0x03, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t ZERO_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x1C, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1C, 0x00};

/*
*
*	{
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 1, 1, 0, 0, 0, 0},
*		{0, 1, 0, 1, 0, 0, 0, 0},
*		{1, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t ONE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x48, 0x44, 0x42, 0x7F, 0x40, 0x40, 0x40, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 1, 0, 0},
*		{0, 1, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t TWO_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x44, 0x62, 0x51, 0x51, 0x51, 0x49, 0x46, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t THREE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x22, 0x41, 0x49, 0x49, 0x49, 0x55, 0x22, 0x00};

/*
*
*	{
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 1, 1, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 1, 0, 0, 1, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t FOUR_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x10, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x10, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t FIVE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x4F, 0x49, 0x49, 0x49, 0x49, 0x49, 0x31, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 1, 1, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t SIX_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x3C, 0x4A, 0x49, 0x49, 0x49, 0x49, 0x31, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 1, 0, 0, 0, 0, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t SEVEN_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x41, 0x21, 0x11, 0x09, 0x05, 0x03, 0x01, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t EIGHT_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{1, 1, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t NINE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x46, 0x49, 0x49, 0x49, 0x49, 0x29, 0x1E, 0x00};

/*
*
*	{
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 1, 1, 0, 0, 0},
*		{0, 0, 0, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t DOT_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 0, 0, 0, 0},
*		{0, 1, 0, 0, 1, 0, 0, 0},
*		{0, 1, 0, 0, 1, 0, 0, 0},
*		{0, 0, 1, 1, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t DEG_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x00, 0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00};

 static const uint8_t* const DIGITS[10] PROGMEM = {ZERO_PATTERN, ONE_PATTERN, TWO_PATTERN, THREE_PATTERN, FOUR_PATTERN, FIVE_PATTERN, SIX_PATTERN, SEVEN_PATTERN, EIGHT_PATTERN, NINE_PATTERN};

 static const uint8_t* const SELECT[SELECT_WORD_LENGTH] PROGMEM = {S_PATTERN, E_PATTERN, L_PATTERN, E_PATTERN, C_PATTERN, T_PATTERN};

 static const uint8_t* const MODE[MODE_WORD_LENGTH] PROGMEM = {M_PATTERN, O_PATTERN, D_PATTERN, E_PATTERN};

 static const uint8_t* const STANDARD[STANDARD_WORD_LENGTH] PROGMEM = {S_PATTERN, T_PATTERN, A_PATTERN, N_PATTERN, D_PATTERN, A_PATTERN, R_PATTERN, D_PATTERN};

 static const uint8_t* const ADVANCED[ADVANCED_WORD_LENGTH] PROGMEM = {A_PATTERN, D_PATTERN, V_PATTERN, A_PATTERN, N_PATTERN, C_PATTERN, E_PATTERN, D_PATTERN};

 static const uint8_t* const TEMP[TEMP_WORD_LENGTH] PROGMEM = {T_PATTERN, E_PATTERN, M_PATTERN, P_PATTERN, NULL, NULL, NULL, NULL, NULL, NULL, DOT_PATTERN, NULL, DEG_PATTERN, C_PATTERN};

 static const uint8_t* const SPEED[SPEED_WORD_LENGTH] PROGMEM = {S_PATTERN, P_PATTERN, E_PATTERN, E_PATTERN, D_PATTERN, NULL, NULL, NULL, NULL, NULL, NULL, R_PATTERN, P_PATTERN, M_PATTERN};

 static const uint8_t* const NODE[NODE_WORD_LENGTH] PROGMEM = {N_PATTERN, O_PATTERN, D_PATTERN, E_PATTERN};

 static const uint8_t* const ERR[ERR_WORD_LENGTH] PROGMEM = {E_PATTERN, R_PATTERN, R_PATTERN};

static display_errors char_write(const uint8_t* pattern_add, size_t row, size_t column) {
	ssd_errors error_code = 0;
	uint8_t pattern[CHAR_PATTERN_BYTES] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	if (pattern_add != NULL) {
	
		for (size_t i = 0; i < 8; i++) {
			pattern[i] = pgm_read_byte(pattern_add + i);
		}
	}
	
	error_code = ssd1306_data_write(pattern, row, column);
	
	if (error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	return DISPLAY_ERR_OK;
}

static display_errors blank_write(size_t length, size_t row, size_t column) {
	display_errors error_code = 0;
	
	for (size_t i = 0; i < length; i++) {
		error_code = char_write(NULL, row, (column + i));
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

static display_errors word_write(const uint8_t* const* word, size_t length, size_t row, size_t column) {
	display_errors error_code = 0;
	
	for (size_t i = 0; i < length; i++) {
		error_code = char_write(pgm_read_ptr_near(word + i), row, (column + i));
	
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

static void digits_extract(uint16_t full_value, uint8_t* digits, size_t length) {
	uint16_t divisor = 1;
	
	for (size_t i = 1; i < length; i++) {
		divisor *= 10;
	}
	
	for (size_t i = 0; i < length; i++) {
		*(digits + i) = (!i) ? (uint8_t) (full_value / divisor) : (uint8_t) ((full_value / divisor) % 10);
		divisor /= 10;
	}
}

display_errors display_intro_write(void) {
	ssd_errors ssd_error_code = 0;
	display_errors display_error_code = 0;
	
	ssd_error_code = ssd1306_data_reset();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	display_error_code = word_write(SELECT, SELECT_WORD_LENGTH, DISPLAY_SELECT_WORD_ROW, DISPLAY_SELECT_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	display_error_code = word_write(MODE, MODE_WORD_LENGTH, DISPLAY_MODE_WORD_ROW, DISPLAY_MODE_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	ssd_error_code = ssd1306_screen_turn_on();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_standard_write(void) {
	return word_write(STANDARD, STANDARD_WORD_LENGTH, DISPLAY_STANDARD_WORD_ROW, DISPLAY_STANDARD_WORD_COLUMN);
}

display_errors display_advanced_write(void) {
	return word_write(ADVANCED, ADVANCED_WORD_LENGTH, DISPLAY_ADVANCED_WORD_ROW, DISPLAY_ADVANCED_WORD_COLUMN);
}

display_errors display_index_write(size_t index) {
	display_errors error_code = 0;
	
	if (index < MIN_INDEX || index > MAX_INDEX) {
		return DISPLAY_ERR_PARAM;
	}
	
	error_code = word_write(NODE, NODE_WORD_LENGTH, DISPLAY_NODE_WORD_ROW, DISPLAY_NODE_WORD_COLUMN);
	
	if (error_code != DISPLAY_ERR_OK) {
		return error_code;
	}
	
	if (index < MAX_INDEX) {
		error_code = blank_write(1, DISPLAY_NODE_DIGITS_ROW, DISPLAY_NODE_DIGITS_COLUMN);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
		
		error_code = char_write(pgm_read_ptr_near(DIGITS + index), DISPLAY_NODE_DIGITS_ROW, DISPLAY_NODE_DIGITS_COLUMN + 1);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	else if (index == MAX_INDEX) {
		
		error_code = char_write(pgm_read_ptr_near(DIGITS + 1), DISPLAY_NODE_DIGITS_ROW, DISPLAY_NODE_DIGITS_COLUMN);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
		
		error_code = char_write(pgm_read_ptr_near(DIGITS), DISPLAY_NODE_DIGITS_ROW, DISPLAY_NODE_DIGITS_COLUMN + 1);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_units_write(void) {
	ssd_errors ssd_error_code = 0;
	display_errors display_error_code = 0;
	
	ssd_error_code = ssd1306_data_reset();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	display_error_code = word_write(TEMP, TEMP_WORD_LENGTH, DISPLAY_TEMP_WORD_ROW, DISPLAY_TEMP_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	display_error_code = word_write(SPEED, SPEED_WORD_LENGTH, DISPLAY_SPEED_WORD_ROW, DISPLAY_SPEED_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}

	return DISPLAY_ERR_OK;
}

display_errors display_temp_write(uint16_t temp_c_x10) {
	display_errors error_code = 0;
	uint8_t digits[TEMP_DIGITS] = {0, 0, 0};
	
	if (temp_c_x10 > MAX_TEMP_C_X10) {
		return DISPLAY_ERR_PARAM;
	}
	
	digits_extract(temp_c_x10, digits, TEMP_DIGITS);
	
	for (size_t i = 0; i < TEMP_DIGITS; i++) {
		error_code = char_write(pgm_read_ptr_near(DIGITS + digits[i]), DISPLAY_TEMP_DIGITS_ROW, (uint8_t) (DISPLAY_TEMP_DIGITS_COLUMN + i + (i / 2)));
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}

	return DISPLAY_ERR_OK;
}

display_errors display_speed_write(uint16_t speed_rpm) {
	display_errors error_code = 0;
	uint8_t digits[SPEED_DIGITS] = {0, 0, 0, 0};
	
	if (speed_rpm > MAX_SPEED_RPM) {
		return DISPLAY_ERR_PARAM;
	}
	
	digits_extract(speed_rpm, digits, SPEED_DIGITS);
	
	for (size_t i = 0; i < SPEED_DIGITS; i++) {
		error_code = char_write(pgm_read_ptr_near(DIGITS + digits[i]), DISPLAY_SPEED_DIGITS_ROW, DISPLAY_SPEED_DIGITS_COLUMN + i);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_error_write(system_errors error_code) {
	display_errors display_error_code = 0;
	ssd_errors ssd_error_code = 0;
	
	if (error_code > SYS_ERR_FAN_DRIVER) {
		return DISPLAY_ERR_PARAM;
	}

	ssd_error_code = ssd1306_data_reset();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	display_error_code = word_write(ERR, ERR_WORD_LENGTH, DISPLAY_ERR_WORD_ROW, DISPLAY_ERR_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	display_error_code = char_write(pgm_read_ptr_near(DIGITS + error_code), DISPLAY_ERR_DIGITS_ROW, DISPLAY_ERR_DIGITS_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	return DISPLAY_ERR_OK;
}

typedef struct {
	uint16_t target_duration;
	uint16_t update_t_0;
	blink_options option;
	uint8_t state;
	uint8_t digit;
	uint8_t init_flag;
} blink_controller;

static blink_controller blink = {0};

static display_errors word_blink_state_switch(blink_options option, uint8_t state) {
	display_errors error_code = 0;
	
	if (option == STANDARD_WORD) {
		error_code = state ? blank_write(STANDARD_WORD_LENGTH, DISPLAY_STANDARD_WORD_ROW, DISPLAY_STANDARD_WORD_COLUMN) : display_standard_write();
	}
		
	else if (option == ADVANCED_WORD) {
		error_code = state ? blank_write(ADVANCED_WORD_LENGTH, DISPLAY_ADVANCED_WORD_ROW, DISPLAY_ADVANCED_WORD_COLUMN) : display_advanced_write();
	}
	
	return error_code;
}
		
static display_errors digit_blink_state_switch(blink_options option, uint8_t state, uint8_t digit) {
	display_errors error_code = 0;
		
	switch (option) {
		
		case TEMP_FIRST_DIGIT:
		
			error_code = state ? blank_write(1, DISPLAY_TEMP_DIGITS_ROW, DISPLAY_TEMP_DIGITS_COLUMN) : char_write(pgm_read_ptr_near(DIGITS + digit), DISPLAY_TEMP_DIGITS_ROW, DISPLAY_TEMP_DIGITS_COLUMN);
			break;
		
		case TEMP_SECOND_DIGIT:
		
			error_code = state ? blank_write(1, DISPLAY_TEMP_DIGITS_ROW, DISPLAY_TEMP_DIGITS_COLUMN + 1) : char_write(pgm_read_ptr_near(DIGITS + digit), DISPLAY_TEMP_DIGITS_ROW, DISPLAY_TEMP_DIGITS_COLUMN + 1);
			break;
		
		case TEMP_THIRD_DIGIT:
		
			error_code = state ? blank_write(1, DISPLAY_TEMP_DIGITS_ROW, DISPLAY_TEMP_DIGITS_COLUMN + 3) : char_write(pgm_read_ptr_near(DIGITS + digit), DISPLAY_TEMP_DIGITS_ROW, DISPLAY_TEMP_DIGITS_COLUMN + 3);
			break;
		
		case SPEED_FIRST_DIGIT:
		
			error_code = state ? blank_write(1, DISPLAY_SPEED_DIGITS_ROW, DISPLAY_SPEED_DIGITS_COLUMN) : char_write(pgm_read_ptr_near(DIGITS + digit), DISPLAY_SPEED_DIGITS_ROW, DISPLAY_SPEED_DIGITS_COLUMN);
			break;
		
		case SPEED_SECOND_DIGIT:
		
			error_code = state ? blank_write(1, DISPLAY_SPEED_DIGITS_ROW, DISPLAY_SPEED_DIGITS_COLUMN + 1) : char_write(pgm_read_ptr_near(DIGITS + digit), DISPLAY_SPEED_DIGITS_ROW, DISPLAY_SPEED_DIGITS_COLUMN + 1);
			break;
		
		case SPEED_THIRD_DIGIT:
		
			error_code = state ? blank_write(1, DISPLAY_SPEED_DIGITS_ROW, DISPLAY_SPEED_DIGITS_COLUMN + 2) : char_write(pgm_read_ptr_near(DIGITS + digit), DISPLAY_SPEED_DIGITS_ROW, DISPLAY_SPEED_DIGITS_COLUMN + 2);
			break;
		
		default:
			
			error_code = DISPLAY_ERR_PARAM;
			break;
	}

	return error_code;
}

static void blink_controller_reset(void) {
	blink.target_duration = 0;
	blink.update_t_0 = 0;
	blink.option = 0;
	blink.state = 0;
	blink.digit = 0;
	blink.init_flag = 0;
}

display_errors display_word_blink(uint16_t target_duration, blink_options option) {
	
	if (!target_duration || option > ADVANCED_WORD || option < STANDARD_WORD) {
		return DISPLAY_ERR_PARAM;
	}
	
	blink_controller_reset();
	blink.target_duration = target_duration;
	blink.update_t_0 = scheduler_timestamp_capture();
	blink.option = option;
	blink.state = 1;
	blink.init_flag = 1;
	return DISPLAY_ERR_OK;
}

display_errors display_digit_blink(uint16_t target_duration, blink_options option, uint16_t full_value) {
	uint8_t digits [SPEED_DIGITS] = {0, 0, 0, 0};
	
	if (target_duration == 0 || option < TEMP_FIRST_DIGIT || option > SPEED_THIRD_DIGIT) {
		return DISPLAY_ERR_PARAM;
	}
	
	if (option < SPEED_FIRST_DIGIT && full_value > MAX_TEMP_C_X10) {
		return DISPLAY_ERR_PARAM;
	}
	
	if (option > TEMP_THIRD_DIGIT && full_value > MAX_SPEED_RPM) {
		return DISPLAY_ERR_PARAM;
	}
	
	blink_controller_reset();
	blink.target_duration = target_duration;
	blink.update_t_0 = scheduler_timestamp_capture();
	blink.option = option;
	blink.state = 1;
	blink.init_flag = 1;
	
	if (option < SPEED_FIRST_DIGIT) {
		digits_extract(full_value, digits, TEMP_DIGITS);
		blink.digit = !(option - TEMP_FIRST_DIGIT) ? digits[0] : (!(option - TEMP_SECOND_DIGIT) ? digits[1] : digits[2]);
	}
	
	else {
		digits_extract(full_value, digits, SPEED_DIGITS);
		blink.digit = !(option - SPEED_FIRST_DIGIT) ? digits[0] : (!(option - SPEED_SECOND_DIGIT) ? digits[1] : digits[2]);
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_blink_state_switch(void) {
	display_errors error_code = 0;
	
	if (	!blink.init_flag) {
		return DISPLAY_ERR_PARAM;
	}
	
	if (scheduler_timer_elapsed(blink.update_t_0, blink.target_duration)) {
		error_code = (blink.option < TEMP_FIRST_DIGIT) ? word_blink_state_switch(blink.option, blink.state) : digit_blink_state_switch(blink.option, blink.state, blink.digit);
		blink.state = !blink.state;
		blink.update_t_0 = scheduler_timestamp_capture();
	}
	
	return error_code;
}
