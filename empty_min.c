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
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* Standard Files */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
//#include <ti/sysbios/hal/Swi.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/gates/GateHwi.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

/* Board Header file */
#include "Board.h"

#include "motor/motor.h"
#include "sensor/sensor.h"
#include "UI/UI.h"

/* Other Includes */
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"

#define TASKSTACKSIZE   1024

Task_Struct task1Struct, task2Struct;
Task_Params taskParams;
Char taskUIStack[TASKSTACKSIZE];
Char taskLightStack[TASKSTACKSIZE];

// Handles for Hwis

//Error_Block eb;
//Error_init(&eb);



/*
 *  ======== main ========
 */
int main(void)
{
    FPUEnable();
    FPULazyStackingEnable();
    Task_Params taskParams;
    Task_Params_init(&taskParams);

    // Set system clock
    uint32_t ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
            SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
            SYSCTL_CFG_VCO_480), 120000000);

    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initI2C();

    initUI();
    initMotor();

    /* Construct Task for UI */
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &taskUIStack;
    taskParams.priority = 10;
    taskParams.arg0 = ui32SysClock;
    Task_construct(&task1Struct, (Task_FuncPtr)drawFxn, &taskParams, NULL);

    /* Construct Task for Light Sensor */
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &taskLightStack;
    taskParams.priority = 11;
    Task_construct(&task2Struct, (Task_FuncPtr)initI2C_opt3001, &taskParams, NULL);

    // Enable interrupts
    IntMasterEnable();

    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
