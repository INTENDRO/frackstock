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
#define WHO_AM_I	0x00
#define SMPLRT_DIV	0x15
#define DLPF_FS		0x16
#define INT_CFG		0x17
#define INT_STATUS	0x1A
#define TEMP_OUT_H	0x1B
#define TEMP_OUT_L	0x1C
#define GYRO_XOUT_H	0x1D
#define GYRO_XOUT_L	0x1E
#define GYRO_YOUT_H	0x1F
#define GYRO_YOUT_L	0x20
#define GYRO_ZOUT_H	0x21
#define GYRO_ZOUT_L	0x22
#define PWR_MGM		0x3E

int8_t gyro_init(void);
int8_t gyro_enable(void);
int8_t gyro_disable(void);
int8_t gyro_x(int16_t* value);
int8_t gyro_y(int16_t* value);
int8_t gyro_z(int16_t* value);
int8_t gyro_xyz(int16_t* x, int16_t* y, int16_t* z);


#endif /* ITG3205_H_ */