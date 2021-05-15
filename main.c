/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Types.h>

// System libraries
#include <stdint.h>
#include <stdbool.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/gates/GateHwi.h>
#include <ti/drivers/UART.h>
#include <stdio.h>
#include <string.h>

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"

/* Example/Board Header files */
#include "Board.h"

#define TASKSTACKSIZE 512
#define MOVEINC (10)
#define MAXNODES 20

typedef struct MsgObj {
    Queue_Elem elem; // First field for message
    int id; // Custom writer task id
    Char data; // Message data
} MsgObj;

//This file is not complete and requires modification to get Task C to work
//This file will not compile in its current form
//You also need to have some correct modules included in the TI-RTOS .cfg file

Queue_Handle queue;
MsgObj msg_mem[1];

Task_Struct task1Struct;
Task_Struct task2Struct;
Char task1Stack[TASKSTACKSIZE];
Char task2Stack[TASKSTACKSIZE];

// Setup global hw gate and semaphore handle and supporting data structures
GateHwi_Handle gateHwi;
GateHwi_Params gHwiprms;

Semaphore_Struct semStruct;
Semaphore_Handle semHandle;

Swi_Handle SwiHandle;
Hwi_Handle UartHwiHandle;

Event_Struct evtStruct;
Event_Handle evtHandle;

uint32_t g_ui32SysClock;

int xpos = 50;
int ypos = 50;
int xvol = 0;
int yvol = 0;

int lineNodeCount = 0;
int lineX[MAXNODES] = {};
int lineY[MAXNODES] = {};

int16_t screenWidth = 0;
int16_t screenHeight = 0;

tContext sContext;
tRectangle sRect;
tRectangle sCursor;

void redrawScreen(bool redrawRequired);

/*
///Implement Swi Function to flag to task that a space character has been found
void SwiFxn(){
    UInt gateKey;

    //use this to access shared buffers
    // The design this exercises expouses is actually basically single threaded...
    // All the overheads of multithreading but with none of the gains...
    // It's bizzare.
    gateKey = GateHwi_enter(gateHwi);

    //it is now safe to read from buffers
    int i;
    // We're supposed to debounce or something
    // Stuff that
    //for (i = 0; i < 5; i++) {
    for (i = 0; i < 1; i++) {
        char input = inputs[i];
        redrawRequired = true;
        // Left
        if (input == 'a' && xpos - MOVEINC >= 0) {
            xpos -= MOVEINC;
        // Up
        } else if (input == 'w' && ypos - MOVEINC >= 0) {
            ypos -= MOVEINC;
        // Down
        } else if (input == 's' && ypos + MOVEINC <= screenHeight - 1 - 3) {
            ypos += MOVEINC;
        // Right
        } else if (input == 'd' && xpos + MOVEINC <= screenWidth - 1 - 3) {
            xpos += MOVEINC;
        } else if (input == '\r') {
            System_printf("Enter detected\n");
            System_flush();

            if (lineNodeCount + 1 <= MAXNODES) {
                lineX[lineNodeCount] = xpos;
                lineY[lineNodeCount] = ypos;
                lineNodeCount++;
            } else {
                System_printf("Max line count reached!\n");
                System_flush();
            }
        } else if (input == '\x7f') {
            System_printf("Erasing\n");
            System_flush();

            lineNodeCount = 0;
        } else {
            redrawRequired = false;
        }
    }

    //Use gatekey only if access shared resources is needed
    GateHwi_leave(gateHwi, gateKey);
}
*/

// Read the uart and modify global variables
void uartReader() {
    /*while (true) {
        System_printf("Received return carriage via events");
        System_flush();
        continue;
    }*/

    while (true) {
        Semaphore_pend(semHandle, BIOS_WAIT_FOREVER);

        UInt gateKey;
        //use this to access shared buffers
        // The design this exercises expouses is actually basically single threaded...
        // All the overheads of multithreading but with none of the gains...
        // It's bizzare.
        gateKey = GateHwi_enter(gateHwi);

        MsgObj *msg;
        msg = Queue_get(queue);
        Queue_put(queue, &msg->elem);

        //it is now safe to read from buffers
        char input = msg->data;
        bool redrawRequired = true;
        // Left
        if (input == 'a' && xpos - MOVEINC >= 0) {
            xpos -= MOVEINC;
        // Up
        } else if (input == 'w' && ypos - MOVEINC >= 0) {
            ypos -= MOVEINC;
        // Down
        } else if (input == 's' && ypos + MOVEINC <= screenHeight - 1 - 3) {
            ypos += MOVEINC;
        // Right
        } else if (input == 'd' && xpos + MOVEINC <= screenWidth - 1 - 3) {
            xpos += MOVEINC;
        } else if (input == '\r') {
            //System_printf("Carriage return detected\n");
            //System_flush();

            if (lineNodeCount + 1 <= MAXNODES) {
                lineX[lineNodeCount] = xpos;
                lineY[lineNodeCount] = ypos;
                lineNodeCount++;
            } else {
                System_printf("Max line count reached!\n");
                System_flush();
            }
        } else if (input == '\x7f') {
            System_printf("Erasing\n");
            System_flush();

            lineNodeCount = 0;
        } else {
            redrawRequired = false;
        }

        redrawScreen(redrawRequired);

        //Use gatekey only if access shared resources is needed
        GateHwi_leave(gateHwi, gateKey);
    }
}

//Interrupt Handlers
void UARTFxn() {
    UInt gateKey;

    // Enter mutex
    gateKey = GateHwi_enter(gateHwi);

    uint32_t ui_Status;

    /* Read the current interrupt status */
    ui_Status = UARTIntStatus(UART0_BASE, true);
    /* Clear interrupt status */
    UARTIntClear(UART0_BASE, ui_Status);

    /* If you receive data from the FIFO, it will continue to receive */
    /* Receive data */
    char input = (char)UARTCharGetNonBlocking(UART0_BASE);

    if (input == '\r') {
        // Post Event 00
        Event_post(evtHandle, Event_Id_00);
    }

    // Post into queue
    MsgObj *msg;
    msg = Queue_get(queue);

    msg->id = 1;
    msg->data = input;

    Queue_put(queue, &msg->elem);

    //make sure to flag we have finished accessing buffers
    Semaphore_post(semHandle);
    GateHwi_leave(gateHwi, gateKey);
}

//This is a suggested initialisation function for simple UART implementation
void initUART () {
        //Create Hwi for UART interrupt
        Hwi_Params hwiParams;
        Hwi_Params_init(&hwiParams);

        //need to find the interrupt number for the uart and replace at XX
        UartHwiHandle = Hwi_create(INT_UART0_TM4C129, (Hwi_FuncPtr)UARTFxn , &hwiParams, NULL);

        if (UartHwiHandle == NULL) {
            System_abort("Hwi create failed");
        }
        //SysCtlClockGet()

        Types_FreqHz cpuFreq;
        BIOS_getCpuFreq(&cpuFreq);

        /* Set the mode of UART0, baud rate */
        UARTConfigSetExpClk(UART0_BASE, (uint32_t)cpuFreq.lo, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

        UARTEnable(UART0_BASE);

        /* Enable UART0 interrupt */
        IntEnable(INT_UART0);

        UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RT);
}

void redrawScreen(bool redrawRequired) {
    if (!redrawRequired) {
        return;
    }

    // Enter mutex
    //gateKey = GateHwi_enter(gateHwi);

    //redrawRequired = false;

    // Fill background
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);
    // Draw outer rectangle border
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);
    // Draw title
    GrStringDrawCentered(&sContext, "Lab 5 Task B Drawing App", -1,
            GrContextDpyWidthGet(&sContext) / 2, 10, 1);

    int i;
    int lineCount = lineNodeCount / 2;
    for (i = 0; i < lineCount; i++) {
        GrLineDraw(&sContext, lineX[i*2], lineY[i*2], lineX[i*2 + 1], lineY[i*2 + 1]);
    }

    sCursor.i16XMin = xpos;
    sCursor.i16YMin = ypos;
    sCursor.i16XMax = xpos + 3;
    sCursor.i16YMax = ypos + 3;

    // Draw cursor
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sCursor);

    //GateHwi_leave(gateHwi, gateKey);
}

void displayFxn()
{
    // Initialize the display driver.
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    // Initialize the graphics context.
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    GrContextFontSet(&sContext, &g_sFontCm20);

    screenWidth = GrContextDpyWidthGet(&sContext);
    screenHeight = GrContextDpyHeightGet(&sContext);

    // Fill background
    GrContextForegroundSet(&sContext, ClrDarkMagenta);
    GrRectFill(&sContext, &sRect);
    // Draw outer rectangle
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    GrStringDrawCentered(&sContext, "Lab 5 Task B Drawing App", -1,
            GrContextDpyWidthGet(&sContext) / 2, 10, 1);

    while (true) {
        UInt posted = Event_pend(evtHandle, Event_Id_00, Event_Id_NONE, BIOS_WAIT_FOREVER);

        if (posted == 0) {
            System_printf("Timeout expired for Event_pend()\n");
            System_flush();
            continue;
        }

        if (posted & Event_Id_00) {
            System_printf("Carriage return Event received\n");
            System_flush();
        }
    }
    /*
    while (1) {
        if (!redrawRequired) {
            continue;
        }

        // Enter mutex
        gateKey = GateHwi_enter(gateHwi);

        redrawRequired = false;

        // Fill background
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &sRect);
        // Draw outer rectangle border
        GrContextForegroundSet(&sContext, ClrWhite);
        GrRectDraw(&sContext, &sRect);
        // Draw title
        GrStringDrawCentered(&sContext, "Lab 3 Drawing App", -1,
                GrContextDpyWidthGet(&sContext) / 2, 10, 1);

        int i;
        int lineCount = lineNodeCount / 2;
        for (i = 0; i < lineCount; i++) {
            GrLineDraw(&sContext, lineX[i*2], lineY[i*2], lineX[i*2 + 1], lineY[i*2 + 1]);
        }

        sCursor.i16XMin = xpos;
        sCursor.i16YMin = ypos;
        sCursor.i16XMax = xpos + 3;
        sCursor.i16YMax = ypos + 3;

        // Draw cursor
        GrContextForegroundSet(&sContext, ClrWhite);
        GrRectDraw(&sContext, &sCursor);

        GateHwi_leave(gateHwi, gateKey);
    }
    */
}

 /*  ======== main ========*/
int main(void)
{
        // Call board init functions
        Board_initGeneral();
        Board_initGPIO();
        Board_initUART();

        g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
                | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

        // Setup events
        Event_construct(&evtStruct, NULL);

        // Obtain event instance handle
        evtHandle = Event_handle(&evtStruct);

        // Dynamically allocate a new Queue and return the handle
        MsgObj *msg = msg_mem;
        queue = Queue_create(NULL, NULL);
        Queue_put(queue, &msg->elem);

        // Construct BIOS objects
        Task_Params taskParams1;
        Task_Params taskParams2;

        // Create the task threads
        // Can use create or construct
        // Construct requires stack parameters to be set
        Task_Params_init(&taskParams1);
        taskParams1.stackSize = TASKSTACKSIZE;
        taskParams1.stack = &task1Stack;
        taskParams1.instance->name = "display";
        Task_construct(&task1Struct, (Task_FuncPtr)displayFxn, &taskParams1, NULL);

        // Create second task thread for drawing to the screen
        Task_Params_init(&taskParams2);
        taskParams2.stackSize = TASKSTACKSIZE;
        taskParams2.stack = &task2Stack;
        taskParams2.instance->name = "uartreader";
        Task_construct(&task2Struct, (Task_FuncPtr)uartReader, &taskParams2, NULL);

        // Setup a the semaphore
        Semaphore_Params semParams;

        // Construct a Semaphore object to be use as a resource lock, initial count 0
        Semaphore_Params_init(&semParams);
        Semaphore_construct(&semStruct, 0, &semParams);

        // Obtain instance handle
        semHandle = Semaphore_handle(&semStruct);

        // Create Hwi Gate Mutex
        GateHwi_Params_init(&gHwiprms);
        gateHwi = GateHwi_create(&gHwiprms, NULL);
        if (gateHwi == NULL) {
            System_abort("Gate Hwi create failed");
        }

        // Setup screen border
        sRect.i16XMin = 0;
        sRect.i16YMin = 0;
        sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
        sRect.i16YMax = GrContextDpyWidthGet(&sContext) - 1;
        //sRect.i16YMax = GrContextDpyHeightGet(&sContext);

        // Construct a Mailbox Instance
        /*
        Mailbox_Params mbxParams;
        Mailbox_Params_init(&mbxParams);
        mbxParams.readerEvent = evtHandle;
        mbxParams.readerEventId = Event_Id_02;
        Mailbox_construct(&mbxStruct, sizeof(MsgObj), 2, &mbxParams, NULL);
        mbxHandle = Mailbox_handle(&mbxStruct);
        */

        // Init UART
        initUART();

        /* SysMin will only print to the console when you call flush or exit */
        System_flush();

        /* Start BIOS */
        BIOS_start();

    return (0);
}
