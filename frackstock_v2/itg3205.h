/*
 * itg3205.h
 *
 * Created: 24/04/2019 22:47:45
 *  Author: Mario
 */ 


#ifndef ITG3205_H_
#define ITG3205_H_


#define ITG3205_ADDRESS 0x68
/* ITG3205 Register Map */
#define PWR_MGM		0x3E

int8_t gyro_init(void);
int8_t gyro_enable(void);
int8_t gyro_disable(void);


#endif /* ITG3205_H_ */