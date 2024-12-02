/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice and the following disclaimer.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_task.c
 * @brief       task management C file
 * @version     V1.2021.05
 * @authors     Yiqing Huang
 * @date        2021 MAY
 *
 * @attention   assumes NO HARDWARE INTERRUPTS
 * @details     The starter code shows one way of implementing context switching.
 *              The code only has minimal sanity check.
 *              There is no stack overflow check.
 *              The implementation assumes only three simple tasks and
 *              NO HARDWARE INTERRUPTS.
 *              The purpose is to show how context switch could be done
 *              under stated assumptions.
 *              These assumptions are not true in the required RTX Project!!!
 *              Understand the assumptions and the limitations of the code before
 *              using the code piece in your own project!!!
 *
 *****************************************************************************/


#include "k_inc.h"
//#include "k_task.h"
#include "k_rtx.h"

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

TCB             *gp_current_task = NULL;    // the current RUNNING task
TCB             g_tcbs[MAX_TASKS];          // an array of TCBs
//TASK_INIT       g_null_task_info;           // The null task info
U32             g_num_active_tasks = 0;     // number of non-dormant tasks
task_t sending_priority_queue[MAX_TASKS][MAX_TASKS - 1];
task_t rt_sending_priority_queue[MAX_TASKS][MAX_TASKS - 1];
int sending_priority_queue_beginning[MAX_TASKS];
U32 rt_sending_priority_queue_task_count[MAX_TASKS];
// BOOL blk_recieving_array[MAX_TASKS];

TASK_RT rt_priority_queue[MAX_TASKS];

TCB *priority_queue[5];
U32 num_of_rt_tasks = 0;
BOOL is_uart = FALSE;

// function declarations

int k_tsk_get(task_t tid, RTX_TASK_INFO *buffer);
void sort_rt_tsk(void);
void sort_time_increment(volatile uint32_t ticks);

/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
    g_k_stacks[MAX_TASKS-1]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */


/**************************************************************************//**
 * @brief   scheduler, pick the TCB of the next to run task
 *
 * @return  TCB pointer of the next to run task
 * @post    gp_curret_task is updated
 * @note    you need to change this one to be a priority scheduler
 *
 *****************************************************************************/

TCB *scheduler(void)
{
    /* *****MODIFY THIS FUNCTION ***** */
    // task_t tid = gp_current_task->tid;
    // U8 prio = gp_current_task->prio;

    TCB *dummy_task = NULL; // dummy task

    for (int i = 0; i < 5; i++) { // go through each level of priority in the priority queue 
        if (priority_queue[i] != NULL) {
            dummy_task = priority_queue[i];
            priority_queue[i] = priority_queue[i]->next;

            if (priority_queue[i] != NULL) {
                priority_queue[i]->prev = NULL;
            }

            dummy_task->next = NULL;
            dummy_task->prev = NULL;

            break;
        }
    }

    // return &g_tcbs[(++tid)%g_num_active_tasks];
    return dummy_task;
}

void add_tsk_to_front_of_high(TCB *tsk_to_add) {
    tsk_to_add->next = priority_queue[0];
    if(priority_queue[0] != NULL) {
        priority_queue[0]->prev = tsk_to_add;
    }
    priority_queue[0] = tsk_to_add;
}

int tsk_run_new_finish_send(TCB *task_to_run)
{
    TCB *p_tcb_old = NULL;
    
    if (gp_current_task == NULL) {
        return RTX_ERR;
    }

    p_tcb_old = gp_current_task;
    gp_current_task = task_to_run;

    if (gp_current_task == NULL) {
        gp_current_task = p_tcb_old;        // revert back to the old task
        return RTX_ERR;
    }

    // at this point, gp_current_task != NULL and p_tcb_old != NULL
    if (gp_current_task != p_tcb_old) {
        gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb

        p_tcb_old->state = READY;
        if(p_tcb_old->prio > PRIO_RT_UB) {
            add_tsk_to_front_of_high(p_tcb_old);
        }
        /*if (p_tcb_old->state != DORMANT && p_tcb_old->state != BLK_RECV && p_tcb_old->state != BLK_SEND) {
            p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb
        }*/

        // p_tcb_old->msp = (U32*)__get_MSP();

        k_tsk_switch(p_tcb_old);            // switch kernel stacks 
    }

    return RTX_OK;
}

void first_check_real_time_tasks_sending_priority_queue_for_any_blocked_real_time_tasks_and_freeing_these_in_order_of_priority_then_if_there_is_still_room_remaining_then_start_checking_sending_queue_for_non_real_time_task_corresponding_to_tid_mailbox_then_if_there_are_tasks_waiting_to_send_proceed_to_let_them_finish_the_send_as_long_as_there_is_enough_space_remaining_in_the_mailbox_and_ensure_the_sender_immediately_gives_control_back_to_receiver_after_sending_completes_and_loop_until_there_is_no_longer_enough_space_in_the_mailbox(task_t tid_mailbox) {
    int available_space = k_mbx_get(tid_mailbox);

    while (rt_sending_priority_queue[tid_mailbox][0] != MAX_TASKS && (available_space - (int)g_tcbs[rt_sending_priority_queue[tid_mailbox][0]].msg_size) >= 0) {
        available_space -= (int)g_tcbs[rt_sending_priority_queue[tid_mailbox][0]].msg_size;
        tsk_run_new_finish_send(&g_tcbs[rt_sending_priority_queue[tid_mailbox][0]]);

        g_tcbs[rt_sending_priority_queue[tid_mailbox][0]].tid_send_dest = MAX_TASKS;
        g_tcbs[rt_sending_priority_queue[tid_mailbox][0]].msg_size = 0;

        for (int i = 0; i < rt_sending_priority_queue_task_count[tid_mailbox]; i++) {
            rt_sending_priority_queue[tid_mailbox][i] = rt_sending_priority_queue[tid_mailbox][i + 1];
        }
        rt_sending_priority_queue_task_count[tid_mailbox]--;
    }

    while(sending_priority_queue[tid_mailbox][sending_priority_queue_beginning[tid_mailbox]] != MAX_TASKS && (available_space - (int)g_tcbs[sending_priority_queue[tid_mailbox][sending_priority_queue_beginning[tid_mailbox]]].msg_size) >= 0) {
        available_space -= (int)g_tcbs[sending_priority_queue[tid_mailbox][sending_priority_queue_beginning[tid_mailbox]]].msg_size;
        tsk_run_new_finish_send(&g_tcbs[sending_priority_queue[tid_mailbox][sending_priority_queue_beginning[tid_mailbox]]]);

        g_tcbs[sending_priority_queue[tid_mailbox][sending_priority_queue_beginning[tid_mailbox]]].tid_send_dest = MAX_TASKS;
        g_tcbs[sending_priority_queue[tid_mailbox][sending_priority_queue_beginning[tid_mailbox]]].msg_size = 0;

        sending_priority_queue[tid_mailbox][sending_priority_queue_beginning[tid_mailbox]] = MAX_TASKS;
        sending_priority_queue_beginning[tid_mailbox] = (sending_priority_queue_beginning[tid_mailbox] + 1) % (MAX_TASKS - 1);
    }

    modify_priority_queue(0);   
}

void add_task_to_back_priority_queue(TCB *task) {
    TCB *temp_tcb = priority_queue[(U32)(task->prio - HIGH)];
	
    if (temp_tcb == NULL) {
        priority_queue[(U32)(task->prio - HIGH)] = task;
    } else {
        while(temp_tcb->next != NULL) {
            temp_tcb = temp_tcb->next;
        }

        temp_tcb->next = task;
        task->prev = temp_tcb;
    }
}

int modify_sending_mbx_queue(BOOL blocking, task_t receiver_tid, const void *buf, BOOL *blocked) {
    int current_mbx_index = sending_priority_queue_beginning[receiver_tid];

    if (sending_priority_queue[receiver_tid][current_mbx_index] == MAX_TASKS) {
        return RTX_OK;
    }

    if (blocking) {
        *blocked = 1;
        return block_sending_mbx_queue(receiver_tid, ((RTX_MSG_HDR *)buf)->length);
    } else {
        errno = ENOSPC;
        return RTX_ERR;
    }
}

int block_sending_mbx_queue(task_t receiver_tid, U32 msg_size) {
    int current_mbx_index = sending_priority_queue_beginning[receiver_tid];
    gp_current_task->state = BLK_SEND;

    g_tcbs[gp_current_task->tid].tid_send_dest = receiver_tid;
    g_tcbs[gp_current_task->tid].msg_size = msg_size;

    if (gp_current_task->prio <= PRIO_RT_UB) { // rt tasks
        rt_sending_priority_queue[receiver_tid][rt_sending_priority_queue_task_count[receiver_tid]] = gp_current_task->tid;
        rt_sending_priority_queue_task_count[receiver_tid]++;
        for (int i = rt_sending_priority_queue_task_count[receiver_tid] - 1; i > 0; i--) {
            if (g_tcbs[rt_sending_priority_queue[receiver_tid][i]].prio < g_tcbs[rt_sending_priority_queue[receiver_tid][i - 1]].prio) {
                task_t temp = rt_sending_priority_queue[receiver_tid][i];
                rt_sending_priority_queue[receiver_tid][i] = rt_sending_priority_queue[receiver_tid][i - 1];
                rt_sending_priority_queue[receiver_tid][i - 1] = temp;
            } else {
                break;
            }
        }
        sort_rt_tsk();
    } else { // not rt tasks
        while(sending_priority_queue[receiver_tid][current_mbx_index] != MAX_TASKS) {
            current_mbx_index = (current_mbx_index + 1) % (MAX_TASKS);
        }

        sending_priority_queue[receiver_tid][current_mbx_index] = gp_current_task->tid;
    }

    int res = k_tsk_run_new();
    if(res == RTX_OK && gp_current_task->tid_send_dest == MAX_TASKS) {
        res = MAX_TASKS;
    }
    return res;
}

int block_recv_task(task_t tid) {
    if (tid >= MAX_TASKS) {
        return RTX_ERR; // We should not be able to block a task with invalid tid
    } else if (g_tcbs[tid].state == BLK_RECV) {
        return RTX_ERR; // We should not be able to block a task that is in BLK_RECV state
    }
    // blk_recieving_array[tid] = TRUE;
    g_tcbs[tid].state = BLK_RECV;
    sort_rt_tsk();

    return k_tsk_run_new();
}

int unblock_recv_task(task_t tid, BOOL preempt) {
    if (tid >= MAX_TASKS) {
        return RTX_ERR; // We should not be able to unblock a task with invalid tid
    } else if (g_tcbs[tid].state == READY) {
        return RTX_OK;
    } else if (g_tcbs[tid].state != BLK_RECV) {
        return RTX_ERR; // We should not be able to unblock a task that is neither a task with state READY OR BLK_RECV
    }

    g_tcbs[tid].state = READY;
    add_task_to_back_priority_queue(&g_tcbs[tid]);

    if (preempt) {
        return modify_priority_queue(0);
    } else {
        return RTX_OK;
    }
}

int modify_priority_queue(int voluntary) {
    BOOL is_current_highest = TRUE;
    if(gp_current_task->prio > PRIO_RT_UB) {
        if( !(num_of_rt_tasks > 0 && g_tcbs[rt_priority_queue[0].tid].state == READY) ) {
            if (gp_current_task->prio != PRIO_NULL) {
                for(int i = 0; i <= gp_current_task->prio - HIGH; i++) {
                    if (i == gp_current_task->prio - HIGH) {
                        if (voluntary) {
                            is_current_highest = FALSE;
                        }
                    } else {
                        if (priority_queue[i] != NULL) {
                            is_current_highest = FALSE;

                            break;
                        }
                    }
                }
            } else {
                for(int i = 0; i < 4; i++) {
                    if (priority_queue[i] != NULL) {
                        is_current_highest = FALSE;

                        break;
                    }
                }
            }

            if (is_current_highest) {
                return RTX_OK;
            }
        }

        if (gp_current_task->prio == PRIO_NULL) {
            priority_queue[4] = gp_current_task;
        } else {
            if (priority_queue[(U32)(gp_current_task->prio - HIGH)] == NULL) {
                priority_queue[(U32)(gp_current_task->prio - HIGH)] = gp_current_task;
            } else {
                if (voluntary) {
                    add_task_to_back_priority_queue(gp_current_task);
                } else {
                    gp_current_task->next = priority_queue[(U32)(gp_current_task->prio - HIGH)];
                    priority_queue[(U32)(gp_current_task->prio - HIGH)]->prev = gp_current_task;
                    priority_queue[(U32)(gp_current_task->prio - HIGH)] = gp_current_task;
                }
            }
        }
    }
    
    if (num_of_rt_tasks > 0 && (g_tcbs[rt_priority_queue[0].tid].state == READY || g_tcbs[rt_priority_queue[0].tid].state == RUNNING) ) {
        if (rt_priority_queue[0].tid == gp_current_task->tid) {
            return RTX_OK;
        } else {
            TCB *old = gp_current_task;
            gp_current_task->state = READY;
            g_tcbs[rt_priority_queue[0].tid].state = RUNNING;
            gp_current_task = &g_tcbs[rt_priority_queue[0].tid];
            k_tsk_switch(old);
            return RTX_OK;
        }
    }

    k_tsk_run_new();

    return RTX_OK; 
}

/**
 * @brief initialzie the first task in the system
 */
void k_tsk_init_first(TASK_INIT *p_task)
{
    p_task->prio         = PRIO_NULL;
    p_task->priv         = 0;
    p_task->tid          = TID_NULL;
    p_task->ptask        = &task_null;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

/**
 * @brief initialize the kcd task in the system
 */
void k_tsk_init_kcd(TASK_INIT *p_task)
{
    p_task->prio         = HIGH;
    p_task->priv         = 0;
    p_task->tid          = TID_KCD;
    p_task->ptask        = &task_kcd;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

/**
 * @brief initialize the cdisp task in the system
 */
void k_tsk_init_cdisp(TASK_INIT *p_task)
{
    p_task->prio         = HIGH;
    p_task->priv         = 1;
    p_task->tid          = TID_CON;
    p_task->ptask        = &task_cdisp;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

/**
 * @brief initialize the wall clock task in the system
 */
void k_tsk_init_wall_clock(TASK_INIT *p_task)
{
    p_task->prio         = HIGH;
    p_task->priv         = 0;
    p_task->tid          = TID_WCLCK;
    p_task->ptask        = &task_wall_clock;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

/**************************************************************************//**
 * @brief       initialize all boot-time tasks in the system,
 *
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       task_info   boot-time task information structure pointer
 * @param       num_tasks   boot-time number of tasks
 * @pre         memory has been properly initialized
 * @post        none
 * @see         k_tsk_create_first
 * @see         k_tsk_create_new
 *****************************************************************************/

int k_tsk_init(TASK_INIT *task, int num_tasks)
{
    if (num_tasks > MAX_TASKS - 3) {
        return RTX_ERR;
    }
    
    TASK_INIT null_taskinfo;
    TASK_INIT kcd_taskinfo;
    TASK_INIT cdisp_taskinfo;
    TASK_INIT wall_clock_taskinfo;
    
    k_tsk_init_first(&null_taskinfo);
    if ( k_tsk_create_new(&null_taskinfo, &g_tcbs[TID_NULL], TID_NULL) == RTX_OK ) {
        gp_current_task = &g_tcbs[TID_NULL];
    } else {
        return RTX_ERR;
    }

    k_tsk_init_kcd(&kcd_taskinfo);
    if ( k_tsk_create_new(&kcd_taskinfo, &g_tcbs[TID_KCD], TID_KCD) == RTX_OK ) {
    } else {
        return RTX_ERR;
    }

    k_tsk_init_cdisp(&cdisp_taskinfo);
    if ( k_tsk_create_new(&cdisp_taskinfo, &g_tcbs[TID_CON], TID_CON) == RTX_OK ) {
    } else {
        return RTX_ERR;
    }

    k_tsk_init_wall_clock(&wall_clock_taskinfo);
    if ( k_tsk_create_new(&wall_clock_taskinfo, &g_tcbs[TID_WCLCK], TID_WCLCK) == RTX_OK ) {
    } else {
        return RTX_ERR;
    }
    
    // create the rest of the tasks
    for ( int i = 0; i < num_tasks; i++ ) {
        TCB *p_tcb = &g_tcbs[i+1];
        if (k_tsk_create_new(&task[i], p_tcb, i+1) != RTX_OK) {
            return RTX_ERR;
        }
    }

    for ( int i = num_tasks + 1; i < MAX_TASKS - 3; i++ ) {
        TCB *p_tcb = &g_tcbs[i];
        p_tcb->state = DORMANT;
        p_tcb->prio = 0;
        p_tcb->priv = 0;
        p_tcb->msp = NULL;
        p_tcb->msp_base = NULL;
        p_tcb->psp_base = NULL;
        p_tcb->next = NULL;
        p_tcb->prev = NULL;
        p_tcb->ptask = NULL;
        p_tcb->tid = i;
        p_tcb->u_stack_size = 0;
        p_tcb->tid_send_dest = MAX_TASKS;
        p_tcb->msg_size = 0;
        p_tcb->p_n.sec = 0;
        p_tcb->p_n.usec = 0;
    }

    for(int i = 0; i < MAX_TASKS; i++) {
        sending_priority_queue_beginning[i] = 0;
        rt_sending_priority_queue_task_count[i] = 0;
        for(int j = 0; j < MAX_TASKS - 1; j++) {
            sending_priority_queue[i][j] = MAX_TASKS;
            rt_sending_priority_queue[i][j] = MAX_TASKS;
        }
    }
    
    return RTX_OK;
}
/**************************************************************************//**
 * @brief       initialize a new task in the system,
 *              one dummy kernel stack frame, one dummy user stack frame
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       p_taskinfo  task initialization structure pointer
 * @param       p_tcb       the tcb the task is assigned to
 * @param       tid         the tid the task is assigned to
 *
 * @details     From bottom of the stack,
 *              we have user initial context (xPSR, PC, SP_USR, uR0-uR3)
 *              then we stack up the kernel initial context (kLR, kR4-kR12, PSP, CONTROL)
 *              The PC is the entry point of the user task
 *              The kLR is set to SVC_RESTORE
 *              20 registers in total
 * @note        YOU NEED TO MODIFY THIS FILE!!!
 *****************************************************************************/
int k_tsk_create_new(TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid)
{
    extern U32 SVC_RTE;

    U32 *usp;
    U32 *ksp;

    if (p_taskinfo == NULL || p_tcb == NULL)
    {
        return RTX_ERR;
    }

    p_tcb->tid   = tid;
    p_tcb->state = READY;
    p_tcb->prio  = p_taskinfo->prio;
    p_tcb->priv  = p_taskinfo->priv;
    p_tcb->ptask = p_taskinfo->ptask;
    p_tcb->tid_send_dest = MAX_TASKS;
    p_tcb->msg_size = 0;
    p_tcb->p_n.sec = 0;
    p_tcb->p_n.usec = 0;

    /*---------------------------------------------------------------
     *  Step1: allocate user stack for the task
     *         stacks grows down, stack base is at the high address
     * ATTENTION: you need to modify the following three lines of code
     *            so that you use your own dynamic memory allocator
     *            to allocate variable size user stack.
     * -------------------------------------------------------------*/
    
    if (tid == TID_NULL) {
        usp = (U32*)__get_PSP();

        p_tcb->u_stack_size = p_taskinfo->u_stack_size;
    } else {
        size_t alloc_size;

        if( p_taskinfo->u_stack_size < PROC_STACK_SIZE ) {
            alloc_size = (size_t)PROC_STACK_SIZE;
        } else {
            alloc_size = (size_t)p_taskinfo->u_stack_size;
        }

        p_tcb->u_stack_size = (U32)alloc_size;


        usp = (U32*)k_mpool_alloc(MPID_IRAM2, alloc_size);

        if (usp == NULL) {
            return RTX_ERR;
        }

        usp += (alloc_size / 4) - 1;
    }

    p_tcb->psp_base = usp;

    /*-------------------------------------------------------------------
     *  Step2: create task's thread mode initial context on the user stack.
     *         fabricate the stack so that the stack looks like that
     *         task executed and entered kernel from the SVC handler
     *         hence had the exception stack frame saved on the user stack.
     *         This fabrication allows the task to return
     *         to SVC_Handler before its execution.
     *
     *         8 registers listed in push order
     *         <xPSR, PC, uLR, uR12, uR3, uR2, uR1, uR0>
     * -------------------------------------------------------------*/

    // if kernel task runs under SVC mode, then no need to create user context stack frame for SVC handler entering
    // since we never enter from SVC handler in this case
    
    *(--usp) = INITIAL_xPSR;             // xPSR: Initial Processor State
    *(--usp) = (U32) (p_taskinfo->ptask);// PC: task entry point
        
    // uR14(LR), uR12, uR3, uR3, uR1, uR0, 6 registers
    for ( int j = 0; j < 6; j++ ) {
        
#ifdef DEBUG_0
        *(--usp) = 0xDEADAAA0 + j;
#else
        *(--usp) = 0x0;
#endif
    }
    
    // allocate kernel stack for the task
    ksp = (U32 *)k_mpool_alloc(MPID_IRAM2, KERN_STACK_SIZE);

    if ( ksp == NULL ) {
        return RTX_ERR;
    }

    ksp += (KERN_STACK_SIZE) / 4 - 1;

    p_tcb->msp_base = ksp;

    /*---------------------------------------------------------------
     *  Step3: create task kernel initial context on kernel stack
     *
     *         12 registers listed in push order
     *         <kLR, kR4-kR12, PSP, CONTROL>
     * -------------------------------------------------------------*/
    // a task never run before directly exit
    *(--ksp) = (U32) (&SVC_RTE);
    // kernel stack R4 - R12, 9 registers
    #define NUM_REGS 9
    for ( int j = 0; j < NUM_REGS; j++) {        
        *(--ksp) = 0x0;
    }
        
    // put user sp on to the kernel stack
    *(--ksp) = (U32) usp;
    
    // save control register so that we return with correct access level
    if (p_taskinfo->priv == 1) {  // privileged 
        *(--ksp) = __get_CONTROL() & ~BIT(0); 
    } else {                      // unprivileged
        *(--ksp) = __get_CONTROL() | BIT(0);
    }

    p_tcb->msp = ksp;

    if (p_tcb->prio == PRIO_NULL) {
        priority_queue[4] = p_tcb;
        p_tcb->prev = NULL;
        p_tcb->next = NULL;
    } else {
        TCB *temp_tcb = priority_queue[(U32)(p_tcb->prio - HIGH)];

        if (temp_tcb == NULL) {
            priority_queue[p_tcb->prio - HIGH] = p_tcb;
            p_tcb->prev = NULL;
            p_tcb->next = NULL;
        } else {
            while(temp_tcb->next != NULL) {
                temp_tcb = temp_tcb->next;
            }

            temp_tcb->next = p_tcb;
            p_tcb->prev = temp_tcb;
            p_tcb->next = NULL;
        }
    }

    return RTX_OK;
}

/**************************************************************************//**
 * @brief       switching kernel stacks of two TCBs
 * @param       p_tcb_old, the old tcb that was in RUNNING
 * @return      RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre         gp_current_task is pointing to a valid TCB
 *              gp_current_task->state = RUNNING
 *              gp_crrent_task != p_tcb_old
 *              p_tcb_old == NULL or p_tcb_old->state updated
 * @note        caller must ensure the pre-conditions are met before calling.
 *              the function does not check the pre-condition!
 * @note        The control register setting will be done by the caller
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
__asm void k_tsk_switch(TCB *p_tcb_old)
{
        PRESERVE8
        EXPORT  K_RESTORE
        
        PUSH    {R4-R12, LR}                // save general pupose registers and return address
        MRS     R4, CONTROL                 
        MRS     R5, PSP
        PUSH    {R4-R5}                     // save CONTROL, PSP
        STR     SP, [R0, #TCB_MSP_OFFSET]   // save SP to p_old_tcb->msp
K_RESTORE
        LDR     R1, =__cpp(&gp_current_task)
        LDR     R2, [R1]
        LDR     SP, [R2, #TCB_MSP_OFFSET]   // restore msp of the gp_current_task
        POP     {R4-R5}
        MSR     PSP, R5                     // restore PSP
        MSR     CONTROL, R4                 // restore CONTROL
        ISB                                 // flush pipeline, not needed for CM3 (architectural recommendation)
        POP     {R4-R12, PC}                // restore general purpose registers and return address
}

__asm void k_tsk_start(void)
{
        PRESERVE8
        B K_RESTORE
}

/**************************************************************************//**
 * @brief       run a new thread. The caller becomes READY and
 *              the scheduler picks the next ready to run task.
 * @return      RTX_ERR on error and zero on success
 * @pre         gp_current_task != NULL && gp_current_task == RUNNING
 * @post        gp_current_task gets updated to next to run task
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *****************************************************************************/
int k_tsk_run_new(void)
{
    TCB *p_tcb_old = NULL;
    
    if (gp_current_task == NULL) {
        return RTX_ERR;
    }

    p_tcb_old = gp_current_task;
    gp_current_task = scheduler();

    if (gp_current_task == NULL) {
        gp_current_task = p_tcb_old;        // revert back to the old task
        return RTX_ERR;
    }

    // at this point, gp_current_task != NULL and p_tcb_old != NULL
    if (gp_current_task != p_tcb_old) {
        gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb

        if (p_tcb_old->state != DORMANT && p_tcb_old->state != SUSPENDED && p_tcb_old->state != BLK_RECV && p_tcb_old->state != BLK_SEND) {
            p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb
        }

        // p_tcb_old->msp = (U32*)__get_MSP();

        k_tsk_switch(p_tcb_old);            // switch kernel stacks 
    }

    return RTX_OK;
}

 
/**************************************************************************//**
 * @brief       yield the cpu
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task != NULL &&
 *              gp_current_task->state = RUNNING
 * @post        gp_current_task gets updated to next to run task
 * @note:       caller must ensure the pre-conditions before calling.
 *****************************************************************************/
int k_tsk_yield(void)
{
    return modify_priority_queue(1);
}

/**
 * @brief   get task identification
 * @return  the task ID (TID) of the calling task
 */
task_t k_tsk_gettid(void)
{
    task_t task_id = gp_current_task == &g_tcbs[0] || is_uart ? TID_UART : gp_current_task->tid;
    is_uart = FALSE;
    return task_id;
}

/*
 *===========================================================================
 *                             TO BE IMPLEMETED IN LAB2
 *===========================================================================
 */

int k_tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size)
{
#ifdef DEBUG_0
    printf("k_tsk_create: entering...\n\r");
    printf("task = 0x%x, task_entry = 0x%x, prio=%d, stack_size = %d\n\r", task, task_entry, prio, stack_size);
#endif /* DEBUG_0 */

    // if( stack_size <= 0){
    //     return ENOMEM;
    // }

    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tcbs[i].state == DORMANT) {
            break;
        }
        if (i == MAX_TASKS - 1) {
            errno = EAGAIN;
            return -1;
        }
    }

    TASK_INIT t_init;
    t_init.u_stack_size = stack_size;
    t_init.prio = prio;
    t_init.priv = 0;
    t_init.ptask = task_entry;

    if(prio > LOWEST || prio < HIGH){
        errno = EINVAL;
        return -1;
    }

    for(int i = 1; i < MAX_TASKS; i++) {
        TCB *temp_tcb = &g_tcbs[i];

        if (temp_tcb->state == DORMANT) {
            *task = i;
            int res = k_tsk_create_new(&t_init, temp_tcb, i);

            if (res == RTX_ERR) {
                errno = ENOMEM;
                return -1;
            }

            break;
        }
    }

    modify_priority_queue(0);

    return RTX_OK;
}

void k_tsk_exit(void) 
{
#ifdef DEBUG_0
    printf("k_tsk_exit: entering...\n\r");
#endif /* DEBUG_0 */
    if(k_mbx_get(gp_current_task->tid) != RTX_ERR) {
        S32 index_one = sending_priority_queue_beginning[gp_current_task->tid];
        while(sending_priority_queue[gp_current_task->tid][index_one] != MAX_TASKS) {
            task_t sending_tid = sending_priority_queue[gp_current_task->tid][index_one];
            g_tcbs[sending_tid].state = READY;
            g_tcbs[sending_tid].tid_send_dest = MAX_TASKS;
            g_tcbs[sending_tid].msg_size = 0;
            add_task_to_back_priority_queue(&g_tcbs[sending_tid]);

            sending_priority_queue[gp_current_task->tid][index_one] = MAX_TASKS;
            index_one++;
        }
        sending_priority_queue_beginning[gp_current_task->tid] = 0;
        cleanup_mbx(gp_current_task->tid);
    }

    if(gp_current_task->prio <= PRIO_RT_UB) {
        rt_priority_queue[0].p_n = rt_priority_queue[num_of_rt_tasks - 1].p_n;
        rt_priority_queue[0].rt_mbx_size = rt_priority_queue[num_of_rt_tasks - 1].rt_mbx_size;
        rt_priority_queue[0].ticks_to_deadline = rt_priority_queue[num_of_rt_tasks - 1].ticks_to_deadline;
        rt_priority_queue[0].tid = rt_priority_queue[num_of_rt_tasks - 1].tid;
        num_of_rt_tasks--;
        sort_rt_tsk();
    }

    gp_current_task->state = DORMANT;
    k_mpool_dealloc(MPID_IRAM2, gp_current_task->psp_base - (gp_current_task->u_stack_size / 4 - 1));
    k_mpool_dealloc(MPID_IRAM2, gp_current_task->msp_base - ((KERN_STACK_SIZE >> 2) / 4 - 1));
    k_tsk_run_new();

    return;
}

int k_tsk_set_prio(task_t task_id, U8 prio) 
{
#ifdef DEBUG_0
    printf("k_tsk_set_prio: entering...\n\r");
    printf("task_id = %d, prio = %d.\n\r", task_id, prio);
#endif /* DEBUG_0 */

    if( prio < HIGH || prio > LOWEST || task_id == TID_NULL) {
        return EINVAL;
    }

    RTX_TASK_INFO ct_info;
    RTX_TASK_INFO st_info;
    k_tsk_get(gp_current_task->tid, &ct_info);
    int current_tcb_index = 0;

    for(size_t i = 0; i < MAX_TASKS; i++) {
        if(g_tcbs[i].tid == task_id) {
            k_tsk_get(task_id, &st_info);
            current_tcb_index = i;
            break;
        } else if (i == MAX_TASKS - 1) {
            return EINVAL;
        }
    }

    if (st_info.state == DORMANT) {
        return RTX_OK;
    } else if (ct_info.priv < st_info.priv) {
        return EPERM;
    } else if (st_info.prio == prio) {
        return RTX_OK;
    } else if (st_info.state == BLK_SEND) {
        int max_sending_tasks = MAX_TASKS - 1;
        int recv_mbx_index = st_info.tid_send_dest;
        int starting_task_index = sending_priority_queue_beginning[recv_mbx_index];
        task_t tid_task_to_change = sending_priority_queue[recv_mbx_index][starting_task_index];
        BOOL arrived_at_task_to_change = 0;
        task_t old_tid;

        for(int i = 0; i < max_sending_tasks; i++) {
            int current_index = (i + starting_task_index) % max_sending_tasks;

            if (prio > st_info.prio) {
                if (g_tcbs[sending_priority_queue[recv_mbx_index][current_index]].prio < prio) {
                    old_tid = st_info.tid;
                    arrived_at_task_to_change = 1;
                }

                if (arrived_at_task_to_change) {
                    int temp = sending_priority_queue[recv_mbx_index][current_index];
                    sending_priority_queue[recv_mbx_index][current_index] = old_tid;
                    old_tid = temp;
                }

                if (tid_task_to_change == st_info.tid) {
                    break;
                }
            } else {
                if (tid_task_to_change == st_info.tid) {
                    arrived_at_task_to_change = 1;
                }

                if (g_tcbs[sending_priority_queue[recv_mbx_index][current_index + 1]].prio < prio) {
                    sending_priority_queue[recv_mbx_index][current_index] = st_info.tid;

                    break;
                }

                if (arrived_at_task_to_change) {
                    sending_priority_queue[recv_mbx_index][current_index] = sending_priority_queue[recv_mbx_index][current_index + 1];
                }
            }
        }
    } else if (st_info.state == BLK_RECV) {
        g_tcbs[st_info.tid].prio = prio;
    } else {
        TCB *temp = priority_queue[(U32)(st_info.prio - HIGH)];

        while(temp != NULL) {
            if(temp->tid == task_id) {
                TCB *next = temp->next;
                TCB *prev = temp->prev;
                if(next != NULL) {
                    next->prev = prev;
                }
                if(prev != NULL) {
                    prev->next = next;
                } else {
                    priority_queue[(U32)(st_info.prio - HIGH)] = next;
                }
                temp->next = NULL;
                temp->prev = NULL;
                break;
            } else {
                temp = temp->next;
            }
        }

        g_tcbs[current_tcb_index].prio = prio;

        TCB *new_temp = priority_queue[(U32)(prio - HIGH)];

        if (current_tcb_index != gp_current_task->tid) {
            if (new_temp == NULL) {
                g_tcbs[current_tcb_index].prev = NULL;
                g_tcbs[current_tcb_index].next = NULL;
                priority_queue[(U32)(prio - HIGH)] = &g_tcbs[current_tcb_index];
            } else {
                while(new_temp->next != NULL) {
                    new_temp = new_temp->next;
                }
                new_temp->next = &g_tcbs[current_tcb_index];
                g_tcbs[current_tcb_index].prev = new_temp;
                g_tcbs[current_tcb_index].next = NULL;
            }
        }
        
        if (current_tcb_index == gp_current_task->tid) {
            modify_priority_queue(1);
        } else {
            modify_priority_queue(0);
        }
    }

    return RTX_OK;    
}

/**
 * @brief   Retrieve task internal information 
 * @note    this is a dummy implementation, you need to change the code
 */
int k_tsk_get(task_t tid, RTX_TASK_INFO *buffer)
{
#ifdef DEBUG_0
    printf("k_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    

    //buffer is a null pointer
    if (buffer == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    
    /* The code fills the buffer with some fake task information. 
       You should fill the buffer with correct information    */

    if (tid == gp_current_task->tid) {
        buffer->tid           = tid;
        buffer->prio          = gp_current_task->prio;
        buffer->u_stack_size  = gp_current_task->u_stack_size;
        buffer->priv          = gp_current_task->priv;
        buffer->ptask         = gp_current_task->ptask;
        buffer->k_sp          = (U32)__get_MSP();
        buffer->k_sp_base     = (U32)gp_current_task->msp_base;
        buffer->k_stack_size  = KERN_STACK_SIZE;
        buffer->state         = gp_current_task->state;
        buffer->u_sp          = (U32)__get_PSP();
        buffer->u_sp_base     = (U32)gp_current_task->psp_base;
        buffer->tid_send_dest = gp_current_task->tid_send_dest;
        buffer->msg_size      = gp_current_task->msg_size;
        return RTX_OK; 
    }

    U32 index;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tid == g_tcbs[i].tid) {
            index = i;
            break;
        }
        if (i == 9) { // No matching tid found
            errno = EINVAL;
            return RTX_ERR;
        }
    }
    
    buffer->tid           = tid;
    buffer->prio          = g_tcbs[index].prio;
    buffer->u_stack_size  = g_tcbs[index].u_stack_size;
    buffer->priv          = g_tcbs[index].priv;
    buffer->ptask         = g_tcbs[index].ptask;
    buffer->k_sp          = (U32)g_tcbs[index].msp;
    buffer->k_sp_base     = (U32)g_tcbs[index].msp_base;
    buffer->k_stack_size  = KERN_STACK_SIZE;
    buffer->state         = g_tcbs[index].state;
    buffer->u_sp          = *(g_tcbs[index].msp_base - 10);
    buffer->u_sp_base     = (U32)g_tcbs[index].psp_base;
    buffer->tid_send_dest = g_tcbs[index].tid_send_dest;
    buffer->msg_size      = g_tcbs[index].msg_size;
    return RTX_OK;
}

int k_tsk_ls(task_t *buf, size_t count){
#ifdef DEBUG_0
    printf("k_tsk_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */

    if(buf == NULL || count == 0) {
        return EFAULT;
    }

    U32 tid_counter = 0;
    U32 index_counter = 0;
    while( tid_counter < MAX_TASKS && index_counter < count) {
        if (g_tcbs[tid_counter].state != DORMANT) {
            buf[index_counter] = g_tcbs[tid_counter].tid;
            index_counter++;
        }
        tid_counter++;
    }

    return index_counter;
}

int k_rt_tsk_set(TIMEVAL *p_tv)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_set: p_tv = 0x%x\r\n", p_tv);
#endif /* DEBUG_0 */

    if (gp_current_task->prio <= PRIO_RT_UB) {
        errno = EPERM;
        return RTX_ERR;
    } else if ((p_tv->usec == 0 && p_tv->sec == 0) || p_tv->usec % (RTX_TICK_SIZE * MIN_PERIOD) != 0) {
        errno = EINVAL;
        return RTX_ERR;
    }

    //temp
    //rt_priority_queue[num_of_rt_tasks].p_n.sec = 2 * (p_tv->sec * 1000000 + p_tv->usec) / 1000000;
    //rt_priority_queue[num_of_rt_tasks].p_n.usec = 2 * (p_tv->sec * 1000000 + p_tv->usec) % 1000000;
    rt_priority_queue[num_of_rt_tasks].p_n = *p_tv;
    rt_priority_queue[num_of_rt_tasks].ticks_to_deadline = (p_tv->sec * 1000000 + p_tv->usec)/RTX_TICK_SIZE;
    rt_priority_queue[num_of_rt_tasks].tid = gp_current_task->tid;

    num_of_rt_tasks++;
    sort_rt_tsk();
    
    return RTX_OK;
}

int k_rt_tsk_susp(void)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_susp: entering\r\n");
#endif /* DEBUG_0 */

    if (gp_current_task->prio >= HIGH) {
        errno = EPERM;
        return RTX_ERR;
    }

    gp_current_task->state = SUSPENDED;
    sort_rt_tsk();

    if(g_tcbs[rt_priority_queue[0].tid].state == READY) {
        TCB *temp = gp_current_task;
        gp_current_task = &g_tcbs[rt_priority_queue[0].tid];
        gp_current_task->state = RUNNING;

        k_tsk_switch(temp);
    } else {
        k_tsk_run_new();
    }

    return RTX_OK;
}

int k_rt_tsk_get(task_t tid, TIMEVAL *buffer)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */

    if (buffer == NULL) {
        return RTX_ERR;
    } else if (gp_current_task->prio >= HIGH) {
        errno = EPERM;
        return RTX_ERR;
    } else if (g_tcbs[tid].prio >= HIGH) {
        errno = EINVAL;
        return RTX_ERR;
    }

    *buffer = g_tcbs[tid].p_n;

    /* The code fills the buffer with some fake rt task information. 
       You should fill the buffer with correct information    */
    // buffer->sec  = 0xABCD;
    // buffer->usec = 0xEEFF;
    
    return RTX_OK;
}

// insertion sort?
void sort_rt_tsk(void) {

    for (int i = 1; i < num_of_rt_tasks; i++) {
        TASK_RT key = rt_priority_queue[i];
        int j = i - 1;

        if(g_tcbs[key.tid].state == READY || g_tcbs[key.tid].state == RUNNING) {
            while( ( (g_tcbs[rt_priority_queue[j].tid].state != READY && g_tcbs[rt_priority_queue[j].tid].state != RUNNING) || key.ticks_to_deadline < rt_priority_queue[j].ticks_to_deadline ) && j >= 0) {
                rt_priority_queue[j + 1] = rt_priority_queue[j];
                j--;
            }
            rt_priority_queue[j + 1] = key;
        }
    }

    for (int i = 0; i < num_of_rt_tasks; i++) {
        g_tcbs[rt_priority_queue[i].tid].prio = i;
    }
}

void sort_time_increment(uint32_t ticks) {
    for(int i = 0; i < num_of_rt_tasks; i++) {
        int max_ticks = (rt_priority_queue[i].p_n.sec * 1000000 + rt_priority_queue[i].p_n.usec)/RTX_TICK_SIZE;

        if(g_tcbs[rt_priority_queue[i].tid].state == SUSPENDED && rt_priority_queue[i].ticks_to_deadline <= ticks) {
            g_tcbs[rt_priority_queue[i].tid].state = READY;
        }
        rt_priority_queue[i].ticks_to_deadline = (rt_priority_queue[i].ticks_to_deadline + max_ticks - ticks) % max_ticks;
        if(rt_priority_queue[i].ticks_to_deadline == 0) {
            rt_priority_queue[i].ticks_to_deadline = max_ticks;
        }
    }
    sort_rt_tsk();

    modify_priority_queue(0);
}

void k_set_uart(void) {
    is_uart = TRUE;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

