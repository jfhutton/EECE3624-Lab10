/*
 * Program: BTReadyAVRController.c
 * Author: Greg Nordstrom
 * Date: 3/4/19
 *
 * This program allows the ReadyAVR to act as a Bluetooth-based
 * game controller. It was specifically designed for the S2019
 * EECE4254 final project (a ZumoBot competition) but is easily
 * adaptable to other uses.
 *
 * The five on-board LEDs and the active low joystick switches
 * are connected as follows:
 *
 *          LED   Pin       Switch  Pin
 *         -----  ---       ------  ---
 *          Boot  PA7       Right   PB0
 *          LED0  PC0       Down    PB1
 *          LED1  PC1       Left    PB2
 *          LED2  PC2        Up     PB3
 *          LED3  PC3       Press   PB4
 *
 *
 * This code uses the USART0Library.h and .c files to configure
 * the Rx (PE0) and Tx (PE1) pins of the ATMega128A. The ReadyAVR
 * board uses USART1 for communicating with Atmel Studio, leaving
 * USART0 for more direct use (e.g. connecting to a slave!) That
 * way, the code can be debugged while communicating with the slave.
 *
 * Bluetooth HC-05 modules are expected to be connected to both the
 * slave and master nodes, although any sort of link (wired or
 * wireless) will work. This code simply expects the to send data
 * to the slave via the USART0 Tx line (pin PE1 on the ATMega128A).
 *
 */ 

#include <avr/io.h>
#include <stdlib.h>
#define F_CPU 7372800		  // eliminating this causes "not defined" error (in delay.h)
#include <util/delay.h>
#include "USART0Library.h"	  // can include libUSART0Library.a in solution explorer "Libraries"
                              // or add the USART0Library.c file in this project

#define BaudRate   9600       // doesn't seem to work beyond 38400 baud
#define DataBits      8
#define StopBits      1
#define NoParity      0

#define MIN_SPEED    -25		// full speed backward
#define MAX_SPEED     25		// full speed forward

#define MIN_TURN     -25		// full left turn
#define MAX_TURN      25		// full right turn
/* Turning involves one side having more power than the other.
   the table below shows the percent of power to each track
   for all allowed currentTurn values (-25 to 25). Note that
   multipliers are applied to the current speed values and
   applied to each track.
UPDATE THIS TABLE
     motion      currentTurn    left track            right track
   ----------    -----------    -------------------   -------------------
   full left         -4         currentSpeed x 0.00   currentSpeed x 1.00
   0.75 left         -3         currentSpeed x 0.25   currentSpeed x 1.00
   0.50 left         -2         currentSpeed x 0.50   currentSpeed x 1.00
   0.25 left         -1         currentSpeed x 0.75   currentSpeed x 1.00
   straight           0         currentSpeed x 1.00   currentSpeed x 1.00
   0.25 right         1         currentSpeed x 1.00   currentSpeed x 0.75
   0.50 right         2         currentSpeed x 1.00   currentSpeed x 0.50
   0.75 right         3         currentSpeed x 1.00   currentSpeed x 0.25
   full right         4         currentSpeed x 1.00   currentSpeed x 0.00
*/

#define LEDS			PORTC
#define STOP_BUTTON		16
#define FORWARD_BUTTON	 8
#define LEFT_BUTTON		 4
#define BACKWARD_BUTTON  2
#define RIGHT_BUTTON	 1

int main(void)
{
	// initialize joystick switches (PB4:PB0) as inputs
	DDRB |= (1<<DDRB7 | 1<<DDRB6 | 1<<DDRB5);

	// turn on pull-ups for active low joystick switches
	PORTB |= ~(1<<PORTB7 | 1<<PORTB6 | 1<<PORTB5);

	// initialize LEDs 0-3 (PC3:PC0) as outputs
	DDRC |= (1<<DDRC3 | 1<<DDRC2 | 1<<DDRC1 | 1<<DDRC0);

	// turn off the active low LEDs
	LEDS |= (1<<DDRC3 | 1<<DDRC2 | 1<<DDRC1 | 1<<DDRC0);

    // initialize USART0 (NB: PE0=Rx, PE1=Tx)
    usart0_init(BaudRate, DataBits, StopBits, NoParity);

	// initialize the joystickState state variable
	unsigned char joyMask = (unsigned char) ~(1<<PORTB7 | 1<<PORTB6 | 1<<PORTB5);
	unsigned char joyState;
	// NB: in AtmelStudio chars are unsigned by default (!@#$!)
	signed char currentSpeed = 0;  // initially not moving
	signed char currentTurn  = 0;  // initially not turning
	unsigned char ledPattern;

	while(1) {
		// read the joystick switches (NB: joymask = 0b00011111)
		joyState = (PINB & joyMask) ^ joyMask;
		
		// determine if a button was pressed
		switch(joyState) {
			case STOP_BUTTON:
				usart0_tx('S');
				currentSpeed = 0;
				currentTurn = 0;

				// wait for button to be released
				while(((PINB & joyMask) ^ joyMask) == STOP_BUTTON) {
					_delay_ms(1);
				}

				break;
			case FORWARD_BUTTON:
				if(currentSpeed < MAX_SPEED) {
					currentSpeed++;
				}

				// send direction character ('F' or 'B')
				if(currentSpeed >= 0) {
					usart0_tx('F');
				}
				else {
					usart0_tx('B');
				}

				// send speed value (0-25) as a character (a-z)
				usart0_tx(abs(currentSpeed) + 'a');
				break;
			case BACKWARD_BUTTON:
				if(currentSpeed > MIN_SPEED) {
					currentSpeed--;
				}

				// send direction character ('F' or 'B')
				if(currentSpeed >= 0) {
					usart0_tx('F');
				}
				else {
					usart0_tx('B');
				}

				// send speed value (0-25) as a character (a-z)
				usart0_tx(abs(currentSpeed) + 'a');
				break;
			case LEFT_BUTTON:
 				usart0_tx('L');
				break;
			case RIGHT_BUTTON:
 				usart0_tx('R');
				break;
		}

		// show the LED "speed bar" according to currentSpeed (1-4)
		// NB: the LEDs form a stacked "speed bar" with 1, 2, 3, or 4
		// LEDs lit (corresponding to speeds of 1 to 4). Note that
		// the bottom LED is LED3, so must light them in reverse
		// order (LED3:LED1)
		if(abs(currentSpeed)==0)
			ledPattern = (LEDS & 0xF0) + 0b1111;
		else if(abs(currentSpeed)<=6)
			ledPattern = (LEDS & 0xF0) + 0b0111;
		else if(abs(currentSpeed)<=12)
			ledPattern = (LEDS & 0xF0) + 0b0011;
		else if(abs(currentSpeed)<=18)
			ledPattern = (LEDS & 0xF0) + 0b0001;
		else
			ledPattern = (LEDS & 0xF0) + 0b0000;


		// set transmit delay for "optimal" responsiveness (about 1/20 sec)
		if(currentSpeed < 0) {
			// moving in reverse, so blink speed LEDS
			LEDS = 0x0F;		// all LEDs off
			_delay_ms(20);
			LEDS = ledPattern;	// turn on proper speed bits
			_delay_ms(30);
		}
		else {
			// moving forward or stopped, so just display and delay
			LEDS = ledPattern;
			_delay_ms(50);
		}
	}
}
