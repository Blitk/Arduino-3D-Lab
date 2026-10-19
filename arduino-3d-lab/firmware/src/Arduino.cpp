
#include "Arduino.h"

#include <avr/io.h>
#include <util/delay.h>

void pinMode(uint8_t pin, uint8_t mode)
{
    if (pin != 13) {
        return;
    }

    if (mode == OUTPUT) {
        DDRB |= _BV(DDB5);
    } else {
        DDRB &= static_cast<uint8_t>(~_BV(DDB5));
    }
}

void digitalWrite(uint8_t pin, uint8_t value)
{
    if (pin != 13) {
        return;
    }

    if (value == HIGH) {
        PORTB |= _BV(PORTB5);
    } else {
        PORTB &= static_cast<uint8_t>(~_BV(PORTB5));
    }
}

void delay(unsigned long milliseconds)
{
    while (milliseconds > 0) {
        _delay_ms(1);
        --milliseconds;
    }
}
