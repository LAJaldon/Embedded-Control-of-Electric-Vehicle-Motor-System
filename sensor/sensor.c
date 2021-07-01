/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== empty_min.c ========
 *
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/gates/GateHwi.h>
//#include <ti/sysbios/BIOS.h>
//#include <ti/drivers/UART.h>

/* TI-RTOS Header files */
// #include <ti/drivers/EMAC.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART.h>
#include "driverlib/gpio.h"

/* Board Header file */
#include "Board.h"
//#include "drivers/i2cOptDriver.h"

/* Slave address */
#define OPT3001_I2C_ADDRESS             0x47

/* Register addresses */
#define REG_RESULT                      0x00
#define REG_CONFIGURATION               0x01
#define REG_LOW_LIMIT                   0x02
#define REG_HIGH_LIMIT                  0x03
#define CONFIG_LOW_LIMIT                0xFF0F  /// 45
#define CONFIG_HIGH_LIMIT               0x9A78 // 2818.56
#define CONFIG_ENABLE                   0x10C4 //0x10C4 // 0xC410   - 100 ms, continuous
#define CONFIG_DISABLE                  0x10C0 // 0xC010   - 100 ms, shutdown

int convertedLuxInt;

bool readI2C(I2C_Handle i2cHandle, uint8_t ui8Reg, uint16_t *data){

    I2C_Transaction i2cTransaction;
    uint8_t txBuffer[1];
    uint8_t rxBuffer[2];
    bool transferOK;

    txBuffer[0] = ui8Reg;

    i2cTransaction.slaveAddress = OPT3001_I2C_ADDRESS;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 2;

    transferOK = I2C_transfer(i2cHandle, &i2cTransaction);

    if (!transferOK) {
        System_abort("Bad I2C Read transfer!");
        return false;
    }
    else{
        data[0] = rxBuffer[0];
        data[1] = rxBuffer[1];
        return true;
    }
}

bool writeI2C(I2C_Handle i2cHandle, uint8_t ui8Reg){
    I2C_Transaction i2cTransaction;
    uint8_t txBuffer[3];
    bool transferOK;

    txBuffer[0] = REG_CONFIGURATION;
    txBuffer[1] = CONFIG_ENABLE;
    txBuffer[2] = CONFIG_DISABLE;

    i2cTransaction.slaveAddress = OPT3001_I2C_ADDRESS;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    transferOK = I2C_transfer(i2cHandle, &i2cTransaction);

    if (!transferOK) {
        System_abort("Bad I2C transfer!");
        return false;
    }
    else{
        return true;
    }
}



bool sensorOpt3001Read(I2C_Handle i2cHandle, uint16_t *rawData)
{
    bool success;
    uint16_t val;

    success = readI2C(i2cHandle, REG_CONFIGURATION, &val);

    if (success)
    {
        success = readI2C(i2cHandle, REG_RESULT, &val);
    }

    if (success)
    {
        // Swap bytes
        *rawData = (val << 8) | (val>>8 &0xFF);
    }

    return (success);
}

void sensorOpt3001Convert(uint16_t rawData, float *convertedLux)
{
    uint16_t e, m;

    m = rawData & 0x0FFF;
    e = (rawData & 0xF000) >> 12;

    *convertedLux = m * (0.01 * exp2(e));
}

void initI2C_opt3001(){
    I2C_Handle      i2cHandle;
    I2C_Params      i2cParams;
    uint16_t rawData;
    float convertedLuxFloat;
    bool success;

    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2cHandle = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2cHandle == NULL) {
        System_abort("Error Initializing Opt3001 I2C\n");
    } else {
        System_printf("I2C Initialized!\n");
        System_flush();
    }

    writeI2C(i2cHandle, REG_CONFIGURATION);

    System_flush();

    while (1) {
        success = sensorOpt3001Read(i2cHandle, &rawData);
        if (success) {
            sensorOpt3001Convert(rawData, &convertedLuxFloat);

            convertedLuxInt = (int)convertedLuxFloat;
            if (convertedLuxInt < 5){
                GPIO_write(Board_LED2, Board_LED_ON);
//                System_printf("low lux: %d\n", convertedLuxInt);
            }
            else {
                GPIO_write(Board_LED2, Board_LED_OFF);
//                System_printf("lux: %d\n", convertedLuxInt);
            }
        }
        System_flush();
    }
}

int getLux(){
    return convertedLuxInt;
}
