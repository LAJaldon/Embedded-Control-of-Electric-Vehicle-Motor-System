/*
 * motor.h
 *
 *  Created on: 14 May 2021
 *      Author: David
 */

#ifndef MOTOR_H_
#define MOTOR_H_

extern void initMotor(void);
//extern void startMotor(void);
//extern void stopMotor(void);
extern void increaseSpeed();
extern void decreaseSpeed();
extern int getSpeed();
extern int getGoalBaby();
#endif /* MOTOR_H_ */
