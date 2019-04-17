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


void pin_debug(uint8_t value);
void pin_debug_array(uint8_t* data, uint8_t length);

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



void wait_1ms(uint16_t factor)
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

void twi_cb(twi_report_t* report)
{
	pin_debug(report->error);
	pin_debug(report->length);
	pin_debug_array(report->data,report->length);
	
}

void pin_debug(uint8_t value)
{
	PORTB |= (1<<DDB0);
	wait_1ms(value);
	PORTB &= ~(1<<DDB0);
	wait_1ms(1);
}

void pin_debug_array(uint8_t* data, uint8_t length)
{
	uint8_t i;
	
	for(i=0; i<length; i++)
	{
		pin_debug(data[i]);
	}
}


void timer0_pwm_init(void)
{
	TCCR0A = 0b10100011;
	TCCR0B = 0b00000000;
	TIMSK0 = 0b00000000;

	TCNT0 = 0;
	OCR0A = 0;
	OCR0B = 0;
}

void timer0_pwma_set_duty(uint8_t duty)
{
	OCR0A = duty;
}

void timer0_pwmb_set_duty(uint8_t duty)
{
	OCR0B = duty;
}

void timer0_pwm_start(void)
{
	TCCR0B |= 0x05;
}

void timer0_pwm_stop(void)
{
	TCCR0B &= ~0x07;
}

int main(void)
{
    uint8_t send[] = {0x01,0x02,0x03};
	twi_report_t* twi_report;
	//uint8_t i;
	
	DDRD |= (1<<DDD6);

	DDRB |= (1<<DDB0);
	PORTB &= ~(1<<DDB0);
	
	timer0_pwm_init();
	timer0_pwma_set_duty(10);
	timer0_pwm_start();

	twi_init();
	//INT_1ms_setup();
	sei();
	
	wait_1ms(1000);
	
	twi_write(0x53, send, sizeof(send), twi_cb);
	//twi_write(0x53, send, sizeof(send), NULL);
	//twi_report = twi_wait();
	
// 	pin_debug(twi_report->error);
// 	pin_debug(twi_report->length);
// 	pin_debug_array(twi_report->data,twi_report->length);

	while(1)
	{
		timer0_pwma_set_duty(200);
		wait_1ms(1000);
		timer0_pwma_set_duty(10);
		wait_1ms(1000);
	}
	
    while (1) 
    {
		PORTB ^= (1<<PORTB0);
    }
}

