/*
 * NombreProgra.c
 *
 * Created: 12/04/2026
 * Author: Juan Rodrigo Donis
 * Description: Contador binario de 8 bits + lectura ADC de potenciómetro
 *              y despliegue del valor en hexadecimal en 2 displays de 7 segmentos.
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/****************************************/
// Function prototypes

void initGPIO(void);
void initADC(void);

void mostrarContador(uint8_t valor);
void revisarBotones(void);
void revisarAlarma(uint8_t valorADC, uint8_t valorContador);

uint8_t ADC_read8(void);

uint8_t patronHex(uint8_t valor);
void escribirSegmentos(uint8_t patron);
void refrescarDisplays(uint8_t valorHex);

/****************************************/
// Main Function

#define BTN_INC   PB0
#define BTN_DEC   PB1
#define LED_ALARM PD1

uint8_t contador = 0;
uint8_t adcHex = 0;

int main(void)
{
    // Aqui primero dejo listo todo lo basico
    initGPIO();
    initADC();

    // Inicio mostrando el contador en 0
    mostrarContador(contador);

    // Tambien hago una primera lectura del ADC
    adcHex = ADC_read8();

    while (1)
    {
        // Yo reviso los botones para actualizar el contador
        revisarBotones();

        // Aqui vuelvo a reflejar el valor binario en los LEDs
        mostrarContador(contador);

        // Leo el ADC desde el potenciómetro
        adcHex = ADC_read8();

        // Comparo ADC vs contador para manejar la alarma
        revisarAlarma(adcHex, contador);

        // Refresco los dos displays para mostrar el valor hexadecimal
        refrescarDisplays(adcHex);
    }
}

/****************************************/
// NON-Interrupt subroutines

void initGPIO(void)
{
    // Yo uso D2-D7 para los bits 0 a 5 del contador
    DDRD |= 0b11111100;

    // Yo uso A0-A1 para los bits 6 y 7 del contador
    DDRC |= 0b00000011;

    // Configuro D8 y D9 como entradas para los botones
    DDRB &= ~((1 << BTN_INC) | (1 << BTN_DEC));

    // Activo pull-up interno para no usar resistencia externa
    PORTB |= (1 << BTN_INC) | (1 << BTN_DEC);

    // Segmentos del display:
    // a -> PB2 (D10)
    // b -> PB3 (D11)
    // c -> PB4 (D12)
    // d -> PB5 (D13)
    DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);

    // e -> PC2 (A2)
    // f -> PC3 (A3)
    // seleccion display izquierdo -> PC4 (A4)
    // seleccion display derecho   -> PC5 (A5)
    DDRC |= (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5);

    // g -> PD0 (D0)
    DDRD |= (1 << PD0);

    // LED de alarma en D1
    DDRD |= (1 << LED_ALARM);

    // Inicio con la alarma apagada
    PORTD &= ~(1 << LED_ALARM);

    // Como mis displays son anodo comun, los apago dejando los comunes en 0
    PORTC &= ~((1 << PC4) | (1 << PC5));
}

void initADC(void)
{
    // REFS0 = 1  -> referencia AVcc
    // ADLAR = 1  -> justificado a la izquierda
    // MUX  = 0110 -> ADC6 (A6)
    ADMUX = (1 << REFS0) | (1 << ADLAR) | 0b00000110;

    // ADEN = 1   -> habilito el ADC
    // ADPS2:0=111 -> prescaler 128
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // No uso auto trigger
    ADCSRB = 0x00;
}

uint8_t ADC_read8(void)
{
    // Aqui inicio la conversion
    ADCSRA |= (1 << ADSC);

    // Espero a que termine
    while (ADCSRA & (1 << ADSC))
    {
        ;
    }

    // Como lo justifique a la izquierda, en ADCH tengo 8 bits utiles
    return ADCH;
}

void mostrarContador(uint8_t valor)
{
    // Bits 0 a 5 del contador -> PD2 a PD7
    // Yo conservo PD0 y PD1 porque ahi tengo segmento g y LED alarma
    PORTD = (PORTD & 0b00000011) | ((valor & 0b00111111) << 2);

    // Bits 6 y 7 del contador -> PC0 y PC1
    // Yo conservo PC2-PC5 porque ahi tengo parte de los displays
    PORTC = (PORTC & 0b11111100) | ((valor >> 6) & 0b00000011);
}

void revisarBotones(void)
{
    // Boton para incrementar
    if (!(PINB & (1 << BTN_INC)))
    {
        // Yo dejo un pequeño tiempo para antirrebote
        _delay_ms(25);

        if (!(PINB & (1 << BTN_INC)))
        {
            contador++;
            mostrarContador(contador);
            revisarAlarma(adcHex, contador);

            // Mientras el boton siga presionado, yo sigo refrescando todo
            while (!(PINB & (1 << BTN_INC)))
            {
                adcHex = ADC_read8();
                revisarAlarma(adcHex, contador);
                refrescarDisplays(adcHex);
            }

            // Este delay me ayuda a estabilizar al soltar
            _delay_ms(25);
        }
    }

    // Boton para decrementar
    if (!(PINB & (1 << BTN_DEC)))
    {
        _delay_ms(25);

        if (!(PINB & (1 << BTN_DEC)))
        {
            contador--;
            mostrarContador(contador);
            revisarAlarma(adcHex, contador);

            while (!(PINB & (1 << BTN_DEC)))
            {
                adcHex = ADC_read8();
                revisarAlarma(adcHex, contador);
                refrescarDisplays(adcHex);
            }

            _delay_ms(25);
        }
    }
}

void revisarAlarma(uint8_t valorADC, uint8_t valorContador)
{
    // Aqui comparo ambos valores.
    // Si el ADC es mayor, enciendo la alarma.
    if (valorADC > valorContador)
    {
        PORTD |= (1 << LED_ALARM);
    }
    else
    {
        PORTD &= ~(1 << LED_ALARM);
    }
}

uint8_t patronHex(uint8_t valor)
{
    switch (valor)
    {
        case 0x0: return 0x3F;
        case 0x1: return 0x06;
        case 0x2: return 0x5B;
        case 0x3: return 0x4F;
        case 0x4: return 0x66;
        case 0x5: return 0x6D;
        case 0x6: return 0x7D;
        case 0x7: return 0x07;
        case 0x8: return 0x7F;
        case 0x9: return 0x6F;
        case 0xA: return 0x77;
        case 0xB: return 0x7C;
        case 0xC: return 0x39;
        case 0xD: return 0x5E;
        case 0xE: return 0x79;
        default:  return 0x71; // F
    }
}

void escribirSegmentos(uint8_t patron)
{
    // Como mis displays son anodo comun, invierto la logica
    patron = (~patron) & 0x7F;

    // a,b,c,d -> PB2,PB3,PB4,PB5
    PORTB = (PORTB & 0b00000011) | ((patron & 0b00001111) << 2);

    // e,f -> PC2,PC3
    PORTC = (PORTC & 0b11110011) | (((patron >> 4) & 0b00000011) << 2);

    // g -> PD0
    if (patron & 0b01000000)
    {
        PORTD |= (1 << PD0);
    }
    else
    {
        PORTD &= ~(1 << PD0);
    }
}

void refrescarDisplays(uint8_t valorHex)
{
    uint8_t nibbleAlto;
    uint8_t nibbleBajo;

    nibbleAlto = (valorHex >> 4) & 0x0F;
    nibbleBajo = valorHex & 0x0F;

    // Yo apago ambos displays antes de cambiar segmentos
    PORTC &= ~((1 << PC4) | (1 << PC5));

    // Display izquierdo = nibble alto
    escribirSegmentos(patronHex(nibbleAlto));
    PORTC |= (1 << PC4);
    _delay_ms(2);

    // Vuelvo a apagar ambos
    PORTC &= ~((1 << PC4) | (1 << PC5));

    // Display derecho = nibble bajo
    escribirSegmentos(patronHex(nibbleBajo));
    PORTC |= (1 << PC5);
    _delay_ms(2);

    // Los apago al final del refresco
    PORTC &= ~((1 << PC4) | (1 << PC5));
}

/****************************************/
// Interrupt routines