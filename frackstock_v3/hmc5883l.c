/*
 * hmc5883l.c
 *
 * Created: 30/04/2019 21:44:28
 *  Author: Mario
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stddef.h>
#include "hmc5883l.h"
#include "twi.h"


int8_t mag_init(void)
{
	uint8_t send[2];
	twi_report_t* twi_report;

	send[0] = CONFIG_A;
	send[1] = 0b00010000; // 8 samples averaged, 15Hz output rate, normal measurement flow
	twi_write(HMC5883L_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	
	send[0] = CONFIG_B;
	send[1] = 0b00000000; // +-0.8 Ga range
	twi_write(HMC5883L_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}
	
	send[0] = MAG_MODE;
	send[1] = 0b00000011; // idle mode	
	twi_write(HMC5883L_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -3;
	}
}

int8_t mag_enable(void)
{
	uint8_t send[2];
	twi_report_t* twi_report;
	
	send[0] = MAG_MODE;
	send[1] = 0b00000000; //continuous mode
	twi_write(HMC5883L_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	return 0;
}

int8_t mag_disable(void)
{
	uint8_t send[2];
	twi_report_t* twi_report;
	
	send[0] = MAG_MODE;
	send[1] = 0b00000011; //idle mode
	twi_write(HMC5883L_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	return 0;
}

int8_t mag_xyz(int16_t* x, int16_t* y, int16_t* z)
{
	twi_report_t* twi_report;
	uint8_t send[1];
	
	send[0] = MAG_XOUT_H;
	twi_write(HMC5883L_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	
	twi_read(HMC5883L_ADDRESS, 6, NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}
	
	if(twi_report->length != 6)
	{
		return -3;
	}
	
	*x = (int16_t)((((uint16_t)(twi_report->data[0]))<<8) | ((uint16_t)(twi_report->data[1])));
	*z = (int16_t)((((uint16_t)(twi_report->data[2]))<<8) | ((uint16_t)(twi_report->data[3])));
	*y = (int16_t)((((uint16_t)(twi_report->data[4]))<<8) | ((uint16_t)(twi_report->data[5])));
	
	return 0;
}