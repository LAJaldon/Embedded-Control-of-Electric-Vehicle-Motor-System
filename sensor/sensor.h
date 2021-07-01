/*
 * motor.h
 *
 *  Created on: 29 May 2021
 *      Author: Luigi
 */

#ifndef SENSOR_H_
#define SENSOR_H_


extern void initI2C_opt3001();
bool writeI2C(I2C_Handle i2cHandle, uint8_t ui8Reg, uint16_t *data);
extern int getLux();

#endif /* SENSOR_H_ */
