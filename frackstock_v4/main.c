/*
 * frackstock_v4.c
 *
 * Created: 21/04/2019 12:19:57
 * Author : Mario
 *
 * code base: frackstock_v2
 * - OFF
 * - ON
 * - TWINKLE
 * - SOS
 * - tap and doubletap works
 * - turning to increase/decrease settings
 * - acceleration mode (differentiating the sensor output)
 * - acceleration twinkle mode (differentiating the sensor output)
 * - acceleration threshold mode (differentiating the sensor output)
 * 
 */ 



#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/twi.h>
#include "utils.h"
#include "twi.h"
#include "adxl345.h"
#include "itg3205.h"


#define MIN_RAMP_STEP 400
#define MAX_RAMP_STEP 800

#define RAMP_STEP_MEAN_DEFAULT 600
#define RAMP_STEP_MEAN_MIN 300
#define RAMP_STEP_MEAN_MAX 2000
#define RAMP_STEP_SIGMA 0.5f

#define RAMP_BOTTOM 10
#define MIN_RAMP_TOP 200
#define MAX_RAMP_TOP 250
#define MIN_RAMP_BOTTOM 30
#define MAX_RAMP_BOTTOM 50

#define ON_BRIGHTNESS_DEFAULT 32000

#define ACCEL_LAMBDA 0.001f
#define ACCEL_MIN_BRIGHTNESS 10
#define ACCEL_ABS_RANGE 250 // maximum expected output of accelerometer (absolute)

#define ACCEL_TWINKLE_LAMBDA 0.001f
#define ACCEL_TWINKLE_ABS_RANGE 250 // maximum expected output of accelerometer (absolute)

#define ACCEL_THRESH_THRESHOLD 150
#define ACCEL_THRESH_STEP 600

#define SOS_DIT_LENGTH 30

#define TURNOVER_LAMBDA 0.1f
#define TURNOVER_THRESHOLD_ON 220
#define TURNOVER_THRESHOLD_OFF 50

#define SINGLE_TAP_IGNORE_DURATION 4
#define DOUBLE_TAP_IGNORE_DURATION 4


typedef enum
{
	OFF,
	ON,
	TWINKLE,
	ACCEL,
	ACCEL_TWINKLE,
	ACCEL_THRESH,
	SOS
}mode_t;

mode_t mode = ON;

volatile uint8_t isr_flag = 0;

ISR(TIMER2_COMPA_vect)
{
	isr_flag = 1;
}


int16_t lowpass(float lambda, int16_t new_value, int16_t old_value)
{
	old_value += (int16_t)(TURNOVER_LAMBDA * (float)(new_value - old_value));
	return old_value;
}

int16_t clamp(int16_t value, int16_t min, int16_t max)
{
	if(value>max) value = max;
	if(value<min) value = min;
	return value;
}

int16_t max(int16_t a, int16_t b)
{
	if(a>b)
	{
		return a;
	}
	else
	{
		return b;
	}
}

int16_t get_diff(int16_t value)
{
	static int16_t last_value = 0;
	int16_t diff;
	
	diff = value-last_value;
	last_value = value;
	
	return diff;
}

uint8_t get_turnover_state(int16_t y, uint8_t current_state)
{
	static int16_t y_filt = 0;
	
	y = clamp(y,-512, 511);
	y_filt = lowpass(TURNOVER_LAMBDA, y, y_filt);
	//set_duty(0,Map(y,-512,511,0,255)); //debug output
	//set_duty(1,Map(y_filt,-512,511,0,255));

	if(current_state)
	{
		if(y_filt < TURNOVER_THRESHOLD_OFF)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if(y_filt > TURNOVER_THRESHOLD_ON)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
}


int8_t read_accel(int16_t* y, int16_t* y_diff, uint8_t* turnover, uint8_t* single_tap, uint8_t* double_tap, uint8_t current_turnover_state)
{
	int8_t err;
	
	err = accel_y(y);
	if(err)
	{
		return -1;
	}
	
	*turnover = get_turnover_state(*y, current_turnover_state);
	
	*y_diff = get_diff(*y);

	err = accel_tap(single_tap, double_tap);
	if(err)
	{
		return -2;
	}

	
	return 0;
}

int8_t read_gyro(int16_t* y)
{
	int8_t err;
	
	err = gyro_y(y);
	if(err)
	{
		return -1;
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


void twinkle_setup(uint8_t* up, uint8_t* top, uint8_t* bottom, uint16_t* step, uint16_t* duty, uint16_t ramp_step_mean)
{
	uint8_t i;
	
	for(i=0;i<4;i++)
	{
		step[i] = get_rand(ramp_step_mean*(1-RAMP_STEP_SIGMA), ramp_step_mean*(1+RAMP_STEP_SIGMA)+1);
		top[i] = get_rand(MIN_RAMP_TOP, MAX_RAMP_TOP);
		bottom[i] = get_rand(MIN_RAMP_BOTTOM, MAX_RAMP_BOTTOM);

		up[i] = 1;
		duty[i] = 0;
	}
}

void sos_setup(uint16_t* count, uint8_t* stage)
{
	*count = 0;
	*stage = 0;
}


int main(void)
{
	int8_t err;
	int16_t y, y_diff, y_velocity;
	uint8_t single_tap, double_tap, connected;
	uint8_t single_tap_ignore, double_tap_ignore;
	uint16_t i;
	uint8_t up[4],top[4],bottom[4];
	uint16_t step[4], duty[4];
	uint16_t ramp_step_mean, ramp_step_mean_fullscale;
	uint16_t accel_count, accel_max_count;
	uint16_t gyro_count, gyro_max_count;
	uint8_t turnover, turnover_old;
	uint8_t adjustmode;
	uint16_t adjust_level;
	uint8_t watchdog_reset;
	uint16_t on_brightness = ON_BRIGHTNESS_DEFAULT;
	int16_t accel_output, accel_temp, accel_output_filt = 0;
	int16_t accel_twinkle_output, accel_twinkle_temp, accel_twinkle_output_filt = 0;
	int16_t accel_thresh_output;
	uint16_t accel_thresh_count;
	uint16_t sos_count, sos_temp;
	uint8_t sos_stage;
	
	
	
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
	
	

	accel_max_count = 10; 
	accel_count = 0;
	
	turnover = 0;
	turnover_old = 0;
	adjustmode = 0;
	adjust_level = 0;
	
	single_tap_ignore = 0;
	
	ramp_step_mean = RAMP_STEP_MEAN_DEFAULT;
	ramp_step_mean_fullscale = Map(RAMP_STEP_MEAN_DEFAULT, RAMP_STEP_MEAN_MIN, RAMP_STEP_MEAN_MAX, 0, UINT16_MAX);
	
	pwm_init();
	
	//only needed if started directly with on (during coding)
	pwm_connect_pins();
	pwm_start();
	for(i=0;i<4;i++)
	{
		set_duty(i,Map(on_brightness, 0, UINT16_MAX, 0, UINT8_MAX));
	}


	//only needed if started directly with twinkle (during coding)
	//pwm_connect_pins();
	//pwm_start();
	//twinkle_setup(up,top,bottom,step,duty);
	
	//only needed if started directly with sos (during coding)
	//sos_setup(&sos_count, &sos_stage);
	

	while(1)
	{
		if(isr_flag)
		{
			isr_flag = 0;
			//PORTB |= (1<<DDB0);
			
			// get imu data
			accel_count++;
			if(accel_count >= accel_max_count)
			{
				accel_count = 0;
				
				if(single_tap_ignore)
				{
					single_tap_ignore--;
					PORTB |= (1<<PORTB0);
				}
				else
				{
					PORTB &= ~(1<<PORTB0);
				}
				
				if(double_tap_ignore)
				{
					double_tap_ignore--;
					PORTB |= (1<<PORTB0);
				}
				else
				{
					PORTB &= ~(1<<PORTB0);
				}
				
				err = read_accel(&y, &y_diff, &turnover, &single_tap, &double_tap, turnover);
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
							adjustmode = 0;
							gyro_disable();
							//PORTB &= ~(1<<DDB0);
						
						}
						turnover_old = turnover;
					}

					if(turnover && double_tap && !double_tap_ignore)
					{
						if((mode == ON) || (mode == TWINKLE) || (mode == ACCEL_TWINKLE))
						{
							if(adjustmode)
							{
								adjustmode = 0;
								gyro_disable();
								//PORTB &= ~(1<<DDB0);
							}
							else
							{
								adjustmode = 1;
								gyro_enable();
								//PORTB |= (1<<DDB0);
							}
							
							connected = are_pwm_pins_connected();
							pwm_disconnect_pins();
							for(i=0;i<2;i++)
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
						single_tap_ignore = SINGLE_TAP_IGNORE_DURATION;
						double_tap_ignore = DOUBLE_TAP_IGNORE_DURATION;
					}
					else if(turnover && single_tap && !single_tap_ignore && !adjustmode)
					{
						switch(mode)
						{
							case OFF:
							pwm_connect_pins();
							pwm_start();
							for(i=0;i<4;i++)
							{
								set_duty(i,Map(on_brightness, 0, UINT16_MAX, 0, UINT8_MAX));
							}
							mode = ON;
							break;
							
							case ON:
							pwm_connect_pins();
							pwm_start();
							twinkle_setup(up,top,bottom,step,duty,ramp_step_mean);
							mode = TWINKLE;
							break;
							
							case TWINKLE:
							pwm_connect_pins();
							pwm_start();
							for(i=0;i<4;i++)
							{
								set_duty(i,ACCEL_MIN_BRIGHTNESS);
							}
							mode = ACCEL;
							break;
							
							case ACCEL:
							pwm_connect_pins();
							pwm_start();
							twinkle_setup(up,top,bottom,step,duty,ramp_step_mean);
							mode = ACCEL_TWINKLE;
							break;
							
							case ACCEL_TWINKLE:
							pwm_connect_pins();
							pwm_start();
							for(i=0;i<4;i++)
							{
								set_duty(i,0);
							}
							accel_thresh_count = 0;
							mode = ACCEL_THRESH;
							break;
							
							case ACCEL_THRESH:
							pwm_stop();
							pwm_disconnect_pins();
							set_all_led_pins(0);
							sos_setup(&sos_count, &sos_stage);
							mode = SOS;
							break;
							
							case SOS:
							pwm_stop();
							pwm_disconnect_pins();
							set_all_led_pins(0);
							mode = OFF;
							break;
						}
						
						single_tap_ignore = SINGLE_TAP_IGNORE_DURATION;
						double_tap_ignore = DOUBLE_TAP_IGNORE_DURATION;
					}
				}
			}
			
			if((accel_count == 1) && (adjustmode))
			{
				err = read_gyro(&y_velocity);
				if(err == 0)
				{
					switch(mode)
					{
						case TWINKLE:
						case ACCEL_TWINKLE:
						if(y_velocity>0)
						{
							if((UINT16_MAX-ramp_step_mean_fullscale) >= y_velocity)
							{
								ramp_step_mean_fullscale += y_velocity;
							}
							else
							{
								ramp_step_mean_fullscale = UINT16_MAX;
							}
						}
						else
						{
							if(ramp_step_mean_fullscale >= (-y_velocity))
							{
								ramp_step_mean_fullscale += y_velocity;
							}
							else
							{
								ramp_step_mean_fullscale = 0;
							}
						}
						ramp_step_mean = Map(ramp_step_mean_fullscale, 0, UINT16_MAX, RAMP_STEP_MEAN_MIN, RAMP_STEP_MEAN_MAX);
						
						break;
						
						case ON:
						if(y_velocity>0)
						{
							if((UINT16_MAX-on_brightness) >= y_velocity)
							{
								on_brightness += y_velocity;
							}
							else
							{
								on_brightness = UINT16_MAX;
							}
						}
						else
						{
							if(on_brightness >= (-y_velocity))
							{
								on_brightness += y_velocity;
							}
							else
							{
								on_brightness = 0;
							}
						}
						break;
					}
					
					
				}
			}
			
			
			//display based on mode
			
			switch(mode)
			{
				case OFF:
				
				break;
				
				case ON:
				for(i=0;i<4;i++)
				{
					set_duty(i,Map(on_brightness, 0, UINT16_MAX, 0, UINT8_MAX));
				}
				break;
				
				case TWINKLE:
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
							step[i] = get_rand(ramp_step_mean*(1-RAMP_STEP_SIGMA), ramp_step_mean*(1+RAMP_STEP_SIGMA)+1);
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
							step[i] = get_rand(ramp_step_mean*(1-RAMP_STEP_SIGMA), ramp_step_mean*(1+RAMP_STEP_SIGMA)+1);
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
				
				case ACCEL:
				accel_output = clamp(y_diff, -2*ACCEL_ABS_RANGE, 2*ACCEL_ABS_RANGE);
				accel_output = abs(accel_output);
				accel_output_filt = lowpass(ACCEL_LAMBDA, accel_output, accel_output_filt);
				accel_temp = Map(accel_output_filt, 0, 2*ACCEL_ABS_RANGE, 0, UINT8_MAX);
				accel_temp = max(ACCEL_MIN_BRIGHTNESS, accel_temp);
				
				
				for(i=0; i<4; i++)
				{
					set_duty(i,accel_temp);
				}
				break;
				
				case ACCEL_TWINKLE:
				accel_twinkle_output = clamp(y_diff, -2*ACCEL_TWINKLE_ABS_RANGE, 2*ACCEL_TWINKLE_ABS_RANGE);
				accel_twinkle_output = abs(accel_twinkle_output);
				accel_twinkle_output_filt = lowpass(ACCEL_TWINKLE_LAMBDA, accel_twinkle_output, accel_twinkle_output_filt);
				accel_twinkle_temp = Map(accel_twinkle_output_filt, 0, 2*ACCEL_TWINKLE_ABS_RANGE, 0, UINT8_MAX);
				
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
							step[i] = get_rand(ramp_step_mean*(1-RAMP_STEP_SIGMA), ramp_step_mean*(1+RAMP_STEP_SIGMA)+1);
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
							step[i] = get_rand(ramp_step_mean*(1-RAMP_STEP_SIGMA), ramp_step_mean*(1+RAMP_STEP_SIGMA)+1);
							top[i] = get_rand(MIN_RAMP_TOP, MAX_RAMP_TOP);
						}
						else
						{
							duty[i] -= step[i];
						}
					}
					
					set_duty(i,max(Map(duty[i],0,UINT16_MAX,bottom[i],top[i]),accel_twinkle_temp));
					
				}
				break;
				
				case ACCEL_THRESH:
				//accel_thresh_output = clamp(y_diff, -2*ACCEL_TWINKLE_ABS_RANGE, 2*ACCEL_TWINKLE_ABS_RANGE);
				//accel_thresh_output = abs(accel_thresh_output);
				
				accel_thresh_output = clamp(y_diff, 0, 2*ACCEL_TWINKLE_ABS_RANGE);
							
				if(accel_thresh_output>ACCEL_THRESH_THRESHOLD)
				{
					accel_thresh_count = UINT16_MAX;
				}
				else if(accel_thresh_count >= ACCEL_THRESH_STEP)
				{
					accel_thresh_count -= ACCEL_THRESH_STEP;
				}
				else
				{
					accel_thresh_count = 0;
				}
				
				for(i=0; i<4; i++)
				{
					set_duty(i, Map(accel_thresh_count,0,UINT16_MAX,0,UINT8_MAX));
				}
				
				
				break;
				
				case SOS:
				switch(sos_stage)
				{
					case 0: //first short burst
					sos_temp = sos_count/SOS_DIT_LENGTH;
					if(sos_temp >= 6)
					{
						sos_count = 0;
						sos_stage = 1;
						set_all_led_pins(0);
					}
					else
					{
						if(sos_temp % 2)
						{
							set_all_led_pins(0);
						}
						else
						{
							set_all_led_pins(1);
						}
						sos_count++;
					}
					break;
					
					case 1: //long burst
					sos_temp = sos_count/(SOS_DIT_LENGTH);
					if(sos_temp >= 12)
					{
						sos_count = 0;
						sos_stage = 2;
						set_all_led_pins(0);
					}
					else
					{
						if((sos_temp % 4) == 3)
						{
							set_all_led_pins(0);
						}
						else
						{
							set_all_led_pins(1);
						}
						sos_count++;
					}
					break;
					
					case 2: //second short burst
					sos_temp = sos_count/SOS_DIT_LENGTH;
					if(sos_temp >= 5)
					{
						sos_count = 0;
						sos_stage = 3;
						set_all_led_pins(0);
					}
					else
					{
						if(sos_temp % 2)
						{
							set_all_led_pins(0);
						}
						else
						{
							set_all_led_pins(1);
						}
						sos_count++;
					}
					break;
					
					case 3: //pause
					if(sos_count >= (SOS_DIT_LENGTH*7))
					{
						sos_count = 0;
						sos_stage = 0;
					}
					else
					{
						sos_count++;
					}
					break;
				}
				break;
			}
			//PORTB &= ~(1<<DDB0);
		}
		asm("wdr");
	}
}

