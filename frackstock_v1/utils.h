/*
 * utils.h
 *
 * Created: 21/04/2019 10:50:35
 *  Author: Mario
 */ 


#ifndef UTILS_H_
#define UTILS_H_



void wait_1ms(uint16_t factor);
void timer0_pwm_init(void);
void timer1_pwm_init(void);
void timer0_pwma_set_duty(uint8_t duty);
void timer0_pwmb_set_duty(uint8_t duty);
void timer1_pwma_set_duty(uint8_t duty);
void timer1_pwmb_set_duty(uint8_t duty);
void timer0_pwm_start(void);
void timer0_pwm_stop(void);
void timer1_pwm_start(void);
void timer1_pwm_stop(void);
void set_duty(uint8_t num, uint8_t duty);
void pwm_init(void);
void pwm_start(void);
void pwm_stop(void);
uint16_t pseudorandom16 (void);
uint16_t get_rand(uint16_t start, uint16_t stop);
int32_t Map(int32_t data, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);


#endif /* UTILS_H_ */