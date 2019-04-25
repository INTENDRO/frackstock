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
#include <avr/wdt.h>
#include <util/twi.h>
#include "utils.h"
#include "twi.h"
#include "adxl345.h"
#include "itg3205.h"


#define MIN_RAMP_STEP 400
#define MAX_RAMP_STEP 800
#define RAMP_BOTTOM 10
#define MIN_RAMP_TOP 200
#define MAX_RAMP_TOP 250
#define MIN_RAMP_BOTTOM 30
#define MAX_RAMP_BOTTOM 50

#define LAMBDA 0.1f
#define TURNOVER_THRESHOLD 220


typedef enum
{
	OFF,
	ON
}mode_t;

mode_t mode = ON;

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
	//set_duty(0,Map(y,-512,511,0,255)); //debug output
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


int8_t read_imu(int16_t* y, uint8_t* turnover, uint8_t* tap)
{
	int8_t err;
	
	err = accel_y(y);
	if(err)
	{
		return -1;
	}
	
	*turnover = get_turnover_state(*y);

	err = accel_tap(tap);
	if(err)
	{
		return -2;
	}

	
	return 0;
}

void wait_isr_count(uint16_t count)
{
	uint16_t i;
	
	for(i=0; i<count; i++)
	{
		while(!isr_flag);
		isr_flag = 0;
	}
}

void set_all_led_pins(uint8_t value)
{
	if(value)
	{
		PORTB |= (1<<PORTB1);
		PORTB |= (1<<PORTB2);
		PORTD |= (1<<PORTD6);
		PORTD |= (1<<PORTD5);
	}
	else
	{
		PORTB &= ~(1<<PORTB1);
		PORTB &= ~(1<<PORTB2);
		PORTD &= ~(1<<PORTD6);
		PORTD &= ~(1<<PORTD5);
	}
}


int main(void)
{
	int8_t err;
	int16_t y;
	uint8_t tap, connected;
	uint16_t i;
	uint8_t up[4],top[4],bottom[4];
	uint16_t step[4], duty[4];
	uint16_t imu_count, imu_max_count;
	uint8_t turnover, turnover_old, leds_on;
	uint8_t watchdog_reset;
	
	
	if(MCUSR & (1<<WDRF))
	{
		watchdog_reset = 1; //watchdog reset occurred!
	}
	else
	{
		watchdog_reset = 0;
	}
	MCUSR = 0x0F;
	
	WDTCSR |= (1<<WDCE) | (1<<WDE); //start watchdog write sequence
	WDTCSR = (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0); //2s timeout
	
	
	DDRD |= (1<<DDD6); //OC0A / D6
	DDRD |= (1<<DDD5); //OC0B / D5

	DDRB |= (1<<DDB1); //OC1A / D9
	DDRB |= (1<<DDB2); //OC1B / D10

	DDRB |= (1<<DDB0); //DEBUG PIN / D8
	PORTB &= ~(1<<DDB0);
	
	DDRD |= (1<<DDB7);
	PORTD &= ~(1<<PORTD7); //imu power supply
	
	//set all unused pins to inputs with pullups (minimize current consumption)
	DDRB &= ~(1<<DDB3) & ~(1<<DDB4) & ~(1<<DDB5);
	PORTB |= (1<<PORTB3) | (1<<PORTB4) | (1<<PORTB5);
	
	DDRC &= ~(1<<DDC0) & ~(1<<DDC1) & ~(1<<DDC2) & ~(1<<DDC3);
	PORTC |= (1<<PORTC0) | (1<<PORTC1) | (1<<PORTC2) | (1<<PORTC3);
	
	DDRD &= ~(1<<DDD0) & ~(1<<DDD1) & ~(1<<DDD2) & ~(1<<DDD3) & ~(1<<DDD4);
	PORTD |= (1<<PORTD0) | (1<<PORTD1) | (1<<PORTD2) | (1<<PORTD3) | (1<<PORTD4);
	
	//turn off unused modules
	PRR |= (1<<PRADC) | (1<<PRUSART0);
	
	//disable digital buffer of ADC3 - ADC0
	DIDR0 = 0b00001111; 
	
	wait_1ms(100);
	PORTD |= (1<<PORTD7);
	
	twi_init();
	wait_1ms(1);
	
	accel_init();
	wait_1ms(1);
	
	gyro_init();
	wait_1ms(1);
	

	for(i=0;i<4;i++)
	{
		step[i] = get_rand(MIN_RAMP_STEP,MAX_RAMP_STEP);
		top[i] = get_rand(MIN_RAMP_TOP, MAX_RAMP_TOP);
		bottom[i] = get_rand(MIN_RAMP_BOTTOM, MAX_RAMP_BOTTOM);

		up[i] = 1;
		duty[i] = 0;
	}
	
	sei();
	
	timer2_int_5ms_init();
	timer2_int_5ms_start();
	
	if(watchdog_reset)
	{
		for(i=0;i<20;i++)
		{
			set_all_led_pins(1);
			wait_isr_count(20);
			set_all_led_pins(0);
			wait_isr_count(20);
			asm("wdr");
		}
	}
	

	imu_max_count = 10; 
	imu_count = 0;
	
	turnover = 0;
	turnover_old = 0;
	leds_on = 0;
	
	pwm_init();
	pwm_start();
	

	while(1)
	{
		if(isr_flag)
		{
			isr_flag = 0;
			//PORTB |= (1<<DDB0);
			
			// get imu data
			imu_count++;
			if(imu_count >= imu_max_count)
			{
				imu_count = 0;
				
				err = read_imu(&y, &turnover, &tap);
				if(err == 0)
				{
					
					if(turnover != turnover_old)
					{
						if(turnover) //entering menu
						{
							connected = are_pwm_pins_connected();
							pwm_disconnect_pins();
							for(i=0;i<5;i++)
							{
								set_all_led_pins(1);
								wait_isr_count(20);
								set_all_led_pins(0);
								wait_isr_count(20);
							}
							if(connected)
							{
								pwm_connect_pins();
							}
							
						}
						else //leaving menu
						{
							
						
						}
						turnover_old = turnover;
					}
					
					if(turnover && tap)
					{
						switch(mode)
						{
							case OFF:
							pwm_connect_pins();
							pwm_start();
							mode = ON;
							break;
							
							case ON:
							pwm_stop();
							pwm_disconnect_pins();
							set_all_led_pins(0);
							mode = OFF;
							break;
						}
					}
				}
				
			}
			
			
			
			//display based on mode
			switch(mode)
			{
				case OFF:
				
				break;
				
				
				case ON:
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
				break;
			}
			//PORTB &= ~(1<<DDB0);
		}
		asm("wdr");
	}
}

