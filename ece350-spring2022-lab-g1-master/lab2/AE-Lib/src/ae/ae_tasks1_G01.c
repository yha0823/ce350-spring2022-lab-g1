/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2021 Yiqing Huang
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
 * @file        ae_tasks200.c
 * @brief       P2 public testing suite 200
 *
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 Jun
 * *
 *****************************************************************************/

#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */
    
#define NUM_TESTS       3       // number of tests
#define NUM_INIT_TASKS  2       // number of tasks during initialization
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];

AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

int count = 0;

void task0_sub0(void);
void task0_sub1(void);
void task0_sub2(void);
void task0_sub3(void);
void task0_sub4(void);
void task0_sub5(void);

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    // for (int i = 0; i < num; i++ ) {                                                 
    //     tasks[i].u_stack_size = PROC_STACK_SIZE;    
    //     tasks[i].prio = HIGH + i;
    //     tasks[i].priv = 1;
    // }


    tasks[0].priv  = 1;
    tasks[0].ptask = &task0;
    tasks[0].u_stack_size = PROC_STACK_SIZE;
    tasks[0].prio = MEDIUM;

    printf("G001-TS1: START\r\n");

    // tasks[1].priv  = 0;
    // tasks[1].ptask = &task1;
    // tasks[1].u_stack_size = PROC_STACK_SIZE;    
    // tasks[1].prio = MEDIUM;
    // tasks[2].priv  = 0;
    // tasks[2].ptask = &task2;
    // tasks[2].u_stack_size = PROC_STACK_SIZE;    
    // tasks[2].prio = MEDIUM;
}

void task0(void) { // MEDIUM
    task_t tid[6] = {0,0,0,0,0,0};
    int result[6] = {0,0,0,0,0,0};

    result[0] = tsk_create(&(tid[0]), &task0_sub0, HIGH, PROC_STACK_SIZE); // O
    result[1] = tsk_create(&(tid[1]), &task0_sub1, MEDIUM, PROC_STACK_SIZE); // O
    printf("hehehe\r\n");
    result[2] = tsk_create(&(tid[2]), &task0_sub2, PRIO_NULL, PROC_STACK_SIZE); // X

    tsk_yield(); // should run task0_sub1 (next medium prio task)
    result[3] = tsk_create(&(tid[3]), &task0_sub3, HIGH, PROC_STACK_SIZE); // O
    result[4] = tsk_create(&(tid[4]), &task0_sub4, LOWEST, PROC_STACK_SIZE); // O
    result[5] = tsk_create(&(tid[5]), &task0_sub5, LOW, PROC_STACK_SIZE); // O

    if (result[0] == RTX_OK && result[1] == RTX_OK && result[2] != RTX_OK && result[3] == RTX_OK && result[4] == RTX_OK && result[5] == RTX_OK) {
        printf("YAY\r\n");
    }
    tsk_exit();
}

void task0_sub0(void) {
    for(int i = 0; i < 3; i++) {
        printf("aaaa\r\n");
    }
    count++;
    tsk_exit();
}
void task0_sub1(void) {
    for(int i = 0; i < 3; i++) {
        printf("bbbb\r\n");
    }
    count++;
    tsk_exit();
}
void task0_sub2(void) {
    printf("if you see me, you messed up bad...\r\n");
    count--;
    tsk_exit();
}
void task0_sub3(void) {
    printf("there there...\r\n");
    count++;
    tsk_exit();
}
void task0_sub4(void) {
    printf("I am LOWEST\r\n");
    count++;
    printf("G001-TS1: %d/5 tests PASSED\r\n", count);
    printf("G001-TS1: %d/5 tests FAILED\r\n", 5 - count);
    printf("G001-TS1: END\r\n");
    tsk_exit();
}
void task0_sub5(void) {
    printf("I am LOW\r\n");
    count++;
    tsk_exit();
}
