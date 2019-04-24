/*
 * frackstock_v1.c
 *
 * Created: 21/04/2019 12:19:57
 * Author : Mario
 */ 

#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include "utils.h"
#include "twi.h"
#include "uart.h"
#include "adxl345.h"


#define MIN_RAMP_STEP 200
#define MAX_RAMP_STEP 400
#define RAMP_BOTTOM 10
#define MIN_RAMP_TOP 200
#define MAX_RAMP_TOP 250
#define MIN_RAMP_BOTTOM 30
#define MAX_RAMP_BOTTOM 50

#define LAMBDA 0.1f
#define TURNOVER_THRESHOLD 220



volatile uint8_t isr_flag = 0;

ISR(TIMER2_COMPA_vect)
{
	isr_flag = 1;
}


int16_t lowpass(int16_t value)
{
	static int16_t output = 0;

	output += (int16_t)(LAMBDA * (float)(value - output));
	return output;
}

int16_t clamp(int16_t value, int16_t min, int16_t max)
{
	if(value>max) value = max;
	if(value<min) value = min;
	return value;
}

uint8_t get_turnover_state(int16_t y)
{
	int16_t y_filt;
	
	y = clamp(y,-512, 511);
	y_filt = lowpass(y);
	//set_duty(0,Map(y,-512,511,0,255));
	//set_duty(1,Map(y_filt,-512,511,0,255));

	if(y_filt > TURNOVER_THRESHOLD)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


int main(void)
{
	int8_t err;
	int16_t y;
	uint16_t i;
	uint8_t up[4],top[4],bottom[4];
	uint16_t step[4], duty[4];
	uint16_t accel_count, accel_max_count;
	uint8_t turnover, turnover_old, leds_on;

	
	
	DDRD |= (1<<DDD6); //OC0A / D6
	DDRD |= (1<<DDD5); //OC0B / D5

	DDRB |= (1<<DDB1); //OC1A / D9
	DDRB |= (1<<DDB2); //OC1B / D10

	DDRB |= (1<<DDB0); //DEBUG PIN / D8
	PORTB &= ~(1<<DDB0);
	
	uart0_init(UART_BAUD_SELECT(115200, 16000000l));
	wait_1ms(1);
	
	twi_init();
	wait_1ms(1);
	
	accel_init();
	wait_1ms(1);
	
	
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
	
	sei();
	timer2_int_2ms_init();
	timer2_int_2ms_start();

	accel_max_count = 25;
	accel_count = 0;
	
	turnover = 0;
	turnover_old = 0;
	leds_on = 0;

	while(1)
	{
		if(isr_flag)
		{
			isr_flag = 0;
			// calculate led output
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
			
			// get sensor data
			accel_count++;
			if(accel_count >= accel_max_count)
			{
				accel_count = 0;
				err = accel_y(&y);
				turnover = get_turnover_state(y);
				if(turnover && !turnover_old)
				{
					if(leds_on)
					{
						leds_on = 0;
						pwm_stop();
						
					}
					else
					{
						leds_on = 1;
						pwm_init();
						pwm_start();
					}
				}
				turnover_old = turnover;
			}
		}
	}
}