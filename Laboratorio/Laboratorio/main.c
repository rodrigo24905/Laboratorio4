/*
 * NombreProgra.c
 *
 * Created:
 * Author:
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

uint8_t ADC_read8(void);

uint8_t patronHex(uint8_t valor);
void escribirSegmentos(uint8_t patron);
void refrescarDisplays(uint8_t valorHex);

/****************************************/
// Main Function

#define BTN_INC PB0      // D8
#define BTN_DEC PB1      // D9

uint8_t contador = 0;
uint8_t adcHex = 0;

int main(void)
{
    initGPIO();
    initADC();

    mostrarContador(contador);
    adcHex = ADC_read8();

    while (1)
    {
        refrescarDisplays(adcHex);
        revisarBotones();
        mostrarContador(contador);
        adcHex = ADC_read8();
    }
}

/****************************************/
// NON-Interrupt subroutines

void initGPIO(void)
{
    // -----------------------------
    // Parte 1: contador en LEDs
    // D2-D7 -> PD2-PD7
    // A0-A1 -> PC0-PC1
    // -----------------------------
    DDRD |= 0b11111100;
    DDRC |= 0b00000011;

    // -----------------------------
    // Botones en D8 y D9
    // D8 -> PB0
    // D9 -> PB1
    // -----------------------------
    DDRB &= ~((1 << BTN_INC) | (1 << BTN_DEC));
    PORTB |= (1 << BTN_INC) | (1 << BTN_DEC);   // pull-up interno

    // -----------------------------
    // Displays 7 segmentos
    // a -> PB2 (D10)
    // b -> PB3 (D11)
    // c -> PB4 (D12)
    // d -> PB5 (D13)
    // e -> PC2 (A2)
    // f -> PC3 (A3)
    // g -> PD0 (D0)
    //
    // Display izquierdo  -> PC4 (A4)
    // Display derecho    -> PC5 (A5)
    //
    // Asumido: cátodo común
    // Activo en bajo para seleccionar display
    // -----------------------------
    DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);
    DDRC |= (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5);
    DDRD |= (1 << PD0);

    // Apagar displays al inicio
    PORTC |= (1 << PC4) | (1 << PC5);
}

void initADC(void)
{
    // REFS0 = 1 -> referencia AVcc
    // ADLAR = 1 -> justificado a la izquierda
    // MUX[3:0] = 0110 -> ADC6 (A6)
    ADMUX = (1 << REFS0) | (1 << ADLAR) | 0b00000110;

    // ADEN = 1 -> habilita ADC
    // ADPS2:0 = 111 -> prescaler 128
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Sin auto-trigger
    ADCSRB = 0x00;
}

uint8_t ADC_read8(void)
{
    // Inicia conversión
    ADCSRA |= (1 << ADSC);

    // Espera a que termine
    while (ADCSRA & (1 << ADSC))
    {
        ;
    }

    // Como ADLAR = 1, ADCH contiene los 8 bits más significativos
    return ADCH;
}

void mostrarContador(uint8_t valor)
{
    // Bits 0..5 -> PD2..PD7
    PORTD = (PORTD & 0b00000011) | ((valor & 0b00111111) << 2);

    // Bits 6..7 -> PC0..PC1
    // Se preservan PC2..PC5 porque ahí están los displays
    PORTC = (PORTC & 0b11111100) | ((valor >> 6) & 0b00000011);
}

void revisarBotones(void)
{
    // Botón incrementar
    if (!(PINB & (1 << BTN_INC)))
    {
        _delay_ms(20);

        if (!(PINB & (1 << BTN_INC)))
        {
            contador++;
            mostrarContador(contador);

            while (!(PINB & (1 << BTN_INC)))
            {
                refrescarDisplays(adcHex);
            }

            _delay_ms(20);
        }
    }

    // Botón decrementar
    if (!(PINB & (1 << BTN_DEC)))
    {
        _delay_ms(20);

        if (!(PINB & (1 << BTN_DEC)))
        {
            contador--;
            mostrarContador(contador);

            while (!(PINB & (1 << BTN_DEC)))
            {
                refrescarDisplays(adcHex);
            }

            _delay_ms(20);
        }
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
    // patron bit0=a, bit1=b, ..., bit6=g

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

    // Apaga ambos displays
    PORTC |= (1 << PC4) | (1 << PC5);

    // Display izquierdo
    escribirSegmentos(patronHex(nibbleAlto));
    PORTC &= ~(1 << PC4);
    _delay_ms(2);

    // Apaga ambos displays
    PORTC |= (1 << PC4) | (1 << PC5);

    // Display derecho
    escribirSegmentos(patronHex(nibbleBajo));
    PORTC &= ~(1 << PC5);
    _delay_ms(2);

    // Apaga ambos displays al terminar el refresco
    PORTC |= (1 << PC4) | (1 << PC5);
}

/****************************************/
// Interrupt routines