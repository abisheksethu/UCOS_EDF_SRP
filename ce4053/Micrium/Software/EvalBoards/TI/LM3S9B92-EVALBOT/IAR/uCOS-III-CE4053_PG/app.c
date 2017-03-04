/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                          (c) Copyright 2009-2010; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            APPLICATION CODE
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : Team Tesla
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>
#include <stdio.h>
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include <os.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define ONESECONDTICK             7000000

#define WORKLOAD1                     3
#define WORKLOAD2                     2

#define TIMERDIV                      (BSP_CPUClkFreq() / (CPU_INT32U)OSCfg_TickRate_Hz)

/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/
static  OS_TCB       AppTaskStartTCB;
static  CPU_STK      AppTaskStartStk[APP_TASK_START_STK_SIZE];

static  OS_TCB       AppTaskOneTCB;
static  CPU_STK      AppTaskOneStk[APP_TASK_ONE_STK_SIZE];      

static  OS_TCB       AppTaskTwoTCB;
static  CPU_STK      AppTaskTwoStk[APP_TASK_TWO_STK_SIZE];

static  OS_TCB       AppTaskThreeTCB;
static  CPU_STK      AppTaskThreeStk[APP_TASK_ONE_STK_SIZE];

static  OS_TCB       AppTaskFourTCB;
static  CPU_STK      AppTaskFourStk[APP_TASK_TWO_STK_SIZE];

static  OS_TCB       AppTaskFiveTCB;
static  CPU_STK      AppTaskFiveStk[APP_TASK_ONE_STK_SIZE];

CPU_INT32U      iCnt = 0;
CPU_INT08U      Left_tgt;
CPU_INT08U      Right_tgt;
CPU_INT32U      iToken  = 0;
CPU_INT32U      iCounter= 1;
CPU_INT32U      iMove   = 10;
CPU_INT32U      measure=0;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void        AppRobotMotorDriveSensorEnable    ();
        void        IntWheelSensor                    ();
        void        RoboTurn                          (tSide dir, CPU_INT16U seg, CPU_INT16U speed);

static  void        AppTaskStart                 (void  *p_arg);
static  void        AppTaskOne                   (void  *p_arg);
static  void        AppTaskTwo                   (void  *p_arg);
static  void        AppTaskThree                 (void  *p_arg);
static  void        AppTaskFour                  (void  *p_arg);
static  void        AppTaskFive                  (void  *p_arg);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR  err;

    BSP_IntDisAll();                                            /* Disable all interrupts.                              */
    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB,                /* Create the start task                                */
                 (CPU_CHAR   *)"App Task Start",
                 (OS_TASK_PTR ) AppTaskStart,
                 (void       *) 0,
                 (OS_PRIO     ) APP_TASK_START_PRIO,
                 (CPU_STK    *)&AppTaskStartStk[0],
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE / 10u,
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0u,
                 (OS_TICK     ) 0u,
                 (void       *) (CPU_INT32U) 0, 
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */
}

/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is a startup task. BSP init creates a hardware Timer0A with interrupt for every one second
*               Initialize the ticker only once when multitasking has started.
*
* Arguments   : p_arg is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 
* 
*********************************************************************************************************
*/

static  void  AppTaskStart (void  *p_arg)
{
    CPU_INT32U  clk_freq;
    CPU_INT32U  cnts;
    OS_ERR      err;
    (void)&p_arg;
    BSP_Init();                                                 /* Initialize BSP functions                             */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */
    clk_freq = BSP_CPUClkFreq();                                /* Determine SysTick reference freq.                    */
    cnts     = clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;        /* Determine nbr SysTick increments                     */
    OS_CPU_SysTickInit(cnts);                                   /* Init uC/OS periodic time src (SysTick).              */
    CPU_TS_TmrFreqSet(clk_freq);
    
    /* Enable Wheel ISR Interrupt */
    AppRobotMotorDriveSensorEnable();
    
    /*Splay Tree Initialization*/
    Splay_Tree_Init();
    
    /*Release the first task set - OSRecTask() - Insert all recursive tasks into list and delete task accordingly*/ 
    OSRecTaskCreate((OS_TCB     *)&AppTaskOneTCB, (CPU_CHAR   *)"leftLEDBlink", (OS_TASK_PTR ) AppTaskOne, (void       *) 0, (OS_PRIO     ) APP_TASK_ONE_PRIO, (CPU_STK    *)&AppTaskOneStk[0], (CPU_STK_SIZE) APP_TASK_ONE_STK_SIZE / 10u, (CPU_STK_SIZE) APP_TASK_ONE_STK_SIZE, (OS_MSG_QTY  ) 0u, (OS_TICK     ) 0u, (void       *)(CPU_INT32U) 1, (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), (OS_ERR     *)&err, (OS_TASK_PERIOD)TASK1PERIOD, (OS_TASK_DEADLINE)TASK1PERIOD);
    OSRecTaskCreate((OS_TCB     *)&AppTaskTwoTCB, (CPU_CHAR   *)"rightLEDBlink", (OS_TASK_PTR ) AppTaskTwo, (void       *) 0, (OS_PRIO     ) APP_TASK_TWO_PRIO, (CPU_STK    *)&AppTaskTwoStk[0], (CPU_STK_SIZE) APP_TASK_TWO_STK_SIZE / 10u, (CPU_STK_SIZE) APP_TASK_TWO_STK_SIZE, (OS_MSG_QTY  ) 0u, (OS_TICK     ) 0u, (void       *) (CPU_INT32U) 2, (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), (OS_ERR     *)&err, (OS_TASK_PERIOD)TASK2PERIOD, (OS_TASK_DEADLINE)TASK2PERIOD);
    OSRecTaskCreate((OS_TCB     *)&AppTaskThreeTCB, (CPU_CHAR   *)"LEDBlink", (OS_TASK_PTR ) AppTaskThree, (void       *) 0, (OS_PRIO     ) APP_TASK_THREE_PRIO, (CPU_STK    *)&AppTaskThreeStk[0], (CPU_STK_SIZE) APP_TASK_THREE_STK_SIZE / 10u, (CPU_STK_SIZE) APP_TASK_THREE_STK_SIZE, (OS_MSG_QTY  ) 0u, (OS_TICK     ) 0u, (void       *) (CPU_INT32U) 2, (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), (OS_ERR     *)&err, (OS_TASK_PERIOD)TASK3PERIOD, (OS_TASK_DEADLINE)TASK3PERIOD);
    OSRecTaskCreate((OS_TCB     *)&AppTaskFourTCB, (CPU_CHAR   *)"moveForward", (OS_TASK_PTR ) AppTaskFour, (void       *) 0, (OS_PRIO     ) APP_TASK_FOUR_PRIO, (CPU_STK    *)&AppTaskFourStk[0], (CPU_STK_SIZE) APP_TASK_FOUR_STK_SIZE / 10u, (CPU_STK_SIZE) APP_TASK_FOUR_STK_SIZE, (OS_MSG_QTY  ) 0u, (OS_TICK     ) 0u, (void       *) (CPU_INT32U) 2, (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), (OS_ERR     *)&err, (OS_TASK_PERIOD)TASK4PERIOD, (OS_TASK_DEADLINE)TASK4PERIOD);
    OSRecTaskCreate((OS_TCB     *)&AppTaskFiveTCB, (CPU_CHAR   *)"moveBackward", (OS_TASK_PTR ) AppTaskFive, (void       *) 0, (OS_PRIO     ) APP_TASK_FIVE_PRIO, (CPU_STK    *)&AppTaskFiveStk[0], (CPU_STK_SIZE) APP_TASK_FIVE_STK_SIZE / 10u, (CPU_STK_SIZE) APP_TASK_FIVE_STK_SIZE, (OS_MSG_QTY  ) 0u, (OS_TICK     ) 0u, (void       *) (CPU_INT32U) 2, (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), (OS_ERR     *)&err, (OS_TASK_PERIOD)TASK5PERIOD, (OS_TASK_DEADLINE)TASK5PERIOD);
    
    /* Delete AppTaskStart */
    OSRecTaskDel((OS_TCB *)0, &err);
}
/*
*********************************************************************************************************
*                                          USER DEFINED TASK Definition
*
* Description : These are  user defined tasks 
*
*
* Arguments   : p_arg is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 
* 
*********************************************************************************************************
*/
void  AppTaskOne (void  *p_arg)
{ 
    OS_ERR      err;
    CPU_INT32U  i,k,j=0;
   
    for(i=0; i <(ONESECONDTICK); i++)
    {
      j = ((i * 2) + j);
    }
    
    BSP_LED_Off(2u);
    for(k=0; k<1; k++)
    {
      BSP_LED_Toggle(2u);
      for(i=0; i <ONESECONDTICK/2; i++)
         j = ((i * 2)+j);
    }
    
   BSP_LED_Off(2u);
   
   OSRecTaskDel((OS_TCB *)0, &err); 

}
void  AppTaskTwo (void  *p_arg)
{   
    OS_ERR      err;
    CPU_INT32U  i,k,j=0;
   
    for(i=0; i <(ONESECONDTICK); i++)
    {
      j = ((i * 2) + j);
    }
    
    BSP_LED_Off(1u);
    for(k=0; k<1; k++)
    {
      BSP_LED_Toggle(1u);
      for(i=0; i <ONESECONDTICK/2; i++)
         j = ((i * 2)+j);
    }
    
   BSP_LED_Off(1u);
   
   OSRecTaskDel((OS_TCB *)0, &err);
}

void  AppTaskThree (void  *p_arg)
{   
    OS_ERR      err;
    CPU_INT32U  i,k,j=0;
   
    for(i=0; i <(ONESECONDTICK); i++)
    {
      j = ((i * 2) + j);
    }
    
    BSP_LED_Off(0u);
    for(k=0; k<1; k++)
    {
      BSP_LED_Toggle(0u);
      for(i=0; i <ONESECONDTICK/2; i++)
         j = ((i * 2)+j);
    }
    
   BSP_LED_Off(0u);
   
   OSRecTaskDel((OS_TCB *)0, &err);
}

void  AppTaskFour (void  *p_arg)
{ 
    OS_ERR      err;
    CPU_INT32U  k, i, j;
 
    RoboTurn(FRONT, 16, 50);
    
    for(k=0; k<WORKLOAD1; k++)
    {
      for(i=0; i <ONESECONDTICK; i++){
        j=2*i;
      }
     }
    
    OSRecTaskDel((OS_TCB *)0, &err);   
}

void  AppTaskFive (void  *p_arg)
{ 
    OS_ERR      err;
    CPU_INT32U  k, i, j;
  
    RoboTurn(BACK, 16, 50);
    
    for(k=0; k<WORKLOAD1; k++)
    {
      for(i=0; i <ONESECONDTICK; i++){
        j=2*i;
      }
     }
  
    OSRecTaskDel((OS_TCB *)0, &err);   
}

static  void  AppRobotMotorDriveSensorEnable ()
{
    BSP_WheelSensorEnable();
    BSP_WheelSensorIntEnable(RIGHT_SIDE, SENSOR_A, (CPU_FNCT_VOID)IntWheelSensor);
    BSP_WheelSensorIntEnable(LEFT_SIDE, SENSOR_A, (CPU_FNCT_VOID)IntWheelSensor);
}


void IntWheelSensor()
{
	CPU_INT32U         ulStatusR_A;
	CPU_INT32U         ulStatusL_A;

	static CPU_INT08U CountL = 0;
	static CPU_INT08U CountR = 0;

	static CPU_INT08U data = 0;

	ulStatusR_A = GPIOPinIntStatus(RIGHT_IR_SENSOR_A_PORT, DEF_TRUE);
	ulStatusL_A = GPIOPinIntStatus(LEFT_IR_SENSOR_A_PORT, DEF_TRUE);

        if (ulStatusR_A & RIGHT_IR_SENSOR_A_PIN)
        {
          GPIOPinIntClear(RIGHT_IR_SENSOR_A_PORT, RIGHT_IR_SENSOR_A_PIN);           /* Clear interrupt.*/
          CountR = CountR + 1;
        }

        if (ulStatusL_A & LEFT_IR_SENSOR_A_PIN)
        {
          GPIOPinIntClear(LEFT_IR_SENSOR_A_PORT, LEFT_IR_SENSOR_A_PIN);
          CountL = CountL + 1;
        }

	if((CountL >= Left_tgt) && (CountR >= Right_tgt))
        {
          data = 0x11;
          Left_tgt = 0;
          Right_tgt = 0;
          CountL = 0;
          CountR = 0;
          BSP_MotorStop(LEFT_SIDE);
          BSP_MotorStop(RIGHT_SIDE);
        }
        else if(CountL >= Left_tgt)
        {
          data = 0x10;
          Left_tgt = 0;
          CountL = 0;
          BSP_MotorStop(LEFT_SIDE);
        }
        else if(CountR >= Right_tgt)
        {
          data = 0x01;
          Right_tgt = 0;
          CountR = 0;
          BSP_MotorStop(RIGHT_SIDE);
        }
        return;
}

void RoboTurn(tSide dir, CPU_INT16U seg, CPU_INT16U speed)
{
	Left_tgt = seg;
        Right_tgt = seg;

	BSP_MotorStop(LEFT_SIDE);
	BSP_MotorStop(RIGHT_SIDE);

        BSP_MotorSpeed(LEFT_SIDE, speed <<8u);
	BSP_MotorSpeed(RIGHT_SIDE,speed <<8u);

	switch(dir)
	{
            case FRONT :
                    BSP_MotorDir(RIGHT_SIDE,FORWARD);
                    BSP_MotorDir(LEFT_SIDE,FORWARD);
                    BSP_MotorRun(LEFT_SIDE);
                    BSP_MotorRun(RIGHT_SIDE);
                    break;
                    
            case BACK :
                    BSP_MotorDir(LEFT_SIDE,REVERSE);
                    BSP_MotorDir(RIGHT_SIDE,REVERSE);
                    BSP_MotorRun(RIGHT_SIDE);
                    BSP_MotorRun(LEFT_SIDE);
                    break;
                    
            case LEFT_SIDE :
                    BSP_MotorDir(RIGHT_SIDE,FORWARD);
                    BSP_MotorDir(LEFT_SIDE,REVERSE);
                    BSP_MotorRun(LEFT_SIDE);
                    BSP_MotorRun(RIGHT_SIDE);
                    break;
                    
            case RIGHT_SIDE:
                    BSP_MotorDir(LEFT_SIDE,FORWARD);
                    BSP_MotorDir(RIGHT_SIDE,REVERSE);
                    BSP_MotorRun(RIGHT_SIDE);
                    BSP_MotorRun(LEFT_SIDE);
                    break;
                    
            default:
                    BSP_MotorStop(LEFT_SIDE);
                    BSP_MotorStop(RIGHT_SIDE);
                    break;
	}

	return;
}
