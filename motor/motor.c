/*
 *  ======== motor.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* Standard Files */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/gates/GateHwi.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* Board Header file */
#include "Board.h"

#include "drivers/motorlib.h"
#include "driverlib/fpu.h"

#define MAXRPM 10000.00

int pwm_period = 24; //Header files recommend 24
bool emergency_brake = false;
int current_speed = 0;
int goal_speed = 0;

int Hall_A_Start, Hall_B_Start, Hall_C_Start;
float oldRevs, newRevs, newTicks;
float oldTicks = 0;
float currentTicks;
volatile int revCounter = 0;
volatile int oldRevCount = 0;
volatile int newRevCount = 0;
#define tickValue 8.191875 //in ms
float rps;
int rpm;
float revDiff = 0;
float tickDiff = 0;
double goal_speedpreround;
int current_Hall_A, current_Hall_B, current_Hall_C;
//int safetyCounter = 0;

GateHwi_Handle gateHwi;
GateHwi_Params gHwirpm;

int getSpeed(){
    UInt key;
    key = GateHwi_enter(gateHwi);

    // Calculate the number of revs
    oldRevCount = newRevCount;
    newRevCount = revCounter;
    //float revDiff = newRevs - oldRevs;
    //revDiff = newRevCount - oldRevCount;
    tickDiff = newTicks - oldTicks;

    //float tickDiff = oldTicks - newTicks;
    //time of tickdiff in seconds

    if (revDiff > 0){
        float time = (tickDiff * tickValue)/1000.00;
        //convert time to seconds
        rps =  (revCounter/time);
        oldTicks = Clock_getTicks();
    }
    GateHwi_leave(gateHwi,key);
    revCounter = 0;
    //return rps;
    rpm = (int)round((current_speed/100.00) * MAXRPM);
    return rpm;
}

int getGoalBaby(){
    UInt key;
    key = GateHwi_enter(gateHwi);


    int goal_rpm = (goal_speed/100.00) * MAXRPM;
    GateHwi_leave(gateHwi,key);

    return goal_rpm;

}

void setSpeed(float new_speed){
    UInt key;
    key = GateHwi_enter(gateHwi);
    //Set the new speed as the goal
    goal_speedpreround = ((new_speed/MAXRPM) * 100.00);
    goal_speed = round(goal_speedpreround);
    //Toggle LEDs depending if motor will accelerate or decelerate
    if(goal_speed > current_speed){
        GPIO_write(Board_LED0, Board_LED_ON);
        GPIO_write(Board_LED1, Board_LED_OFF);
        //GPIO_write(Board_LED3, Board_LED_OFF);
    }else if(goal_speed < current_speed){
        //GPIO_write(Board_LED3, Board_LED_ON);
        GPIO_write(Board_LED1, Board_LED_ON);
        GPIO_write(Board_LED0, Board_LED_OFF);
    }
    GateHwi_leave(gateHwi, key);
}

void increaseSpeed(){
    int new_speed = ((goal_speed/100.00) * MAXRPM) + MAXRPM/10;
    setSpeed(new_speed);
}

void decreaseSpeed(){
    int new_speed = ((goal_speed/100.00) * MAXRPM) - MAXRPM/10;
    setSpeed(new_speed);
}

// This function sets the current hall readings and saves it
void getHallPos(){
    Hall_A_Start = GPIO_read(HALL_A);
    Hall_B_Start = GPIO_read(HALL_B);
    Hall_C_Start = GPIO_read(HALL_C);
}

//If current hall sensors == start readings then increment
void updateRevCounter(){
//    if(GPIO_read(HALL_A) == Hall_A_Start && GPIO_read(HALL_B) == Hall_B_Start && GPIO_read(HALL_C) == Hall_C_Start){
//        revCounter++;
//    }
}

//Callback function for the GPIO interrupt for Hall Effect Sensor A.
//Will accelerate, decelerate, and stop the motor
void gpioHallAFxn(){
//    updateRevCounter();
    revCounter++;
    currentTicks = Clock_getTicks(); //Get old systicks
//    oldRevs = revCounter;
    int tickDiff = currentTicks - oldTicks;
    getSpeed();

    //If emergency brake has occurred
    //Then slow down at a rate of 1000rpm
    //Else accelerate or decelerate
//    System_printf("This is your tickdiff %d \n", tickDiff );
//    System_flush();
//    System_printf("This is your currentspeed %d \n", current_speed );
//    System_flush();
    if(emergency_brake == true){
        if((current_speed-10) > goal_speed && tickDiff >= 125){
            current_speed -= 1;
            setDuty(current_speed);
            oldTicks = currentTicks;
//            System_printf("printing");
//            System_printf("%d\n", current_speed);
            GPIO_write(Board_LED0, Board_LED_ON);
            GPIO_write(Board_LED1, Board_LED_ON);
        }else if((current_speed-10) <= goal_speed){
            current_speed = 0;
            disableMotor();
            setDuty(current_speed);
            GPIO_write(Board_LED0, Board_LED_OFF);
            GPIO_write(Board_LED1, Board_LED_OFF);
            puts("Emergency Stop!\n");
            System_flush();
        }
    }else{
        //puts("Ebrake off");
        if((current_speed+1) < goal_speed && tickDiff >= 250){
            //Motor is accelerating
            //puts("We speeding");

            current_speed += 1;
            setDuty(current_speed);
            oldTicks = currentTicks;
            //safetyCounter = 0;
        }else if((current_speed-1) > goal_speed && tickDiff >= 250){
            //Motor is decelerating
            //puts("slowing");
            current_speed -= 1;
            setDuty(current_speed);
            oldTicks = currentTicks;
            GPIO_write(Board_LED1, Board_LED_ON);
            //safetyCounter = 0;
        }else{
            //Goal speed has been reached
            //If goal speed was 0, motor will stop running
            //Else motor will run at constant speed
            if(goal_speed == 0 && (current_speed-1) == goal_speed){
                //Motor Stopping
                //puts("thisstop");
                current_speed = 0;
                disableMotor();
                setDuty(current_speed);
                GPIO_write(Board_LED0, Board_LED_OFF);
                GPIO_write(Board_LED1, Board_LED_OFF);
                puts("Motor Stopped\n");
                System_flush();
            }else if((current_speed+1) == goal_speed){
                //Motor is now running at constant speed
                //puts("THis constant");
                current_speed = goal_speed;
                setDuty(current_speed);
                GPIO_write(Board_LED0, Board_LED_ON);
                GPIO_write(Board_LED1, Board_LED_ON);
                //safetyCounter = 0;
            }
        }
    }
    updateMotor(GPIO_read(HALL_A),GPIO_read(HALL_B),GPIO_read(HALL_C));

    //rpm = (int)round((current_speed/100.00) * 10000);

    //oldTicks = Clock_getTicks();
    updateRevCounter();
    //safetyCounter++;
}

//Callback function for the GPIO interrupt for Hall Effect Sensor B and C.
//Just runs the motor
void gpioHallBCFxn(){
    updateMotor(GPIO_read(HALL_A),GPIO_read(HALL_B),GPIO_read(HALL_C));
}

//Start the motor
void startMotor(){
    enableMotor();
    //read initial hall pos
    getHallPos();
    GPIO_write(Board_LED0, Board_LED_ON);
    current_speed = 5;
    if(goal_speed == 0){
        setSpeed(5000);
    }
    setDuty(current_speed);
    updateMotor(GPIO_read(HALL_A),GPIO_read(HALL_B),GPIO_read(HALL_C));
    puts("Motor Started\n");
    System_flush();
}

//Stop the motor
void stoppingMotor(){
    goal_speed = 0;
    //puts("Motor Stopping\n");
    //System_flush();
    GPIO_write(Board_LED0, Board_LED_OFF);
}

//Callback function for the GPIO interrupt for SW1.
//Start and stop the motor
void gpioSW1Fxn(){
    if(GPIO_read(Board_LED0)==0){
        startMotor();
        emergency_brake = false;
    }else{
        stoppingMotor();
    }
}

//Callback function for the GPIO interrupt for SW2.
//Emergency brake
void gpioSW2Fxn(){
    emergency_brake = true;
    setSpeed(0);
    GPIO_write(Board_LED0, Board_LED_OFF);
//    puts("Emergency Stop!\n");
//    System_flush();
}

//Initialising the Hall Effect Sensors and their interrupts.
void initHallABC(){
    //setup GPIO pins for Hall effect lines
    //can use TI-RTOS GPIO driver implementation
    //recommended to use same hwi callback function (aka ISR)
    //for all halleffect sensors
    //used example: gpiointerrupt_EK_TM4C1294XL_TI

    //Install hall effect sensors callbacks
    GPIO_setCallback(HALL_A, gpioHallAFxn);
    GPIO_setCallback(HALL_B, gpioHallBCFxn);
    GPIO_setCallback(HALL_C, gpioHallBCFxn);

    //Enable interrupts
    GPIO_enableInt(HALL_A);
    GPIO_enableInt(HALL_B);
    GPIO_enableInt(HALL_C);

    puts("Hall Effect Sensors Initialised\n");
    System_flush();
}

//Initialising the buttons and their interrupts.
void initButtons(){
    //Install buttons callbacks
    GPIO_setCallback(EK_TM4C1294XL_USR_SW1, gpioSW1Fxn);
    GPIO_setCallback(EK_TM4C1294XL_USR_SW2, gpioSW2Fxn);

    //Enable interrupts
    GPIO_enableInt(EK_TM4C1294XL_USR_SW1);
    GPIO_enableInt(EK_TM4C1294XL_USR_SW2);

    puts("Buttons Initialised\n");
    System_flush();
}

//Initialising the interrupts and motor.
void initMotor(){
    //Initialise Hall Effect sensors
    initHallABC();

    //Initialise the buttons
    initButtons();

    //Initialise MotorLib
    Error_Block error_block;
    Error_init(&error_block);
    initMotorLib(pwm_period, &error_block);

    //Disable motor and set duty
    disableMotor();
    setDuty(pwm_period);
    puts("Motor Ready\n");
    System_flush();
    puts("Press S1 to Start\n");
    System_flush();
}
