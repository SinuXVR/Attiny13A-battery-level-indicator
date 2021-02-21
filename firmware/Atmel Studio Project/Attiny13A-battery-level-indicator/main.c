/*
 * Chip type: Attiny13/Attiny13A
 * Clock frequency: 4.8MHz with divider
 * Fuse settings: -Ulfuse:w:0x65:m -Uhfuse:w:0xFD:m
 * Use -Ulfuse:w:0x75:m -Uhfuse:w:0xFD:m to disable clock divider (increases PWM frequency)
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

// Uncomment to enable smooth LEDs switching
//#define FADE_LEDS

// Brightness of each channel (0 - PWM_MAX_VALUE)
#define LED1_MAX	PWM_MAX_VALUE
#define LED2_MAX	PWM_MAX_VALUE
#define LED3_MAX	PWM_MAX_VALUE
#define LED4_MAX	PWM_MAX_VALUE

#define PWM_MAX_VALUE	100

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
#define set_high(pin) PORTB |= (1 << pin)
#define set_low_all() PORTB &= ~((1 << LED4) | (1 << LED3) | (1 << LED1) | (1 << LED2))

volatile uint8_t blink_counter = 0;
volatile uint8_t led1_brightness = 0;
volatile uint8_t led2_brightness = 0;
volatile uint8_t led3_brightness = 0;
volatile uint8_t led4_brightness = 0;
volatile uint8_t current_state = STATE_HIGH;

static void check_state() {
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
}

static void recalculate_brightness() {
	#ifdef USE_ALL_LEDS
		#ifdef FADE_LEDS
			led1_brightness += (current_state == STATE_LOW ? (blink_counter >> 7) : current_state >= STATE_25) ? (led1_brightness < LED1_MAX ? 1 : 0) : (led1_brightness > 0 ? -1 : 0);
			led2_brightness += (current_state >= STATE_50) ? (led2_brightness < LED2_MAX ? 1 : 0) : (led2_brightness > 0 ? -1 : 0);
			led3_brightness += (current_state >= STATE_75) ? (led3_brightness < LED3_MAX ? 1 : 0) : (led3_brightness > 0 ? -1 : 0);
			led4_brightness += (current_state == STATE_HIGH ? (blink_counter >> 7) : current_state == STATE_100) ? (led4_brightness < LED4_MAX ? 1 : 0) : (led4_brightness > 0 ? -1 : 0);
		#else
			led1_brightness = (current_state == STATE_LOW ? (blink_counter >> 7) : current_state >= STATE_25) ? LED1_MAX : 0;
			led2_brightness = (current_state >= STATE_50) ? LED2_MAX : 0;
			led3_brightness = (current_state >= STATE_75) ? LED3_MAX : 0;
			led4_brightness = (current_state == STATE_HIGH ? (blink_counter >> 7) : current_state == STATE_100) ? LED4_MAX : 0;
		#endif
	#else
		#ifdef FADE_LEDS
			led1_brightness += (current_state == STATE_LOW ? (blink_counter >> 7) : current_state == STATE_25) ? (led1_brightness < LED1_MAX ? 1 : 0) : (led1_brightness > 0 ? -1 : 0);
			led2_brightness += (current_state == STATE_50) ? (led2_brightness < LED2_MAX ? 1 : 0) : (led2_brightness > 0 ? -1 : 0);
			led3_brightness += (current_state == STATE_75) ? (led3_brightness < LED3_MAX ? 1 : 0) : (led3_brightness > 0 ? -1 : 0);
			led4_brightness += (current_state == STATE_HIGH ? (blink_counter >> 7) : current_state == STATE_100) ? (led4_brightness < LED4_MAX ? 1 : 0) : (led4_brightness > 0 ? -1 : 0);
		#else
			led1_brightness = (current_state == STATE_LOW ? (blink_counter >> 7) : current_state == STATE_25) ? LED1_MAX : 0;
			led2_brightness = (current_state == STATE_50) ? LED2_MAX : 0;
			led3_brightness = (current_state == STATE_75) ? LED3_MAX : 0;
			led4_brightness = (current_state == STATE_HIGH ? (blink_counter >> 7) : current_state == STATE_100) ? LED4_MAX : 0;
		#endif
	#endif
	blink_counter++;
}

static void do_pwm() {
	for (uint8_t i = PWM_MAX_VALUE; i > 0; i--) {
		if (i == led1_brightness) set_high(LED1);
		if (i == led2_brightness) set_high(LED2);
		if (i == led3_brightness) set_high(LED3);
		if (i == led4_brightness) set_high(LED4);
	}
	set_low_all();
}

ISR(ADC_vect) {	
	check_state();
	recalculate_brightness();
	do_pwm();
}

int main(void) {
    portinit();
	adcinit();
	sei();
    while (1);
}
