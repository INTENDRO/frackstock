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

#define TURNAROUND_ACCEL_THRESHOLD 100
#define TURNAROUND_MEASUREMENT_AMOUNT 1000u
#define TURNAROUND_THRESHOLD 900u

#define TURNAROUND_INTEGRATE_MAX 1000
#define TURNAROUND_UPPER 800
#define TURNAROUND_LOWER 700



volatile uint8_t isr_flag = 0;

ISR(TIMER2_COMPA_vect)
{
	isr_flag = 1;
}


int main(void)
{
	int8_t err;
	int16_t x,y,z;
	uint16_t i;
	uint8_t up[4],top[4],bottom[4];
	uint16_t step[4], duty[4];
	uint16_t accel_count, accel_max_count;
	uint8_t turnaround_array[TURNAROUND_MEASUREMENT_AMOUNT];
	uint16_t turnaround_index, turnaround_temp;
	uint16_t turnaround_integrate;
	uint8_t turnaround;
	
	
	DDRD |= (1<<DDD6); //OC0A
	DDRD |= (1<<DDD5); //OC0B

	DDRB |= (1<<DDB1); //OC1A
	DDRB |= (1<<DDB2); //OC1B


	DDRB |= (1<<DDB0); //DEBUG PIN
	PORTB &= ~(1<<DDB0);
	
	
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

	pwm_start();
	
	
	sei();
	timer2_int_2ms_init();
	timer2_int_2ms_start();

	accel_max_count = 5;
	accel_count = 0;
	
	for(i=0; i<TURNAROUND_MEASUREMENT_AMOUNT; i++)
	{
		turnaround_array[i] = 0;
	}
	turnaround_index = 0;
	turnaround = 0;
	turnaround_integrate = 0;

	while(1)
	{
		if(isr_flag)
		{
			isr_flag = 0;
			//PORTB |= (1<<PORTB0);
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
				
				if(y > TURNAROUND_ACCEL_THRESHOLD)
				{
					if(turnaround_integrate<TURNAROUND_INTEGRATE_MAX)
					{
						turnaround_integrate++;
					}
					
				}
				else
				{
					if(turnaround_integrate>0)
					{
						turnaround_integrate--;
					}
				}
				
				if(turnaround_integrate > TURNAROUND_UPPER)
				{
					PORTB |= (1<<PORTB0);
				}
				
				if(turnaround_integrate < TURNAROUND_LOWER)
				{
					PORTB &= ~(1<<PORTB0);
				}
				
// 				if(y > TURNAROUND_ACCEL_THRESHOLD)
// 				{
// 					turnaround_array[turnaround_index] = 1;
// 				}
// 				else
// 				{
// 					turnaround_array[turnaround_index] = 0;
// 				}
// 				
// 				turnaround_index++;
// 				if(turnaround_index >= TURNAROUND_MEASUREMENT_AMOUNT)
// 				{
// 					turnaround_index = 0;
// 				}
// 				
// 				turnaround_temp = 0;
// 				for(i=0; i<TURNAROUND_MEASUREMENT_AMOUNT; i++)
// 				{
// 					turnaround_temp += turnaround_array[i];
// 				}
// 				
// 				if(turnaround_temp > TURNAROUND_THRESHOLD)
// 				{
// 					PORTB |= (1<<PORTB0);
// 				}
// 				else
// 				{
// 					PORTB &= ~(1<<PORTB0);
// 				}
			}
			
			//PORTB &= ~(1<<PORTB0);
			
			
		}
	}
}