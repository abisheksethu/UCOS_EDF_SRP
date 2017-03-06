/*
*********************************************************************************************************
*                                              TASK RECURSION CODE
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
*                                            TASK RECURSION CODE
*
* Filename      : OS_RecTask.c
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
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/
static CPU_INT32U  counter;
extern Tree * RecursionTree;
extern Tree * SchedulerTree;
/*OVERHEAD CALCULATION*/
extern CPU_TS StartTime;
extern CPU_TS StartTime2;
CPU_TS OverheadValue;
/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          CREATE RECURSIVE TASKs
*
* Description : This function is called by Timer0A interrupt handler. Based on the task set registered 
*               in the datastructure, it will create a new task with respective to its release time
*
* Arguments   : none
*
* Returns     : none
*
* Notes       :
*               
*********************************************************************************************************
*/

void  OSTaskHandler (void)
{ 
  CPU_INT32U rel_time = 0;
  CPU_INT32U entry = 0;
  CPU_INT32U abs_deadline = 0;
  Tree *min;
  /*--------------- FIND MINIMUM TASK RELEASE TIME -------*/    
  min = GetMinRecTree(RecursionTree);
  if( min->release_time == counter )
  { 
    entry = min->entries;
    for(int i=0; (i < entry && i <= NUM_OF_TASKS); i++)
    {
      if(min->p_tcb[i]!= NULL)
      {         
        /* --------------- SAVE ABSOlUTE RELEASE PERIOD AND ABSOLUTE DEADLINE FOR A TASK -------*/
        rel_time = counter + (min->p_tcb[i]->TaskPeriod);
        abs_deadline = counter + (min->p_tcb[i]->TaskDeadline);
        rel_time = CounterOverflow(rel_time);
        abs_deadline = CounterOverflow(abs_deadline);
        min->p_tcb[i]->TaskRelPeriod = rel_time;
        min->p_tcb[i]->TaskAbsDeadline = abs_deadline;
        /* --------------- ADD TASK TO RECURSION LIST -------*/
        RecursionTree = InsertRecTree(rel_time, RecursionTree, min->p_tcb[i]);
        /* --------------- ADD TASK TO SCHEDULING LIST -------*/
        SchedulerTree = InsertEDFTree(abs_deadline, SchedulerTree, min->p_tcb[i]);
        /* --------------- ADD TCB TO READY LIST -------*/
        OSTaskInsertTCB(min->p_tcb[i]);
      }
      else {
      }
    }
    /* --------------- REMOVE TCB FROM TASK RECURSION LIST -------*/
    RecursionTree = DelRecTree(min->release_time, RecursionTree);
  }
  else {
  
  }
  /* Update the local counter and boundary check */
  counter = counter + 1;
  counter = CounterOverflow(counter);
}
/*
*********************************************************************************************************
*                                          ADD TCB TO READY LIST
*
* Description : Add TCB to ready list with the stack ptr and state as ready
*              
*
* Arguments   : tcb
*
* Returns     : none
*
* Notes       :
*               
*********************************************************************************************************
*/
void OSTaskInsertTCB(OS_TCB *p_tcb) 
{
    CPU_STK *p_sp;
    CPU_STK_SIZE  i;
    
    /* --------------- CLEAR THE TASK'S STACK --------------- */
    if ((p_tcb->Opt & OS_OPT_TASK_STK_CHK) != (OS_OPT)0) {         /* See if stack checking has been enabled                 */
      if ((p_tcb->Opt & OS_OPT_TASK_STK_CLR) != (OS_OPT)0) {     /* See if stack needs to be cleared                       */
        p_sp = p_tcb->StkBasePtr;
        for (i = 0u; i < p_tcb->StkSize; i++) {               /* Stack grows from HIGH to LOW memory                    */
          *p_sp = (CPU_STK)0;                               /* Clear from bottom of stack and up!                     */
          p_sp++;
        }
      }
    }
    
    p_sp = OSTaskStkInit(p_tcb->TaskEntryAddr,
                         p_tcb->TaskEntryArg,
                         p_tcb->StkBasePtr,
                         p_tcb->StkLimitPtr,
                         p_tcb->StkSize,
                         p_tcb->Opt);
    
    p_tcb->StkPtr    = p_sp;                                /* Save the new top-of-stack pointer      */ 
    p_tcb->TaskState = (OS_STATE)OS_TASK_STATE_RDY;         /* Indicate that the task in READY STATE  */
    OS_PrioInsert(p_tcb->Prio);
    
#if TASK_RECURSION_DEBUG                                                                
    CPU_SR_ALLOC();                                        /* --------------- ADD TASK TO READY LIST --------------- */
    OS_CRITICAL_ENTER();
    OS_PrioInsert(p_tcb->Prio);                             
    OS_RdyListInsertTail(p_tcb);
#if OS_CFG_DBG_EN > 0u
    OS_TaskDbgListAdd(p_tcb);
#endif
    OSTaskQty++;                                            /* Increment the #tasks counter                           */
    if (OSRunning != OS_STATE_OS_RUNNING) {                 /* Return if multitasking has not started                 */
      OS_CRITICAL_EXIT();
      return;
    }    
    OS_CRITICAL_EXIT_NO_SCHED();
#endif
    
    OSEDFSched();
}

/*$PAGE*/
/*
************************************************************************************************************************
*                                                     EDF SCHEDULER
*
* Description: This function is called by other uC/OS-III services to determine whether a new, high priority task has
*              been made ready to run.  This function is invoked by TASK level code and is not used to reschedule tasks
*              from ISRs (see OSIntExit() for ISR rescheduling).
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) Rescheduling is prevented when the scheduler is locked (see OSSchedLock())
************************************************************************************************************************
*/
void  OSEDFSched (void)
{  
#if EDF_SCHEDULING_DEBUG     
  Tree * min;
  CPU_INT32U entry; 
  OS_TCB* p_tcb;
  OS_PRIO user_priority_level = 5;
  
  /* ----------FIND MINIMUM FROM THE SCHEDULER LIST ---------- */
  min = GetMinEDFTree(root2);
  entry = min->entries;
  if (min != NULL) 
  {
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    
    //CPU_INT_DIS();
    for(int j=0; (j < entry && j <= NUM_OF_TASKS); j++)
    { 
      user_priority_level++;
      p_tcb = min->p_tcb[j];
      p_tcb->Prio = user_priority_level;
      if (p_tcb == OSTCBCurPtr) 
      { 
        OS_CRITICAL_EXIT();
        CPU_INT_EN();
        return;
      }
      else 
      {  
        if((OS_PrioGetHighest() == 6) && (p_tcb->Prio == 6))
          OS_RdyListRemove(OSTCBCurPtr);
        OS_PrioInsert(p_tcb->Prio); 
        OS_RdyListInsertTail(p_tcb);
        if (OSRunning != OS_STATE_OS_RUNNING) {                 /* Return if multitasking has not started                 */
          OS_CRITICAL_EXIT();
          CPU_INT_EN();
          return;
        }    
        
      }     
    }    
    OS_CRITICAL_EXIT_NO_SCHED();
    //CPU_INT_EN();
  }
  else{
    return;
  }
#endif
  OverheadValue = ((OS_TS_GET() - StartTime2)- (StartTime2 - StartTime));
  OSSched();
}
/*
************************************************************************************************************************
*                                                    CREATE A Recursive TASK
*
* Description: This function is used to have uC/OS-III manage the execution of a task.  Tasks can either be created
*              prior to the start of multitasking or by a running task.  A task cannot be created by an ISR.
*
* Arguments  : p_tcb          is a pointer to the task's TCB
*
*              p_name         is a pointer to an ASCII string to provide a name to the task.
*
*              p_task         is a pointer to the task's code
*
*              p_arg          is a pointer to an optional data area which can be used to pass parameters to
*                             the task when the task first executes.  Where the task is concerned it thinks
*                             it was invoked and passed the argument 'p_arg' as follows:
*
*                                 void Task (void *p_arg)
*                                 {
*                                     for (;;) {
*                                         Task code;
*                                     }
*                                 }
*
*              prio           is the task's priority.  A unique priority MUST be assigned to each task and the
*                             lower the number, the higher the priority.
*
*              p_stk_base     is a pointer to the base address of the stack (i.e. low address).
*
*              stk_limit      is the number of stack elements to set as 'watermark' limit for the stack.  This value
*                             represents the number of CPU_STK entries left before the stack is full.  For example,
*                             specifying 10% of the 'stk_size' value indicates that the stack limit will be reached
*                             when the stack reaches 90% full.
*
*              stk_size       is the size of the stack in number of elements.  If CPU_STK is set to CPU_INT08U,
*                             'stk_size' corresponds to the number of bytes available.  If CPU_STK is set to
*                             CPU_INT16U, 'stk_size' contains the number of 16-bit entries available.  Finally, if
*                             CPU_STK is set to CPU_INT32U, 'stk_size' contains the number of 32-bit entries
*                             available on the stack.
*
*              q_size         is the maximum number of messages that can be sent to the task
*
*              time_quanta    amount of time (in ticks) for time slice when round-robin between tasks.  Specify 0 to use
*                             the default.
*
*              p_ext          is a pointer to a user supplied memory location which is used as a TCB extension.
*                             For example, this user memory can hold the contents of floating-point registers
*                             during a context switch, the time each task takes to execute, the number of times
*                             the task has been switched-in, etc.
*
*              opt            contains additional information (or options) about the behavior of the task.
*                             See OS_OPT_TASK_xxx in OS.H.  Current choices are:
*
*                                 OS_OPT_TASK_NONE            No option selected
*                                 OS_OPT_TASK_STK_CHK         Stack checking to be allowed for the task
*                                 OS_OPT_TASK_STK_CLR         Clear the stack when the task is created
*                                 OS_OPT_TASK_SAVE_FP         If the CPU has floating-point registers, save them
*                                                             during a context switch.
*
*              p_err          is a pointer to an error code that will be set during this call.  The value pointer
*                             to by 'p_err' can be:
*
*                                 OS_ERR_NONE                    if the function was successful.
*                                 OS_ERR_ILLEGAL_CREATE_RUN_TIME if you are trying to create the task after you called
*                                                                   OSSafetyCriticalStart().
*                                 OS_ERR_NAME                    if 'p_name' is a NULL pointer
*                                 OS_ERR_PRIO_INVALID            if the priority you specify is higher that the maximum
*                                                                   allowed (i.e. >= OS_CFG_PRIO_MAX-1) or,
*                                                                if OS_CFG_ISR_POST_DEFERRED_EN is set to 1 and you tried
*                                                                   to use priority 0 which is reserved.
*                                 OS_ERR_STK_INVALID             if you specified a NULL pointer for 'p_stk_base'
*                                 OS_ERR_STK_SIZE_INVALID        if you specified zero for the 'stk_size'
*                                 OS_ERR_STK_LIMIT_INVALID       if you specified a 'stk_limit' greater than or equal
*                                                                   to 'stk_size'
*                                 OS_ERR_TASK_CREATE_ISR         if you tried to create a task from an ISR.
*                                 OS_ERR_TASK_INVALID            if you specified a NULL pointer for 'p_task'
*                                 OS_ERR_TCB_INVALID             if you specified a NULL pointer for 'p_tcb'
*
* Returns    : A pointer to the TCB of the task created.  This pointer must be used as an ID (i.e handle) to the task.
************************************************************************************************************************
*/
/*$PAGE*/
void  OSRecTaskCreate     (OS_TCB        *p_tcb,
                    CPU_CHAR      *p_name,
                    OS_TASK_PTR    p_task,
                    void          *p_arg,
                    OS_PRIO        prio,
                    CPU_STK       *p_stk_base,
                    CPU_STK_SIZE   stk_limit,
                    CPU_STK_SIZE   stk_size,
                    OS_MSG_QTY     q_size,
                    OS_TICK        time_quanta,
                    void          *p_ext,
                    OS_OPT         opt,
                    OS_ERR        *p_err,
                    OS_TASK_PERIOD   p_task_period,
                    OS_TASK_DEADLINE       p_task_deadline)
{
    CPU_STK_SIZE   i;
#if OS_CFG_TASK_REG_TBL_SIZE > 0u
    OS_OBJ_QTY     reg_nbr;
#endif
    CPU_STK       *p_sp;
    CPU_STK       *p_stk_limit;
    //CPU_SR_ALLOC();

#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == DEF_TRUE) {
       *p_err = OS_ERR_ILLEGAL_CREATE_RUN_TIME;
        return;
    }
#endif

#if OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u
    if (OSIntNestingCtr > (OS_NESTING_CTR)0) {              /* ---------- CANNOT CREATE A TASK FROM AN ISR ---------- */
        *p_err = OS_ERR_TASK_CREATE_ISR;
        return;
    }
#endif

#if OS_CFG_ARG_CHK_EN > 0u                                  /* ---------------- VALIDATE ARGUMENTS ------------------ */
    if (p_tcb == (OS_TCB *)0) {                             /* User must supply a valid OS_TCB                        */
        *p_err = OS_ERR_TCB_INVALID;
        return;
    }
    if (p_task == (OS_TASK_PTR)0) {                         /* User must supply a valid task                          */
        *p_err = OS_ERR_TASK_INVALID;
        return;
    }
    if (p_stk_base == (CPU_STK *)0) {                       /* User must supply a valid stack base address            */
        *p_err = OS_ERR_STK_INVALID;
        return;
    }
    if (stk_size < OSCfg_StkSizeMin) {                      /* User must supply a valid minimum stack size            */
        *p_err = OS_ERR_STK_SIZE_INVALID;
        return;
    }
    if (stk_limit >= stk_size) {                            /* User must supply a valid stack limit                   */
        *p_err = OS_ERR_STK_LIMIT_INVALID;
        return;
    }
    if (prio >= OS_CFG_PRIO_MAX) {                          /* Priority must be within 0 and OS_CFG_PRIO_MAX-1        */
        *p_err = OS_ERR_PRIO_INVALID;
        return;
    }
#endif

#if OS_CFG_ISR_POST_DEFERRED_EN > 0u
    if (prio == (OS_PRIO)0) {
        if (p_tcb != &OSIntQTaskTCB) {
            *p_err = OS_ERR_PRIO_INVALID;                   /* Not allowed to use priority 0                          */
            return;
        }
    }
#endif

    if (prio == (OS_CFG_PRIO_MAX - 1u)) {
        if (p_tcb != &OSIdleTaskTCB) {
            *p_err = OS_ERR_PRIO_INVALID;                   /* Not allowed to use same priority as idle task          */
            return;
        }
    }

    OS_TaskInitTCB(p_tcb);                                  /* Initialize the TCB to default values                   */

    *p_err = OS_ERR_NONE;
                                                            /* --------------- CLEAR THE TASK'S STACK --------------- */
    if ((opt & OS_OPT_TASK_STK_CHK) != (OS_OPT)0) {         /* See if stack checking has been enabled                 */
        if ((opt & OS_OPT_TASK_STK_CLR) != (OS_OPT)0) {     /* See if stack needs to be cleared                       */
            p_sp = p_stk_base;
            for (i = 0u; i < stk_size; i++) {               /* Stack grows from HIGH to LOW memory                    */
                *p_sp = (CPU_STK)0;                         /* Clear from bottom of stack and up!                     */
                p_sp++;
            }
        }
    }
                                                            /* ------- INITIALIZE THE STACK FRAME OF THE TASK ------- */
#if (CPU_CFG_STK_GROWTH == CPU_STK_GROWTH_HI_TO_LO)
    p_stk_limit = p_stk_base + stk_limit;
#else
    p_stk_limit = p_stk_base + (stk_size - 1u) - stk_limit;
#endif

    p_sp = OSTaskStkInit(p_task,
                         p_arg,
                         p_stk_base,
                         p_stk_limit,
                         stk_size,
                         opt);

                                                            /* -------------- INITIALIZE THE TCB FIELDS ------------- */
    p_tcb->TaskEntryAddr = p_task;                          /* Save task entry point address                          */
    p_tcb->TaskEntryArg  = p_arg;                           /* Save task entry argument                               */

    p_tcb->NamePtr       = p_name;                          /* Save task name                                         */

    p_tcb->Prio          = prio;                            /* Save the task's priority                               */

    p_tcb->StkPtr        = p_sp;                            /* Save the new top-of-stack pointer                      */
    p_tcb->StkLimitPtr   = p_stk_limit;                     /* Save the stack limit pointer                           */

    p_tcb->TimeQuanta    = time_quanta;                     /* Save the #ticks for time slice (0 means not sliced)    */
    counter              = 0;
    p_tcb->TaskPeriod    = p_task_period;                   /* Save Release time */
    p_tcb->TaskRelPeriod = p_task_period;                   /* Initial release time to be assumed as zero */        
    p_tcb->TaskDeadline  = p_task_deadline;                 /* Save Deadline */
    p_tcb->TaskAbsDeadline = p_task_deadline;                 /* Save Absolute Deadline */   
    
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
    if (time_quanta == (OS_TICK)0) {
        p_tcb->TimeQuantaCtr = OSSchedRoundRobinDfltTimeQuanta;
    } else {
        p_tcb->TimeQuantaCtr = time_quanta;
    }
#endif
    p_tcb->ExtPtr        = p_ext;                           /* Save pointer to TCB extension                          */
    p_tcb->StkBasePtr    = p_stk_base;                      /* Save pointer to the base address of the stack          */
    p_tcb->StkSize       = stk_size;                        /* Save the stack size (in number of CPU_STK elements)    */
    p_tcb->Opt           = opt;                             /* Save task options                                      */
#if OS_CFG_TASK_REG_TBL_SIZE > 0u
    for (reg_nbr = 0u; reg_nbr < OS_CFG_TASK_REG_TBL_SIZE; reg_nbr++) {
        p_tcb->RegTbl[reg_nbr] = (OS_REG)0;
    }
#endif

#if OS_CFG_TASK_Q_EN > 0u
    OS_MsgQInit(&p_tcb->MsgQ,                               /* Initialize the task's message queue                    */
                q_size);
#endif

    OSTaskCreateHook(p_tcb);                                /* Call user defined hook                                 */
  
    RecursionTree = InsertRecTree(p_tcb->TaskRelPeriod, RecursionTree, p_tcb);         /* --------------- ADD TCB TO RECURSION LIST --------------- */
    SchedulerTree = InsertEDFTree(p_tcb->TaskAbsDeadline, SchedulerTree, p_tcb); /* --------------- ADD TCB TO SCHEDULNG LIST --------------- */  
     
#if TASK_RECURSION_DEBUG                                                                 
    CPU_SR_ALLOC();                                           /* --------------- ADD TASK TO READY LIST --------------- */
    OS_CRITICAL_ENTER();
    OS_PrioInsert(p_tcb->Prio);                             
    OS_RdyListInsertTail(p_tcb);
#if OS_CFG_DBG_EN > 0u
    OS_TaskDbgListAdd(p_tcb);
#endif
    OSTaskQty++;                                            /* Increment the #tasks counter                           */
    if (OSRunning != OS_STATE_OS_RUNNING) {                 /* Return if multitasking has not started                 */
      OS_CRITICAL_EXIT();
      return;
    }    
    OS_CRITICAL_EXIT_NO_SCHED();
#endif
    
    OSEDFSched();
}
                     
/*
************************************************************************************************************************
*                                                     DELETE A RECURSIVE TASK
*
* Description: This function allows you to delete a task.  The calling task can delete itself by specifying a NULL
*              pointer for 'p_tcb'.  The deleted task is returned to the dormant state and can be re-activated by
*              creating the deleted task again.
*
* Arguments  : p_tcb      is the TCB of the tack to delete
*
*              p_err      is a pointer to an error code returned by this function:
*
*                             OS_ERR_NONE                  if the call is successful
*                             OS_ERR_STATE_INVALID         if the state of the task is invalid
*                             OS_ERR_TASK_DEL_IDLE         if you attempted to delete uC/OS-III's idle task
*                             OS_ERR_TASK_DEL_INVALID      if you attempted to delete uC/OS-III's ISR handler task
*                             OS_ERR_TASK_DEL_ISR          if you tried to delete a task from an ISR
************************************************************************************************************************
*/

void  OSRecTaskDel (OS_TCB  *p_tcb,
                 OS_ERR  *p_err)
{
    CPU_SR_ALLOC();

#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u
    if (OSIntNestingCtr > (OS_NESTING_CTR)0) {              /* See if trying to delete from ISR                       */
       *p_err = OS_ERR_TASK_DEL_ISR;
        return;
    }
#endif

    if (p_tcb == &OSIdleTaskTCB) {                          /* Not allowed to delete the idle task                    */
        *p_err = OS_ERR_TASK_DEL_IDLE;
        return;
    }

#if OS_CFG_ISR_POST_DEFERRED_EN > 0u
    if (p_tcb == &OSIntQTaskTCB) {                          /* Cannot delete the ISR handler task                     */
        *p_err = OS_ERR_TASK_DEL_INVALID;
        return;
    }
#endif

    if (p_tcb == (OS_TCB *)0) {                             /* Delete 'Self'?                                         */
        CPU_CRITICAL_ENTER();
        p_tcb  = OSTCBCurPtr;                               /* Yes.                                                   */
        CPU_CRITICAL_EXIT();
    }

    OS_CRITICAL_ENTER();
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:
             OS_RdyListRemove(p_tcb);
             break;

        case OS_TASK_STATE_SUSPENDED:
             break;

        case OS_TASK_STATE_DLY:                             /* Task is only delayed, not on any wait list             */
        case OS_TASK_STATE_DLY_SUSPENDED:
             OS_TickListRemove(p_tcb);
             break;

        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             OS_TickListRemove(p_tcb);
             switch (p_tcb->PendOn) {                       /* See what we are pending on                             */
                 case OS_TASK_PEND_ON_NOTHING:
                 case OS_TASK_PEND_ON_TASK_Q:               /* There is no wait list for these two                    */
                 case OS_TASK_PEND_ON_TASK_SEM:
                      break;

                 case OS_TASK_PEND_ON_FLAG:                 /* Remove from wait list                                  */
                 case OS_TASK_PEND_ON_MULTI:
                 case OS_TASK_PEND_ON_MUTEX:
                 case OS_TASK_PEND_ON_Q:
                 case OS_TASK_PEND_ON_SEM:
                      OS_PendListRemove(p_tcb);
                      break;

                 default:
                      break;
             }
             break;

        default:
            OS_CRITICAL_EXIT();
            *p_err = OS_ERR_STATE_INVALID;
            return;
    }

#if OS_CFG_TASK_Q_EN > 0u
    (void)OS_MsgQFreeAll(&p_tcb->MsgQ);                     /* Free task's message queue messages                     */
#endif

    OSTaskDelHook(p_tcb);                                   /* Call user defined hook                                 */

#if OS_CFG_DBG_EN > 0u
    OS_TaskDbgListRemove(p_tcb);
#endif
    OSTaskQty--;                                            /* One less task being managed                            */

    OS_TaskRecDelTCB(p_tcb);                                /* Initialize the TCB to default values                   */

    OS_CRITICAL_EXIT_NO_SCHED();
                                                             /* ---------- DELETE COMPLETED TCB IN SCHEDULER LIST ---------- */
    SchedulerTree = DelEDFTree(p_tcb->TaskAbsDeadline, SchedulerTree);
      
    OSEDFSched();                                              /* Find new highest priority task - EDF Scheduling */

    *p_err = OS_ERR_NONE;
}

/*$PAGE*/
/*
************************************************************************************************************************
*                                                     DELETE A APP START TASK
*
* Description: This function allows you to delete a task.  The calling task can delete itself by specifying a NULL
*              pointer for 'p_tcb'.  The deleted task is returned to the dormant state and can be re-activated by
*              creating the deleted task again.
*
* Arguments  : p_tcb      is the TCB of the tack to delete
*
*              p_err      is a pointer to an error code returned by this function:
*
*                             OS_ERR_NONE                  if the call is successful
*                             OS_ERR_STATE_INVALID         if the state of the task is invalid
*                             OS_ERR_TASK_DEL_IDLE         if you attempted to delete uC/OS-III's idle task
*                             OS_ERR_TASK_DEL_INVALID      if you attempted to delete uC/OS-III's ISR handler task
*                             OS_ERR_TASK_DEL_ISR          if you tried to delete a task from an ISR
************************************************************************************************************************
*/

void  OSTaskAppStartDel (OS_TCB  *p_tcb,
                 OS_ERR  *p_err)
{
    CPU_SR_ALLOC();

#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u
    if (OSIntNestingCtr > (OS_NESTING_CTR)0) {              /* See if trying to delete from ISR                       */
       *p_err = OS_ERR_TASK_DEL_ISR;
        return;
    }
#endif

    if (p_tcb == &OSIdleTaskTCB) {                          /* Not allowed to delete the idle task                    */
        *p_err = OS_ERR_TASK_DEL_IDLE;
        return;
    }

#if OS_CFG_ISR_POST_DEFERRED_EN > 0u
    if (p_tcb == &OSIntQTaskTCB) {                          /* Cannot delete the ISR handler task                     */
        *p_err = OS_ERR_TASK_DEL_INVALID;
        return;
    }
#endif

    if (p_tcb == (OS_TCB *)0) {                             /* Delete 'Self'?                                         */
        CPU_CRITICAL_ENTER();
        p_tcb  = OSTCBCurPtr;                               /* Yes.                                                   */
        CPU_CRITICAL_EXIT();
    }

    OS_CRITICAL_ENTER();
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:
             OS_RdyListRemove(p_tcb);
             break;

        case OS_TASK_STATE_SUSPENDED:
             break;

        case OS_TASK_STATE_DLY:                             /* Task is only delayed, not on any wait list             */
        case OS_TASK_STATE_DLY_SUSPENDED:
             OS_TickListRemove(p_tcb);
             break;

        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             OS_TickListRemove(p_tcb);
             switch (p_tcb->PendOn) {                       /* See what we are pending on                             */
                 case OS_TASK_PEND_ON_NOTHING:
                 case OS_TASK_PEND_ON_TASK_Q:               /* There is no wait list for these two                    */
                 case OS_TASK_PEND_ON_TASK_SEM:
                      break;

                 case OS_TASK_PEND_ON_FLAG:                 /* Remove from wait list                                  */
                 case OS_TASK_PEND_ON_MULTI:
                 case OS_TASK_PEND_ON_MUTEX:
                 case OS_TASK_PEND_ON_Q:
                 case OS_TASK_PEND_ON_SEM:
                      OS_PendListRemove(p_tcb);
                      break;

                 default:
                      break;
             }
             break;

        default:
            OS_CRITICAL_EXIT();
            *p_err = OS_ERR_STATE_INVALID;
            return;
    }

#if OS_CFG_TASK_Q_EN > 0u
    (void)OS_MsgQFreeAll(&p_tcb->MsgQ);                     /* Free task's message queue messages                     */
#endif

    OSTaskDelHook(p_tcb);                                   /* Call user defined hook                                 */

#if OS_CFG_DBG_EN > 0u
    OS_TaskDbgListRemove(p_tcb);
#endif
    OSTaskQty--;                                            /* One less task being managed                            */

    OS_TaskInitTCB(p_tcb);                                  /* Initialize the TCB to default values                   */
    p_tcb->TaskState = (OS_STATE)OS_TASK_STATE_DEL;         /* Indicate that the task was deleted                     */

    OS_CRITICAL_EXIT_NO_SCHED();
    OSSched();                                              /* Find new highest priority task                         */

    *p_err = OS_ERR_NONE;
}

/*$PAGE*/
/*
************************************************************************************************************************
*                                               DELETE REQUIRED TCB FIELDS FOR TASK RECURSION
*
* Description: This function is called to initialize a TCB to default values
*
* Arguments  : p_tcb    is a pointer to the TCB to initialize
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TaskRecDelTCB (OS_TCB *p_tcb)
{
#if OS_CFG_TASK_REG_TBL_SIZE > 0u
    OS_REG_ID   id;
#endif
#if OS_CFG_TASK_PROFILE_EN > 0u
    CPU_TS      ts;
#endif


    //p_tcb->StkPtr             = (CPU_STK       *)0;
    //p_tcb->StkLimitPtr        = (CPU_STK       *)0;

    //p_tcb->ExtPtr             = (void          *)0;

    p_tcb->NextPtr            = (OS_TCB        *)0;
    p_tcb->PrevPtr            = (OS_TCB        *)0;

    p_tcb->TickNextPtr        = (OS_TCB        *)0;
    p_tcb->TickPrevPtr        = (OS_TCB        *)0;
    p_tcb->TickSpokePtr       = (OS_TICK_SPOKE *)0;

    //p_tcb->NamePtr            = (CPU_CHAR      *)((void *)"?Task");

    //p_tcb->StkBasePtr         = (CPU_STK       *)0;

    //p_tcb->TaskEntryAddr      = (OS_TASK_PTR    )0;
    //p_tcb->TaskEntryArg       = (void          *)0;

#if (OS_CFG_PEND_MULTI_EN > 0u)
    p_tcb->PendDataTblPtr     = (OS_PEND_DATA  *)0;
    p_tcb->PendDataTblEntries = (OS_OBJ_QTY     )0u;
#endif

    p_tcb->TS                 = (CPU_TS         )0u;

#if (OS_MSG_EN > 0u)
    //p_tcb->MsgPtr             = (void          *)0;
    //p_tcb->MsgSize            = (OS_MSG_SIZE    )0u;
#endif

#if OS_CFG_TASK_Q_EN > 0u
    OS_MsgQInit(&p_tcb->MsgQ,
                (OS_MSG_QTY)0u);
#if OS_CFG_TASK_PROFILE_EN > 0u
    p_tcb->MsgQPendTime       = (CPU_TS         )0u;
    p_tcb->MsgQPendTimeMax    = (CPU_TS         )0u;
#endif
#endif

#if OS_CFG_FLAG_EN > 0u
    p_tcb->FlagsPend          = (OS_FLAGS       )0u;
    p_tcb->FlagsOpt           = (OS_OPT         )0u;
    p_tcb->FlagsRdy           = (OS_FLAGS       )0u;
#endif

#if OS_CFG_TASK_REG_TBL_SIZE > 0u
    for (id = 0u; id < OS_CFG_TASK_REG_TBL_SIZE; id++) {
        p_tcb->RegTbl[id] = (OS_REG)0u;
    }
#endif

    p_tcb->SemCtr             = (OS_SEM_CTR     )0u;
#if OS_CFG_TASK_PROFILE_EN > 0u
    p_tcb->SemPendTime        = (CPU_TS         )0u;
    p_tcb->SemPendTimeMax     = (CPU_TS         )0u;
#endif

    //p_tcb->StkSize            = (CPU_STK_SIZE   )0u;


#if OS_CFG_TASK_SUSPEND_EN > 0u
    p_tcb->SuspendCtr         = (OS_NESTING_CTR )0u;
#endif

#if OS_CFG_STAT_TASK_STK_CHK_EN > 0u
    p_tcb->StkFree            = (CPU_STK_SIZE   )0u;
    p_tcb->StkUsed            = (CPU_STK_SIZE   )0u;
#endif

    // p_tcb->Opt                = (OS_OPT         )0u;

    p_tcb->TickCtrPrev        = (OS_TICK        )OS_TICK_TH_INIT;
    p_tcb->TickCtrMatch       = (OS_TICK        )0u;
    p_tcb->TickRemain         = (OS_TICK        )0u;

   // p_tcb->TimeQuanta         = (OS_TICK        )0u;
   // p_tcb->TimeQuantaCtr      = (OS_TICK        )0u;

#if OS_CFG_TASK_PROFILE_EN > 0u
    p_tcb->CPUUsage           = (OS_CPU_USAGE   )0u;
    p_tcb->CtxSwCtr           = (OS_CTX_SW_CTR  )0u;
    p_tcb->CyclesDelta        = (CPU_TS         )0u;
    ts                        = OS_TS_GET();                /* Read the current timestamp and save                    */
    p_tcb->CyclesStart        = ts;
    p_tcb->CyclesTotal        = (OS_CYCLES      )0u;
#endif
#ifdef CPU_CFG_INT_DIS_MEAS_EN
    p_tcb->IntDisTimeMax      = (CPU_TS         )0u;
#endif
#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
    p_tcb->SchedLockTimeMax   = (CPU_TS         )0u;
#endif

    p_tcb->PendOn             = (OS_STATE       )OS_TASK_PEND_ON_NOTHING;
    p_tcb->PendStatus         = (OS_STATUS      )OS_STATUS_PEND_OK;
    p_tcb->TaskState          = (OS_STATE       )OS_TASK_STATE_DEL;

    //p_tcb->Prio               = (OS_PRIO        )OS_PRIO_INIT;

#if OS_CFG_DBG_EN > 0u
    p_tcb->DbgPrevPtr         = (OS_TCB        *)0;
    p_tcb->DbgNextPtr         = (OS_TCB        *)0;
    p_tcb->DbgNamePtr         = (CPU_CHAR      *)((void *)" ");
#endif
}
/*
*********************************************************************************************************
*                                          RESETTING THE COUNTER
*
* Description : If a 32bit unsigned integer crosses to window 1, it resets to window 0
*
* Arguments   : 32 bit unsigned integer in window 0/1
*
* Returns     : 32 bit unsigned integer in window 0
*
* Notes       : Window 0 --> counter < BORDER_VALUE
*               Window 1 --> counter > BORDER_VALUE
*               Application should use for the 32 bit unsigned integers variables,
*               which are not used for comparision.(lesser than or greater than)
*               
*********************************************************************************************************
*/
CPU_INT32U CounterOverflow (CPU_INT32U counter) {
	CPU_INT32U window = 0;
	window = (counter & BORDER_BIT) > 7;
	if(window == 1) {
		counter = counter - BORDER_VALUE;
	}
	else {
		counter = counter;	
	}
	return counter;
}