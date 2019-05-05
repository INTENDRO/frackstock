/*
 * frackstock.c
 *
 * Created: 16/04/2019 21:42:56
 * Author : Mario
 *
 * - Flicker (Twinkle) Test
 * - Accelerometer Test
 *
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

//#define FLICKER_TEST
#define ACCEL_TEST




#ifdef ACCEL_TEST

int main(void)
{
	char uart_send[64];
	int16_t value = 1;
	int16_t x,y,z;
	int8_t err = 10;

	DDRB |= (1<<DDB0);
	PORTB &= ~(1<<DDB0);
	
	uart0_init(UART_BAUD_SELECT(115200, 16000000l));
	wait_1ms(1);
	
	twi_init();
	wait_1ms(1);
	
	accel_init();
	wait_1ms(10);
	

	while(1)
	{
		err = accel_xyz(&x, &y, &z);
		sprintf(uart_send, "err = %d\r\n",err);
		uart0_puts(uart_send);
		sprintf(uart_send, "x = %d y = %d z = %d\r\n", x, y, z);
		uart0_puts(uart_send);
		wait_1ms(500);
	}
	
	while(1)
	{
		err = accel_x(&value);
		sprintf(uart_send, "err = %d\r\n",err);
		uart0_puts(uart_send);
		sprintf(uart_send, "value = %d\r\n",value);
		uart0_puts(uart_send);
		wait_1ms(500);
	}
}

#endif

#ifdef FLICKER_TEST
int main(void)
{
    
	uint8_t i;
	uint8_t up[4],top[4],bottom[4];
	uint16_t step[4], duty[4];
	
	DDRD |= (1<<DDD6); //OC0A
	DDRD |= (1<<DDD5); //OC0B

	DDRB |= (1<<DDB1); //OC1A
	DDRB |= (1<<DDB2); //OC1B


	DDRB |= (1<<DDB0); //DEBUG PIN
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
}
#endif

