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
#include <stdint.h>
#include "twi.h"


#define MIN_RAMP_STEP 200
#define MAX_RAMP_STEP 500
#define RAMP_BOTTOM 10
#define MIN_RAMP_TOP 70
#define MAX_RAMP_TOP 100
#define MIN_RAMP_BOTTOM 40
#define MAX_RAMP_BOTTOM 70



void pin_debug(uint8_t value);
void pin_debug_array(uint8_t* data, uint8_t length);

// ISR(TIMER1_COMPA_vect)
// {
// 	//PORTB ^= (1<<PORTB0);
// }
// 
// void INT_1ms_setup(void)
// {
// 	TCCR1A = 0b00000000;
// 	TCCR1B = 0b00001000;
// 	TIMSK1 = 0b00000010;
// 	TIFR1  = 0b00000111;
// 
// 	TCNT1 = 0;
// 	OCR1A = 16000;
// }
// 
// void INT_1ms_start(void)
// {
// 	TCCR1B |= 0x01;
// }
// 
// void INT_1ms_stop(void)
// {
// 	TCCR1B &= ~0x07;
// }



// void wait_1ms(uint16_t factor)
// {
// 	uint16_t i;
// 	TCCR1A = 0b00000000;
// 	TCCR1B = 0b00001000;
// 	TIMSK1 = 0b00000000;
// 	TIFR1 =  0b00000111;
// 	TCNT1 = 0;
// 	OCR1A = 16000;
//
// 	TCCR1B |= 0x01;
//
// 	for(i=0;i<factor;i++)
// 	{
// 		while(!(TIFR1&0x02));
// 		TIFR1 = 0x07;
// 	}
// 	TCCR1B &= ~(0x07);
// }

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

void timer1_pwm_init(void)
{
	TCCR1A = 0b10100001;
	TCCR1B = 0b00001000;
	TIMSK1 = 0b00000000;

	TCNT1 = 0;
	OCR1A = 0;
	OCR1B = 0;
}

// void timer2_pwm_init(void)
// {
// 	TCCR2A = 0b10100011;
// 	TCCR2B = 0b00000000;
// 	TIMSK2 = 0b00000000;
// 
// 	TCNT2 = 0;
// 	OCR2A = 0;
// 	OCR2B = 0;
// }

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

// void timer2_pwma_set_duty(uint8_t duty)
// {
// 	OCR2A = duty;
// }
// 
// void timer2_pwmb_set_duty(uint8_t duty)
// {
// 	OCR2B = duty;
// }

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

// void timer2_pwm_start(void)
// {
// 	TCCR2B |= 0x02; //8kHz pwm @ 16MHz
// }
// 
// void timer2_pwm_stop(void)
// {
// 	TCCR2B &= ~0x07;
// }

uint16_t randreg = 10;

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
			//timer2_pwma_set_duty(duty);
			timer1_pwma_set_duty(duty);
			break;

		case 3:
			//timer2_pwmb_set_duty(duty);
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
	//timer2_pwm_init();
}

void pwm_start(void)
{
	timer0_pwm_start();
	timer1_pwm_start();
	//timer2_pwm_start();
}

void pwm_stop(void)
{
	timer0_pwm_stop();
	timer1_pwm_stop();
	//timer2_pwm_stop();
}

int main(void)
{
    uint8_t send[] = {0x01,0x02,0x03};
	twi_report_t* twi_report;
	uint8_t i;

	uint8_t up[4],top[4],bottom[4];
	uint16_t step[4], duty[4];
	
	DDRD |= (1<<DDD6); //OC0A
	DDRD |= (1<<DDD5); //OC0B

	DDRB |= (1<<DDB1); //OC1A
	DDRB |= (1<<DDB2); //OC1B



	DDRB |= (1<<DDB0);
	PORTB &= ~(1<<DDB0);
	
	pwm_init();

	for(i=0;i<4;i++)
	{
		step[i] = get_rand(MIN_RAMP_STEP,MAX_RAMP_STEP);
		top[i] = get_rand(MIN_RAMP_TOP, MAX_RAMP_TOP);
		bottom[i] = get_rand(MIN_RAMP_BOTTOM, MAX_RAMP_BOTTOM);

		up[i] = 1;
		duty[i] = 0;

		set_duty(i,bottom[i]);
	}

	pwm_start();


	while(1)
	{
		for(i=0; i<4; i++)
		{
			if(up[i])
			{
				if((UINT16_MAX-duty[i])<step[i])
				{
					duty[i] = UINT16_MAX;
					up[i] = 0;

					//NEW CALCULATION
					step[i] = get_rand(MIN_RAMP_STEP,MAX_RAMP_STEP);
					bottom[i] = get_rand(MIN_RAMP_BOTTOM, MAX_RAMP_BOTTOM);
				}
				else
				{
					duty[i] += step[i];
				}
			}
			else
			{
				if((duty[i])<step[i])
				{
					duty[i] = 0;
					up[i] = 1;

					//NEW CALCULATION
					step[i] = get_rand(MIN_RAMP_STEP,MAX_RAMP_STEP);
					top[i] = get_rand(MIN_RAMP_TOP, MAX_RAMP_TOP);
				}
				else
				{
					duty[i] -= step[i];
				}
			}
			set_duty(i,Map(duty[i],0,UINT16_MAX,bottom[i],top[i]));
		}
		
		
		PORTB &= ~(1<<DDB0);
		wait_1ms(1);
		PORTB |= (1<<DDB0);
	}

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

