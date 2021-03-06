/*
 * adxl345.c
 *
 * Created: 21/04/2019 11:15:50
 *  Author: Mario
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stddef.h>
#include "adxl345.h"
#include "twi.h"


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
	
	send[0] = INT_ENABLE;
	send[1] = 0x00;
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -8;
	}
	
	send[0] = POWER_CTL;
	send[1] = 0x08; // no auto sleep, measurement mode
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}
	
	return 0;
}

int8_t get_offset(int16_t* x, int16_t* y, int16_t* z)
{
	twi_report_t* twi_report;
	uint8_t send[1];
	
	send[0] = OFSX;
	twi_write(ADXL345_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	
	twi_read(ADXL345_ADDRESS, 3, NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}
	
	if(twi_report->length != 3)
	{
		return -3;
	}
	
	*x = (int16_t)twi_report->data[0];
	*y = (int16_t)twi_report->data[1];
	*z = (int16_t)twi_report->data[2];
	
	return 0;
	
}

int8_t accel_x(int16_t* value)
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
	
	*value = (int16_t)((((uint16_t)(twi_report->data[1]))<<8) | ((uint16_t)(twi_report->data[0])));
	
	return 0;
}

int8_t accel_y(int16_t* value)
{
	twi_report_t* twi_report;
	uint8_t send[1];
	
	send[0] = DATAY0;
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
	
	*value = (int16_t)((((uint16_t)(twi_report->data[1]))<<8) | ((uint16_t)(twi_report->data[0])));
	
	return 0;
}

int8_t accel_z(int16_t* value)
{
	twi_report_t* twi_report;
	uint8_t send[1];
	
	send[0] = DATAZ0;
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
	
	*value = (int16_t)((((uint16_t)(twi_report->data[1]))<<8) | ((uint16_t)(twi_report->data[0])));
	
	return 0;
}

int8_t accel_xyz(int16_t* x, int16_t* y, int16_t* z)
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
	
	twi_read(ADXL345_ADDRESS, 6, NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}
	
	if(twi_report->length != 6)
	{
		return -3;
	}
	
	*x = (int16_t)((((uint16_t)(twi_report->data[1]))<<8) | ((uint16_t)(twi_report->data[0])));
	*y = (int16_t)((((uint16_t)(twi_report->data[3]))<<8) | ((uint16_t)(twi_report->data[2])));
	*z = (int16_t)((((uint16_t)(twi_report->data[5]))<<8) | ((uint16_t)(twi_report->data[4])));
	
	return 0;
}

