/*
 * hmc5883l.h
 *
 * Created: 30/04/2019 21:44:44
 *  Author: Mario
 */ 


#ifndef HMC5883L_H_
#define HMC5883L_H_


#define HMC5883L_ADDRESS 0x1E
/* ITG3205 Register Map */
#define CONFIG_A	0x00
#define CONFIG_B	0x01
#define MAG_MODE	0x02
#define MAG_XOUT_H	0x03
#define MAG_XOUT_L	0x04
#define MAG_ZOUT_H	0x05
#define MAG_ZOUT_L	0x06
#define MAG_YOUT_H	0x07
#define MAG_YOUT_L	0x08
#define MAG_STATUS	0x09
#define MAG_ID_A	0x0A
#define MAG_ID_B	0x0B
#define MAG_ID_C	0x0C


int8_t mag_init(void);
int8_t mag_enable(void);
int8_t mag_disable(void);
int8_t mag_xyz(int16_t* x, int16_t* y, int16_t* z);


#endif /* HMC5883L_H_ */