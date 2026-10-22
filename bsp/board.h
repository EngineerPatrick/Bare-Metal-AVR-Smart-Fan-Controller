/**
*
*	@file board.h
*
*	@brief UNO R3 board pins map.
*
*	@details MCU-board interface that maps every board pin
*   to a bit in an MCU register.
*
*/

#ifndef BOARD_H
#define BOARD_H

#include <avr/io.h>

#define D0_D7_DATA_REG PORTD

#define DR_D0 (1 << PORTD0)
#define DR_D1 (1 << PORTD1)
#define DR_D2 (1 << PORTD2)
#define DR_D3 (1 << PORTD3)
#define DR_D4 (1 << PORTD4)
#define DR_D5 (1 << PORTD5)
#define DR_D6 (1 << PORTD6)
#define DR_D7 (1 << PORTD7)

#define D8_D13_DATA_REG PORTB

#define DR_D8 (1 << PORTB0)
#define DR_D9 (1 << PORTB1)
#define DR_D10 (1 << PORTB2)
#define DR_D11 (1 << PORTB3)
#define DR_D12 (1 << PORTB4)
#define DR_D13 (1 << PORTB5)

#define D14_D19_DATA_REG PORTC

#define DR_D14 (1 << PORTC0)
#define DR_D15 (1 << PORTC1)
#define DR_D16 (1 << PORTC2)
#define DR_D17 (1 << PORTC3)
#define DR_D18 (1 << PORTC4)
#define DR_D19 (1 << PORTC5)

#define D0_D7_DATA_DIRECTION_REG DDRD

#define DDR_D0 (1 << DDD0)
#define DDR_D1 (1 << DDD1)
#define DDR_D2 (1 << DDD2)
#define DDR_D3 (1 << DDD3)
#define DDR_D4 (1 << DDD4)
#define DDR_D5 (1 << DDD5)
#define DDR_D6 (1 << DDD6)
#define DDR_D7 (1 << DDD7)

#define D8_D13_DATA_DIRECTION_REG DDRB

#define DDR_D8 (1 << DDB0)
#define DDR_D9 (1 << DDB1)
#define DDR_D10 (1 << DDB2)
#define DDR_D11 (1 << DDB3)
#define DDR_D12 (1 << DDB4)
#define DDR_D13 (1 << DDB5)

#define D14_D19_DATA_DIRECTION_REG DDRC

#define DDR_D14 (1 << DDC0)
#define DDR_D15 (1 << DDC1)
#define DDR_D16 (1 << DDC2)
#define DDR_D17 (1 << DDC3)
#define DDR_D18 (1 << DDC4)
#define DDR_D19 (1 << DDC5)

#define D0_D7_INPUT_PINS_REG PIND

#define IPR_D0 (1 << PIND0)
#define IPR_D1 (1 << PIND1)
#define IPR_D2 (1 << PIND2)
#define IPR_D3 (1 << PIND3)
#define IPR_D4 (1 << PIND4)
#define IPR_D5 (1 << PIND5)
#define IPR_D6 (1 << PIND6)
#define IPR_D7 (1 << PIND7)

#define D8_D13_INPUT_PINS_REG PINB

#define IPR_D8 (1 << PINB0)
#define IPR_D9 (1 << PINB1)
#define IPR_D10 (1 << PINB2)
#define IPR_D11 (1 << PINB3)
#define IPR_D12 (1 << PINB4)
#define IPR_D13 (1 << PINB5)

#define D14_D19_INPUT_PINS_REG PINC

#define IPR_D14 (1 << PINC0)
#define IPR_D15 (1 << PINC1)
#define IPR_D16 (1 << PINC2)
#define IPR_D17 (1 << PINC3)
#define IPR_D18 (1 << PINC4)
#define IPR_D19 (1 << PINC5)

#endif