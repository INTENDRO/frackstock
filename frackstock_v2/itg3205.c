/*
 * itg3205.c
 *
 * Created: 24/04/2019 22:46:45
 *  Author: Mario
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stddef.h>
#include "itg3205.h"
#include "twi.h"

int8_t gyro_init(void)
{
	uint8_t send[2];
	twi_report_t* twi_report;

	send[0] = SMPLRT_DIV;
	send[1] = 19; //divide 1khz to 50hz
	twi_write(ITG3205_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}

	send[0] = DLPF_FS;
	send[1] = 0b000111000; //20hz bandwidth
	twi_write(ITG3205_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -2;
	}

	send[0] = PWR_MGM;
	send[1] = 0x78; //sleep mode & all axes in standby
	twi_write(ITG3205_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -3;
	}
	return 0;
}

int8_t gyro_enable(void)
{
	uint8_t send[2];
	twi_report_t* twi_report;
	
	send[0] = PWR_MGM;
	send[1] = 0x00;
	twi_write(ITG3205_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	return 0;
}

int8_t gyro_disable(void)
{
	uint8_t send[2];
	twi_report_t* twi_report;
	
	send[0] = PWR_MGM;
	send[1] = 0x78; //sleep mode & all axes in standby
	twi_write(ITG3205_ADDRESS, send, sizeof(send), NULL);
	twi_report = twi_wait();
	if(twi_report->error != 0)
	{
		return -1;
	}
	return 0;
}
