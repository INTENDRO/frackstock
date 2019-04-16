/*
 * frackstock.c
 *
 * Created: 16/04/2019 21:42:56
 * Author : Mario
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stddef.h>
#include "twi.h"




ISR(TIMER1_COMPA_vect)
{
	//PORTB ^= (1<<PORTB0);
}

void INT_1ms_setup(void)
{
	TCCR1A = 0b00000000;
	TCCR1B = 0b00001000;
	TIMSK1 = 0b00000010;
	TIFR1  = 0b00000111;

	TCNT1 = 0;
	OCR1A = 16000;
}

void INT_1ms_start(void)
{
	TCCR1B |= 0x01;
}

void INT_1ms_stop(void)
{
	TCCR1B &= ~0x07;
}



void wait_1ms(uint8_t factor)
{
	uint16_t i;
	TCCR1A = 0b00000000;
	TCCR1B = 0b00001000;
	TIMSK1 = 0b00000000;
	TIFR1 =  0b00000111;
	TCNT1 = 0;
	OCR1A = 16000;
	
	TCCR1B |= 0x01;
	
	for(i=0;i<factor;i++)
	{
		while(!(TIFR1&0x02));
		TIFR1 = 0x07;
	}
	TCCR1B &= ~(0x07);
}

void twi_cb(uint8_t addr, uint8_t* data)
{
	
	
}

int main(void)
{
    uint8_t send[] = {0x01,0x02,0x03};
	uint8_t* twi_data;
	
	DDRB |= (1<<DDB0);
	PORTB &= ~(1<<DDB0);
	
	twi_init();
	//INT_1ms_setup();
	sei();
	
	wait_1ms(1000);
	
	twi_write(0x52, send, sizeof(send), NULL);
	twi_data = twi_wait();
	
	PORTB |= (1<<DDB0);
	wait_1ms(TW_STATUS);
	PORTB &= ~(1<<DDB0);
	
    while (1) 
    {
		wait_1ms(1);
    }
}

