/*
 *  ======== UI.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <driverlib/gpio.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/swi.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <driverlib/sysctl.h>
#include <driverlib/pin_map.h>

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "grlib/grlib.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119_spi.h"
#include "drivers/pinout.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "grlib/slider.h"

#include "drivers/touch.h"
#include <driverlib/sysctl.h>
#include "utils/ustdlib.h"
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include <driverlib/gpio.h>
#include <ti/sysbios/hal/Seconds.h>
#include <time.h>
#include <ti/drivers/I2C.h>

#include "motor/motor.h"
#include "sensor/sensor.h"

// Graphics
tContext sContext;
int state = 1;
bool refresh = false;

#define SLIDER_TEXT_VAL_INDEX   0
#define SLIDER_LOCKED_INDEX     2
#define SLIDER_CANVAS_VAL_INDEX 4

#define NUM_SLIDERS (sizeof(g_psSliders) / sizeof(g_psSliders[0]))

/* Board Header file */
#include "Board.h"

//#define TASKSTACKSIZE   1024
//Task_Struct task0Struct;
//Char task0Stack[TASKSTACKSIZE];

tCanvasWidget     g_sBackground;
tPushButtonWidget g_sChangeStateBttn;
tPushButtonWidget g_sPlusBttn;
tPushButtonWidget g_sMinusBttn;
extern tCanvasWidget g_psPanels[];

//_----------------GRLIB----------------------

void OnSliderChange(tWidget *psWidget, int32_t i32Value);

Canvas(g_sSliderValueCanvas, g_psPanels + 2, 0, 0,
       &g_sKentec320x240x16_SSD2119, 210, 30, 60, 40,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm24, "50%",
       0, 0);

tSliderWidget g_psSliders[] =
{
    SliderStruct(g_psPanels + 2, g_psSliders + 1, 0,
                     &g_sKentec320x240x16_SSD2119, 160, 60, 150, 15, 0, 7500, 1000,
                     (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                      SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                      ClrWhite, ClrBlack, ClrSilver,ClrBlack , ClrWhite,
                     &g_sFontCm14, "1000rpm", 0, 0, OnSliderChange),
};

void
OnSliderChange(tWidget *psWidget, int32_t i32Value)
{
    static char pcCanvasText[5];
    static char pcSliderText[5];

    //
    // Is this the widget whose value we mirror in the canvas widget and the
    // locked slider?
    //
    if(psWidget == (tWidget *)&g_psSliders[SLIDER_CANVAS_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcCanvasText, "%3d%%", i32Value);
        CanvasTextSet(&g_sSliderValueCanvas, pcCanvasText);
        WidgetPaint((tWidget *)&g_sSliderValueCanvas);

        //
        // Also update the value of the locked slider to reflect this one.
        //
        SliderValueSet(&g_psSliders[SLIDER_LOCKED_INDEX], i32Value);
        WidgetPaint((tWidget *)&g_psSliders[SLIDER_LOCKED_INDEX]);
    }

    if(psWidget == (tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcSliderText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[SLIDER_TEXT_VAL_INDEX], pcSliderText);
        WidgetPaint((tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX]);
    }
}

//--------------------END GRLIB--------------
void changeState(tWidget *psWidget);
void increaseBttnPress(tWidget *psWidget);
void decreaseBttnPress(tWidget *psWidget);

// The canvas widget acting as the background to the display.
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sChangeStateBttn,
       &g_sKentec320x240x16_SSD2119, 10, 25, 300, (240 - 25 -10),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

RectangularButton(g_sChangeStateBttn, &g_sBackground, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 50, 200, 100, 25,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                   g_psFontCmss16b, "RPM", 0, 0, 0, 0, changeState);

RectangularButton(g_sPlusBttn, &g_sBackground, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 250, 70, 40, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                   g_psFontCmss16b, "+", 0, 0, 0, 0, increaseBttnPress);

RectangularButton(g_sMinusBttn, &g_sBackground, 0, 0,
                  &g_sKentec320x240x16_SSD2119, 250, 150, 40, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                   g_psFontCmss16b, "-", 0, 0, 0, 0, decreaseBttnPress);

void changeState(tWidget *psWidget)
{
    if(state == 1)
    {
        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sChangeStateBttn, "Lux");
        PushButtonTextSet(&g_sPlusBttn, "+");
        PushButtonTextSet(&g_sMinusBttn, "-");

        //
        // Repaint the pushbutton and all widgets beneath it (in this case,
        // the welcome message).
        //
        WidgetPaint((tWidget *)&g_sChangeStateBttn);
        WidgetPaint((tWidget *)&g_sPlusBttn);
        WidgetPaint((tWidget *)&g_sMinusBttn);

        state = 2;
        refresh = true;
    }
    else if(state == 2)
    {
        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sChangeStateBttn, "Info");
        //
        // Repaint the pushbutton and all widgets beneath it (in this case,
        // the welcome message).
        //
        WidgetPaint((tWidget *)&g_sChangeStateBttn);

        state = 3;
        refresh = true;

    }else if(state == 3){
        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sChangeStateBttn, "RPM");

        //
        // Repaint the pushbutton and all widgets beneath it (in this case,
        // the welcome message).
        //
        WidgetPaint((tWidget *)&g_sChangeStateBttn);

        state = 1;
        refresh = true;

    }
}

void increaseBttnPress(tWidget *psWidget)
{
    increaseSpeed();
}

void decreaseBttnPress(tWidget *psWidget)
{
    decreaseSpeed();
}



//GRLIB SLIDER HERE:


void refreshScreen(){
    tRectangle sRect;
    GrContextForegroundSet(&sContext, ClrBlack);
    sRect.i16XMin = 30;
    sRect.i16YMin = 40;
    sRect.i16XMax = 230;
    sRect.i16YMax = 180;
    GrRectFill(&sContext, &sRect);
    GrContextForegroundSet(&sContext, ClrWhite);
}


/*
 *  ======== drawFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the draw Task instance.
 */
void drawFxn(UArg arg0, UArg arg1)
{
    // Add the compile-time defined widgets to the widget tree.
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    // Paint the widget tree to make sure they all appear on the display.
    WidgetPaint(WIDGET_ROOT);

    while (1) {
        SysCtlDelay(100);
        WidgetMessageQueueProcess();

        if(state == 1){
            if(refresh){
                refreshScreen();
                refresh = false;
            }
            GrContextForegroundSet(&sContext, ClrWhite);
            GrStringDraw(&sContext, "Students:", 20, 50, 70, true);
            GrStringDraw(&sContext, "David Thai - n9994653", 30, 50, 90, true);
            GrStringDraw(&sContext, "Gavin Smith - n10138196", 30, 50, 110, true);
            GrStringDraw(&sContext, "Wesley Tee - n9506527", 30, 50, 130, true);
            GrStringDraw(&sContext, "Luigi Jaldon - n10000381", 30, 50, 150, true);
        }

        if(state == 2){
            if(refresh){
                refreshScreen();
                refresh = false;
            }
            char str1[25];
            sprintf(str1,"Current rpm : %04d",getSpeed());
            GrStringDraw(&sContext, str1, 25,50,70,true);

            char str2[20];
            sprintf(str2, "Goal rpm : %04d", getGoalBaby());
            GrStringDraw(&sContext, str2, 20,50,90,true);

        }

        if(state == 3){
            if(refresh){
                refreshScreen();
                refresh = false;
            }
            char str3[20];
            sprintf(str3, "Lux : %04d", getLux());
            GrStringDraw(&sContext, str3, 20,50,70,true);
            if(getLux() < 5){
                GrStringDraw(&sContext, "Night Time", 20,50,90,true);
            }else{
                GrStringDraw(&sContext, "Day  Time", 20,50,90,true);
            }

        }

//        TouchScreenIntHandler
    }
}

void initUI(){
//    Task_Params taskParams;
//    PinoutSet(false, false);

    // Set system clock
    uint32_t ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
            SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
            SYSCTL_CFG_VCO_480), 120000000);

//    /* Construct heartBeat Task  thread */
//    Task_Params_init(&taskParams);
//    taskParams.arg0 = ui32SysClock;
//    taskParams.priority = 10;
//    taskParams.stackSize = TASKSTACKSIZE;
//    taskParams.stack = &task0Stack;
//    Task_construct(&task0Struct, (Task_FuncPtr)drawFxn, &taskParams, NULL);

    //tContext sContext;
    Kentec320x240x16_SSD2119Init(ui32SysClock);
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);
    TouchScreenInit(ui32SysClock);
    TouchScreenCallbackSet(WidgetPointerMessage);

    //------------GRLIB--------------
    //
    // Add the title block and the previous and next buttons to the widget
    // tree.
    //


    //
    // Add the first panel to the widget tree.
    //


    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);
    //---------------END GRLIB--------------

    FrameDraw(&sContext, "Group 14 Demo");

    System_printf("UI Initialised.\n");
    System_flush();
}
