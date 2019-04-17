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

//#define FLICKER_TEST
#define ACCEL_TEST

#define ADXL345_ADDRESS 0x53
/* ADXL345/6 Register Map */
#define DEVID		0x00	/* R   Device ID */
#define THRESH_TAP	0x1D	/* R/W Tap threshold */
#define OFSX		0x1E	/* R/W X-axis offset */
#define OFSY		0x1F	/* R/W Y-axis offset */
#define OFSZ		0x20	/* R/W Z-axis offset */
#define DUR		0x21	/* R/W Tap duration */
#define LATENT		0x22	/* R/W Tap latency */
#define WINDOW		0x23	/* R/W Tap window */
#define THRESH_ACT	0x24	/* R/W Activity threshold */
#define THRESH_INACT	0x25	/* R/W Inactivity threshold */
#define TIME_INACT	0x26	/* R/W Inactivity time */
#define ACT_INACT_CTL	0x27	/* R/W Axis enable control for activity and */
/* inactivity detection */
#define THRESH_FF	0x28	/* R/W Free-fall threshold */
#define TIME_FF		0x29	/* R/W Free-fall time */
#define TAP_AXES	0x2A	/* R/W Axis control for tap/double tap */
#define ACT_TAP_STATUS	0x2B	/* R   Source of tap/double tap */
#define BW_RATE		0x2C	/* R/W Data rate and power mode control */
#define POWER_CTL	0x2D	/* R/W Power saving features control */
#define INT_ENABLE	0x2E	/* R/W Interrupt enable control */
#define INT_MAP		0x2F	/* R/W Interrupt mapping control */
#define INT_SOURCE	0x30	/* R   Source of interrupts */
#define DATA_FORMAT	0x31	/* R/W Data format control */
#define DATAX0		0x32	/* R   X-Axis Data 0 */
#define DATAX1		0x33	/* R   X-Axis Data 1 */
#define DATAY0		0x34	/* R   Y-Axis Data 0 */
#define DATAY1		0x35	/* R   Y-Axis Data 1 */
#define DATAZ0		0x36	/* R   Z-Axis Data 0 */
#define DATAZ1		0x37	/* R   Z-Axis Data 1 */
#define FIFO_CTL	0x38	/* R/W FIFO control */
#define FIFO_STATUS	0x39	/* R   FIFO status */
#define TAP_SIGN	0x3A	/* R   Sign and source for tap/double tap */


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
}

int8_t accel_init(void)
{
	uint8_t send[2];
	twi_report_t* twi_report;
	
	send[0] = BW_RATE;
	send[1] = 0x0A; // normal power, 100hz output rate (bw 50hz)
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	
	send[0] = POWER_CTL;
	send[1] = 0x08; // no auto sleep, measurement mode
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}
	
	send[0] = DATA_FORMAT;
	send[1] = 0x00; // +-2g, right justified, normal resolution (10bit)
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -3;
	}
	
	send[0] = FIFO_CTL;
	send[1] = 0x00; // fifo bypass
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -4;
	}
	
	send[0] = OFSX;
	send[1] = 0x00; // zero x-offset
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -5;
	}
	
	send[0] = OFSY;
	send[1] = 0x00; // zero y-offset
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -6;
	}
	
	send[0] = OFSZ;
	send[1] = 0x00; // zero z-offset
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -7;
	}
	
	send[0] = INT_ENABLE;
	send[1] = 0x00; // zero y-offset
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -8;
	}
	
	return 0;
}

int8_t accel_x(uint16_t* value)
{
	twi_report_t* twi_report;
	uint8_t send[1];
	
	send[0] = DATAX0;
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	
	
	twi_read(ADXL345_ADDRESS, 2, NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}
	
	if(twi_report->length != 2)
	{
		return -3;
	}
	
	*value = ((((uint16_t)twi_report->data[1])<<8) | ((uint16_t)twi_report->data[0])) & 0x03FF;
	
	return 0;
}


#ifdef ACCEL_TEST

int main(void)
{
	uint8_t send[] = {0x01,0x02,0x03};
	twi_report_t* twi_report;
	uint16_t value;

	DDRB |= (1<<DDB0);
	PORTB &= ~(1<<DDB0);
	
	twi_init();
	wait_1ms(1);
	
	accel_init();
	wait_1ms(10);
	

	while(1)
	{
		accel_x(&value);
		PORTB |= (1<<DDB0);
		wait_1ms(value);
		PORTB &= ~(1<<DDB0);
		
		wait_1ms(1000);
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

