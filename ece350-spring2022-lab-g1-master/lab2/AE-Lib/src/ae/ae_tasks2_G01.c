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
    
#define NUM_TESTS       1       // number of tests
#define NUM_INIT_TASKS  1       // number of tasks during initialization
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

int count0 = 0;
int count1 = 0;
int count2 = 0;

void task0_sub0(void);
void task0_sub1(void);
void task0_sub2(void);
void task0_sub3(void);
void task0_sub4(void);
void task0_sub5(void);
void task0_sub6(void);
void task0_sub_end(void);
void task1_sub0(void);
void task1_sub1(void);
void task1_sub2(void);
void task1_sub3(void);
void task1_sub4(void);
void task1_sub5(void);
void task1_sub_end(void);
void task2_sub0(void);
void task2_sub1(void);
void task2_sub2(void);
void task2_sub3(void);
void task2_sub4(void);
void task2_sub5(void);
void task2_sub_end(void);

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    // for (int i = 0; i < num; i++ ) {                                                 
    //     tasks[i].u_stack_size = PROC_STACK_SIZE;    
    //     tasks[i].prio = HIGH + i;
    //     tasks[i].priv = 1;
    // }

    tasks[0].priv  = 0;
    tasks[0].ptask = &task0;
    tasks[0].u_stack_size = PROC_STACK_SIZE;
    tasks[0].prio = HIGH;

    // tasks[1].priv  = 0;
    // tasks[1].ptask = &task1;
    // tasks[1].u_stack_size = PROC_STACK_SIZE;    
    // tasks[1].prio = HIGH;

    // tasks[2].priv  = 0;
    // tasks[2].ptask = &task2;
    // tasks[2].u_stack_size = PROC_STACK_SIZE;    
    // tasks[2].prio = HIGH;
    
    printf("G001-TS2: START\r\n");
}

void task0(void) { // MEDIUM
    task_t tid0[8] = {0,0,0,0,0,0,0,0};
    task_t tid1[8] = {0,0,0,0,0,0,0,0};
    task_t tid2[10] = {0,0,0,0,0,0,0,0,0,0};
    int result0[8] = {0,0,0,0,0,0,0,0};
    int result1[8] = {0,0,0,0,0,0,0,0};
    int result2[10] = {0,0,0,0,0,0,0,0,0,0};

    result0[0] = tsk_create(&(tid0[0]), &task0_sub6, LOWEST, PROC_STACK_SIZE); // 7
    result0[1] = tsk_create(&(tid0[1]), &task0_sub_end, LOWEST, PROC_STACK_SIZE); // 8
    result0[2] = tsk_create(&(tid0[2]), &task0_sub0, MEDIUM, PROC_STACK_SIZE); // 1
    result0[3] = tsk_create(&(tid0[3]), &task0_sub1, LOW, PROC_STACK_SIZE); // 3
    result0[4] = tsk_create(&(tid0[4]), &task0_sub2, LOW, PROC_STACK_SIZE); // 4
    result0[5] = tsk_create(&(tid0[5]), &task0_sub5, LOW, PROC_STACK_SIZE); // 5
    result0[6] = tsk_create(&(tid0[6]), &task0_sub4, LOW, PROC_STACK_SIZE); // 6
    result0[7] = tsk_create(&(tid0[7]), &task0_sub3, MEDIUM, PROC_STACK_SIZE); // 2

    for (int i = 0; i < 8; i++) {
        if (result0[i] == RTX_OK) {
            uart1_put_string("-");
        }
    }
    uart1_put_string("\r\n");
    tsk_exit();
}

// void task1(void) { // MEDIUM
//     task_t tid0[10] = {0,0,0,0,0,0,0,0,0,0};
//     task_t tid1[10] = {0,0,0,0,0,0,0,0,0,0};
//     task_t tid2[10] = {0,0,0,0,0,0,0,0,0,0};
//     int result0[10] = {0,0,0,0,0,0,0,0,0,0};
//     int result1[10] = {0,0,0,0,0,0,0,0,0,0};
//     int result2[10] = {0,0,0,0,0,0,0,0,0,0};

//     result[0] = tsk_create(&(tid[0]), &task0_sub0, HIGH, PROC_STACK_SIZE); // O
//     result[1] = tsk_create(&(tid[1]), &task0_sub1, MEDIUM, PROC_STACK_SIZE); // O
//     printf("hehehe\r\n");
//     result[2] = tsk_create(&(tid[2]), &task0_sub2, PRIO_NULL, PROC_STACK_SIZE); // X

//     tsk_yield(); // should run task0_sub1 (next medium prio task)
//     result[3] = tsk_create(&(tid[3]), &task0_sub3, HIGH, PROC_STACK_SIZE); // O
//     result[4] = tsk_create(&(tid[4]), &task0_sub4, LOWEST, PROC_STACK_SIZE); // O
//     result[5] = tsk_create(&(tid[5]), &task0_sub5, LOW, PROC_STACK_SIZE); // O

//     if (result[0] == RTX_OK && result[1] == RTX_OK && result[2] != RTX_OK && result[3] == RTX_OK && result[4] == RTX_OK && result[5] == RTX_OK) {
//         printf("YAY\r\n");
//     }
//     tsk_exit();
// }

// void task2(void) { // MEDIUM
//     task_t tid0[10] = {0,0,0,0,0,0,0,0,0,0};
//     task_t tid1[10] = {0,0,0,0,0,0,0,0,0,0};
//     task_t tid2[10] = {0,0,0,0,0,0,0,0,0,0};
//     int result0[10] = {0,0,0,0,0,0,0,0,0,0};
//     int result1[10] = {0,0,0,0,0,0,0,0,0,0};
//     int result2[10] = {0,0,0,0,0,0,0,0,0,0};

//     result0[0] = tsk_create(&(tid0[0]), &task0_sub3, LOWEST, PROC_STACK_SIZE); // O
//     result0[1] = tsk_create(&(tid0[1]), &task0_sub_end, LOWEST, PROC_STACK_SIZE); // O
//     result0[2] = tsk_create(&(tid0[2]), &task0_sub2, PRIO_NULL, PROC_STACK_SIZE); // X

//     tsk_yield(); // should run task0_sub1 (next medium prio task)
//     result[3] = tsk_create(&(tid[3]), &task0_sub3, HIGH, PROC_STACK_SIZE); // O
//     result[4] = tsk_create(&(tid[4]), &task0_sub4, LOWEST, PROC_STACK_SIZE); // O
//     result[5] = tsk_create(&(tid[5]), &task0_sub5, LOW, PROC_STACK_SIZE); // O

//     if (result[0] == RTX_OK && result[1] == RTX_OK && result[2] != RTX_OK && result[3] == RTX_OK && result[4] == RTX_OK && result[5] == RTX_OK) {
//         printf("YAY\r\n");
//     }
//     tsk_exit();
// }

void task0_sub0(void) {
    printf("Smooth like butter, like a criminal undercover\r\n"); //1
    count0++;
    tsk_yield();
    printf("Cool shade, stunner, yeah, I owe it all to my mother, uh\r\n"); //3
    count0++;
    tsk_exit();
}
void task0_sub1(void) {
    printf("Hot like summer, yeah, I'm making you sweat like that (break it down!)\r\n"); //4
    count0++;
    tsk_yield();
    printf("Ooh (do the boogie, like)\r\n"); //8
    count0++;
    tsk_exit();
}
void task0_sub2(void) {
    printf("Ooh, when I look in the mirror\r\n"); //5
    count0++;
    tsk_yield();
    printf("A side step, right-left, to my beat\r\n"); //9
    count0++;
    tsk_exit();
}
void task0_sub3(void) {
    printf("Gon' pop like trouble breaking into your heart like that (ooh)\r\n"); //2
    count0++;
    tsk_exit();
}
void task0_sub4(void) {
    printf("I got that superstar glow, so\r\n"); //7
    count0++;
    tsk_yield();
    printf("Know that I got that heat\r\n"); //11
    count0++;
    tsk_exit();
}
void task0_sub5(void) {
    printf("I'll melt your heart into two\r\n"); //6
    count0++;
    tsk_yield();
    printf("High like the moon, rock with me, baby\r\n"); //10
    count0++;
    tsk_exit();
}
void task0_sub6(void) {
    printf("Let me show you 'cause talk is cheap\r\n"); //12
    count0++;
    tsk_yield();
    printf("Get it, let it roll\r\n"); //14
    count0++;
    tsk_exit();
}
void task0_sub_end(void) {
    printf("Let me show you 'cause talk is cheap\r\n"); //12
    count0++;
    printf("Side step, right-left, to my beat\r\n"); //13
    count0++;
    printf("Get it, let it roll\r\n"); //14
    count0++;
    tsk_yield();
    printf("... Butter by BTS\r\n"); //15
    count0++;
    printf("G001-TS2: %d/17 tests PASSED\r\n", count0);
    printf("G001-TS2: %d/17 tests FAILED\r\n", 17 - count0);
    printf("G001-TS2: END\r\n");
    tsk_exit();

    //ended
}

void task1_sub0(void) {
    for(int i = 0; i < 3; i++) {
        printf("aaaa\r\n");
    }
    count0++;
    tsk_exit();
}
void task1_sub1(void) {
    for(int i = 0; i < 3; i++) {
        printf("bbbb\r\n");
    }
    count0++;
    tsk_exit();
}
void task1_sub2(void) {
    printf("if you see me, you messed up bad...\r\n");
    count0--;
    tsk_exit();
}
void task1_sub3(void) {
    printf("there there...\r\n");
    count0++;
    tsk_exit();
}
void task1_sub4(void) {
    printf("I am LOWEST\r\n");
    count0++;
    printf("G001-TS1: %d/5 tests PASSED\r\n", count0);
    printf("G001-TS1: %d/5 tests FAILED\r\n", 5 - count0);
    printf("G001-TS1: END\r\n");
    tsk_exit();
}
void task1_sub5(void) {
    printf("I am LOW\r\n");
    count0++;
    tsk_exit();
}
void task1_sub_end(void) {
    printf("End of task 1\r\n");
    count0++;
    tsk_exit();
}
void task2_sub0(void) {
    for(int i = 0; i < 3; i++) {
        printf("aaaa\r\n");
    }
    count0++;
    tsk_exit();
}
void task2_sub1(void) {
    for(int i = 0; i < 3; i++) {
        printf("bbbb\r\n");
    }
    count0++;
    tsk_exit();
}
void task2_sub2(void) {
    printf("if you see me, you messed up bad...\r\n");
    count0--;
    tsk_exit();
}
void task2_sub3(void) {
    printf("there there...\r\n");
    count0++;
    tsk_exit();
}
void task2_sub4(void) {
    printf("I am LOWEST\r\n");
    count0++;
    printf("G001-TS1: %d/5 tests PASSED\r\n", count0);
    printf("G001-TS1: %d/5 tests FAILED\r\n", 5 - count0);
    printf("G001-TS1: END\r\n");
    tsk_exit();
}
void task2_sub5(void) {
    printf("I am LOW\r\n");
    count0++;
    tsk_exit();
}
void task2_sub_end(void) {
    printf("End of task 2\r\n");
    count0++;
    tsk_exit();
}
