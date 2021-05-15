/*
 * sensorAcceleration.h
 *
 *  Created on: 14 May 2021
 *      Author: Gavin
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/i2c.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

//Accelerometer Driver
#include "drivers/bmi160.h"

//Slave Address
#define BMI160_I2C_ADDRESSES            0x69   // BMI160_I2C_ADDR UINT8_C(0x68)? - 70 on board.
