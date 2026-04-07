/*
 * NombreProgra.c
 *
 * Created: 07/04/2026
 * Author: Juan Rodrigo Donis
 * Description: Prelab
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/****************************************/
// Function prototypes

void mostrarContador(uint8_t valor);
uint8_t leerBoton(uint8_t pin);

/****************************************/
// Main Function

#define BTN_INC PB0
#define BTN_DEC PB1

int main(void)
{
    uint8_t contador = 0;

    // D2-D7 como salidas para bits 0-5
    DDRD |= 0b11111100;

    // A0-A1 como salidas para bits 6-7
    DDRC |= 0b00000011;

    // D8 y D9 como entradas
    DDRB &= ~((1 << BTN_INC) | (1 << BTN_DEC));

    // Pull-up interno en botones
    PORTB |= (1 << BTN_INC) | (1 << BTN_DEC);

    mostrarContador(contador);

    while (1)
    {
        if (leerBoton(BTN_INC))
        {
            contador++;
            mostrarContador(contador);
        }
        else if (leerBoton(BTN_DEC))
        {
            contador--;
            mostrarContador(contador);
        }
    }
}

/****************************************/
// NON-Interrupt subroutines

void mostrarContador(uint8_t valor)
{
    // Bits 0 a 5 del contador -> PD2 a PD7
    PORTD = (PORTD & 0b00000011) | ((valor & 0b00111111) << 2);

    // Bits 6 y 7 del contador -> PC0 y PC1
    PORTC = (PORTC & 0b11111100) | ((valor >> 6) & 0b00000011);
}

uint8_t leerBoton(uint8_t pin)
{
    // Con pull-up: presionado = 0
    if (!(PINB & (1 << pin)))
    {
        _delay_ms(25);

        if (!(PINB & (1 << pin)))
        {
            while (!(PINB & (1 << pin)))
            {
                // Espera a que se suelte
            }

            _delay_ms(25);
            return 1;
        }
    }

    return 0;
}

/****************************************/
// Interrupt routines