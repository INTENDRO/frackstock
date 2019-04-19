#ifndef TWI_H
#define TWI_H

#include <stdint.h>

#ifndef TWI_FREQ
#define TWI_FREQ 100000UL
#endif

#ifndef F_CPU
#define F_CPU 16000000UL
#endif



#ifndef TWI_BUFFER_LENGTH
#define TWI_BUFFER_LENGTH 32
#endif

typedef struct {
	uint8_t error;
	uint8_t address;
	uint8_t* data;
	uint8_t length;
}twi_report_t;

typedef void (*report_cb_t)(twi_report_t* report);

typedef struct {
	uint8_t buffer[TWI_BUFFER_LENGTH];
	uint8_t length;
	uint8_t index;
	report_cb_t callback;
} twi_transmission_t;


void twi_init();
void twi_write(uint8_t address, uint8_t* data, uint8_t length, report_cb_t report_cb);
void twi_read(uint8_t address, uint8_t length, report_cb_t report_cb);
twi_report_t *twi_wait();

#endif
