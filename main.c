#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define BUTTON_STANDBY_TIMER_TOP 50
#define BUTTON_PRESS_MANUAL_STOP 1
#define BUTTON_PRESS_RESET_TIMER 2
#define BUTTON_PRESS_MANUAL_SPIN 3
#define PERIOD 20 // 28800

volatile uint8_t timer_seconds;
volatile uint8_t timer_ticks;

volatile uint8_t button_press_counter;
volatile uint8_t button_wait_timer;

static inline void led_on() {
    PORTC |= (1 << PC0);
}

static inline void led_off() {
    PORTC &= !(1 << PC0);
}

static inline void servo_on() {
    OCR1A = 500;
}

static inline void servo_off() {
    OCR1A = 0;
}

void handle_button_press() {
    switch (button_press_counter) {
        case BUTTON_PRESS_MANUAL_STOP:
            servo_off();
            break;
        case BUTTON_PRESS_RESET_TIMER:
            timer_seconds = 0;
            break;
        case BUTTON_PRESS_MANUAL_SPIN:
            servo_on();
            break;
        default:
            break;
    }
    button_press_counter = 0;
}

void handle_led_blinking() {
    switch (timer_seconds) {
        case 1:
            led_on();
            break;
        case 2:
            led_off();
            break;
        case 3:
            led_on();
            break;
        default:
            led_off();
            break;
    }
}

ISR(INT0_vect) {
    servo_off();
}

ISR(INT1_vect) {
    if (button_wait_timer < BUTTON_STANDBY_TIMER_TOP - 10) {
        button_press_counter++;
        button_wait_timer = BUTTON_STANDBY_TIMER_TOP;
    }
}

ISR(TIMER1_COMPA_vect) {
    timer_ticks++;
    if (timer_ticks >= 50) {
        timer_seconds++;
        timer_ticks = 0;
    }
    handle_led_blinking();
    if (timer_seconds >= PERIOD) {
        if (OCR1A == 0) {
            servo_on();
        }
        timer_seconds = 0;
    }

    if (button_wait_timer > 0) {
        button_wait_timer--;
    }
    if (button_press_counter > 0 && button_wait_timer == 0) {
        handle_button_press();
    }
}

int main(void) {
    // Button
    DDRD &= !(1 << PD3);
    // Yellow LED
    DDRC |= (1 << PC0);

    // Servo control
    // Fast PWM Waveform Generation Mode
    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM12) | (1 << WGM13);
    // 1 timer divider
    TCCR1B |= (1 << CS10);
    // Set output to OCR1A
    TCCR1A |= (1 << COM1A1);
    // TOP value
    ICR1 = 19999;
    OCR1A = 0;
    DDRD |= (1 << PD5);

    // Interrupts
    // Turn on the Timer/Counter1 Output Compare A match interrupt
    TIMSK |= (1 << OCIE1A);
    // Turn on interrupts on pins INT0 and INT1
    GICR |= (1 << INT0) | (1 << INT1);
    // Generate INT0 interrupt on rising egde
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    // Generate INT1 interrupt on falling egde
    MCUCR |= (1 << ISC11);

    _delay_ms(1000);
    sei();
    while (1) {
    }
    return 0;
}