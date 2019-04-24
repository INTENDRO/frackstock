/*
 * utils.c
 *
 * Created: 21/04/2019 10:49:32
 *  Author: Mario
 */ 

#include <avr/io.h>
#include "utils.h"


static uint16_t randreg = 10;


void timer2_int_2ms_init(void)
{
	TCCR2A = 0b00000010;
	TCCR2B = 0b00000000;
	TIMSK2 = 0b00000010;
	TIFR2 =  0b00000111;
	TCNT2 = 0;
	OCR2A = 250;
}

void timer2_int_2ms_start(void)
{
	TCCR2B |= 0x05;
}

void timer2_int_2ms_stop(void)
{
	TCCR2B &= ~0x07;
}

void wait_1ms(uint16_t factor)
{
	uint16_t i;
	TCCR2A = 0b00000010;
	TCCR2B = 0b00000000;
	TIMSK2 = 0b00000000;
	TIFR2 =  0b00000111;
	TCNT2 = 0;
	OCR2A = 250;

	TCCR2B |= 0x04;

	for(i=0;i<factor;i++)
	{
		while(!(TIFR2&0x02));
		TIFR2 = 0x07;
	}
	TCCR2B &= ~(0x07);
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

void timer1_pwm_init(void)
{
	TCCR1A = 0b10100001;
	TCCR1B = 0b00001000;
	TIMSK1 = 0b00000000;

	TCNT1 = 0;
	OCR1A = 0;
	OCR1B = 0;
}


void timer0_pwma_set_duty(uint8_t duty)
{
	OCR0A = duty;
}

void timer0_pwmb_set_duty(uint8_t duty)
{
	OCR0B = duty;
}

void timer1_pwma_set_duty(uint8_t duty)
{
	OCR1A = duty;
}

void timer1_pwmb_set_duty(uint8_t duty)
{
	OCR1B = duty;
}


void timer0_pwm_start(void)
{
	TCCR0B |= 0x02; //8kHz pwm @ 16MHz
}

void timer0_pwm_stop(void)
{
	TCCR0B &= ~0x07;
}

void timer1_pwm_start(void)
{
	TCCR1B |= 0x02; //8kHz pwm @ 16MHz
}

void timer1_pwm_stop(void)
{
	TCCR1B &= ~0x07;
}

void set_duty(uint8_t num, uint8_t duty)
{
	switch(num)
	{
		case 0:
		timer0_pwma_set_duty(duty);
		break;

		case 1:
		timer0_pwmb_set_duty(duty);
		break;

		case 2:
		timer1_pwma_set_duty(duty);
		break;

		case 3:
		timer1_pwmb_set_duty(duty);
		break;

		default:
		break;
	}
}

void pwm_init(void)
{
	timer0_pwm_init();
	timer1_pwm_init();
}

void pwm_start(void)
{
	timer0_pwm_start();
	timer1_pwm_start();
}

void pwm_stop(void)
{
	timer0_pwm_stop();
	timer1_pwm_stop();

	TCCR0A = 0;
	TCNT0 = 0;

	TCCR1A = 0;
	TCNT1 = 0;
	PORTD &= ~(1<<PORTD6);
	PORTD &= ~(1<<PORTD5);
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
}




uint16_t pseudorandom16 (void)
{
	uint16_t newbit = 0;

	if (randreg == 0) {
		randreg = 1;
	}
	if (randreg & 0x8000) newbit = 1;
	if (randreg & 0x4000) newbit ^= 1;
	if (randreg & 0x1000) newbit ^= 1;
	if (randreg & 0x0008) newbit ^= 1;
	randreg = (randreg << 1) + newbit;
	return randreg;
}

uint16_t get_rand(uint16_t start, uint16_t stop)
{
	uint16_t rand;
	rand = start + pseudorandom16()%(stop - start);
	return rand;
}

int32_t Map(int32_t data, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
	return((data-in_min)*(out_max-out_min)/(in_max-in_min)+out_min);
}




