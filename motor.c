/*
 * motor.c
 *
 *  Created on: 14 May 2021
 *      Author: Gavin
 */

#include 'motorlib.h'

volatile uint8_t g_phase_cnt = 0;
volatile uint8_t g_phase = 0;
uint32_t dutyVal = 0;

int pwm_period = 24; //Header files recommend 24
bool motoron = false;

void motorFxn(){
    unsigned int events;

    initHallABC();
    success = initMotorLib(pwn_period, error_block);

    while(1){

        events = Event_pend(motor_evHandle, Event_Id_None, STOP_MOTOR | START_MOTOR | SPEED_UP | SPEED_DOWN, BIOS_WAIT_FOREVER);

        switch(events){

            case START_MOTOR:
                motoron = true;
                setDuty(??); //This needs to be decided based on PI Controller

                //kickstart motor
                //read the state of hall abc
                readHallABC();

                //force one step of motor using update function
                updateMotor(A,B,??);

                break;

            case STOP_MOTOR:
                motoron = false;
                stopMotor(false);

            default:
            break;

        }
    }
}

void initHallABC(){
    //setup GPIO pins for Hall effect lines
    //can use TI-RTOS GPIO driver implementation
    //recommended to use same hwi callback function (aka ISR)
    //for all halleffect sensors

    GPIO_enableInt(HALL_A);
    GPIO_enableInt(HALL_B);
    GPIO_enableInt(HALL_C);
}

void hwiHallInt(){

    //read state of Hall effect GPIO lines

    //update motor here
    //may need to check if motor is running
    updateMotor(??,??,??);

    //recommend to update LEDs to indicate error motor is getting updated
    setHallLeds(Hall_a << 2 | Hall_b << 1 | Hall_c <<1);
}

//This is leds that light up depending on the phase. Phase 3 continues but is unseen. We can just make our own versions anyway
void setHallLeds(uint8_t phase){

    switch(phase){
        case PHASE_1:
        GPIO_write(Board_LED0, Board_LED_ON);
        GPIO_write(Board_LED1, Board_LED_OFF);
        GPIO_write(Board_LED2, Board_LED_OFF);

        break;

        case PHASE_2:
        GPIO_write(Board_LED0, Board_LED_ON);
        GPIO_write(Board_LED1, Board_LED_ON);
        GPIO_write(Board_LED2, Board_LED_OFF);

        break;

        case PHASE_3:


    }
}




