/*
 * Chip type: Attiny13/Attiny13A
 * Clock frequency: 4.8MHz with divider
 * Fuse settings: -Ulfuse:w:0x65:m -Uhfuse:w:0xFD:m
 *
 * Wiring:
 *                       +--------+
 *        [        (PB5) |1*     8| (VCC)  BATT+ ]
 *        [ LED1   (PB3) |2      7| (PB2)    ADC ]
 *        [ LED2   (PB4) |3      6| (PB1)   LED3 ]
 *        [ BATT-  (GND) |4      5| (PB0)   LED4 ]
 *                       +--------+
 *
 * Voltage divider:
 * VCC--[ R1 ]--ADC--[ R2 ]--GND
 *
 * Author: SinuX
 * License: CC BY-NC-SA
 */ 

#define F_CPU 4800000UL
#include <avr/interrupt.h>

// ADC value calculation: (Ubat * R2 * 255) / ((R1 + R2) * 1.1) ± 10%
// Calculated values for R1 = 18K, R2 - 4.7K:
#define UHI		210 // Ubat > 4.2V
#define U100	195 // Ubat > 3.9V
#define U75		185 // Ubat > 3.7V
#define U50		175 // Ubat > 3.5V
#define U25		160 // Ubat > 3.3V

// Hysteresis to prevent flickering
#define UHYS	10

// Comment out to use single led indication mode
#define USE_ALL_LEDS

#define LED1	PB3
#define LED2	PB4
#define LED3	PB1
#define LED4	PB0
#define ADC_IN	PB2
#define ADC_CH	1

#define STATE_LOW		0
#define STATE_25		1
#define STATE_50		2
#define STATE_75		3
#define STATE_100		4
#define STATE_HIGH		5

// Set LED1...LED4 as out; assign zero to all; disable digital input buffer on ADC port
#define portinit() do { DDRB = (1 << LED4) | (1 << LED3) | (1 << LED1) | (1 << LED2); PORTB = 0; DIDR0 |= (1 << ADC_IN); } while (0)
// Reference 1.1V, left-adjust for ADC1/PB2; enable; start; auto; interrupt; clk/128
#define adcinit() do { ADMUX = 0b01100000 | ADC_CH; ADCSRA = 0b11101111; } while (0)
// Set level on pin
#define set_out(val, pin) val ? (PORTB |= (1 << pin)) : (PORTB &= ~(1 << pin))

volatile uint8_t blink_counter = 0;
volatile uint8_t current_state = STATE_HIGH;

ISR(ADC_vect) {	
	if (current_state == STATE_HIGH && ADCH <= UHI) current_state = STATE_100;
	if (current_state == STATE_100 && ADCH <= U100) current_state = STATE_75;
	if (current_state == STATE_75 && ADCH <= U75) current_state = STATE_50;
	if (current_state == STATE_50 && ADCH <= U50) current_state = STATE_25;
	if (current_state == STATE_25 && ADCH <= U25) current_state = STATE_LOW;
	
	if (current_state == STATE_LOW && ADCH >= U25 + UHYS) current_state = STATE_25;
	if (current_state == STATE_25 && ADCH >= U50 + UHYS) current_state = STATE_50;
	if (current_state == STATE_50 && ADCH >= U75 + UHYS) current_state = STATE_75;
	if (current_state == STATE_75 && ADCH >= U100 + UHYS) current_state = STATE_100;
	if (current_state == STATE_100 && ADCH >= UHI + UHYS) current_state = STATE_HIGH;
	
	#ifdef USE_ALL_LEDS
		set_out((current_state == STATE_HIGH ? (blink_counter >> 7) : current_state == STATE_100), LED4);
		set_out(current_state >= STATE_75, LED3);
		set_out(current_state >= STATE_50, LED2);
		set_out((current_state == STATE_LOW ? (blink_counter >> 7) : current_state >= STATE_25), LED1);
	#else
		set_out((current_state == STATE_HIGH ? (blink_counter >> 7) : current_state == STATE_100), LED4);
		set_out(current_state == STATE_75, LED3);
		set_out(current_state == STATE_50, LED2);
		set_out((current_state == STATE_LOW ? (blink_counter >> 7) : current_state == STATE_25), LED1);
	#endif
	
	blink_counter++;
}

int main(void) {
    portinit();
	adcinit();
	sei();
    while (1);
}
