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
#define NUM_INIT_TASKS  3       // number of tasks during initialization
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

void task1(void);
void task2(void);
void task3(void);

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    printf("G001-TS1: START\r\n");

    printf("Creating task 1\r\n");
    tasks[0].priv  = 0;
    tasks[0].ptask = &task1;
    tasks[0].u_stack_size = PROC_STACK_SIZE;
    tasks[0].prio = HIGH;

    printf("Creating task 2\r\n");
    tasks[1].priv  = 0;
    tasks[1].ptask = &task2;
    tasks[1].u_stack_size = PROC_STACK_SIZE;
    tasks[1].prio = MEDIUM;

    printf("Creating task 3\r\n");
    tasks[2].priv  = 0;
    tasks[2].ptask = &task3;
    tasks[2].u_stack_size = PROC_STACK_SIZE;
    tasks[2].prio = MEDIUM;
}

void task1(void) {
    int result[4] = {-2, -2, -2, -2};
    void *receive_buf = mem_alloc(0x20);

    printf("Creating mailbox for task 1 with size 0x20\r\n");
    result[0] = mbx_create(0x20);

    if (result[0] != 1) {
        printf("Could not create mailbox for task 1\r\n");
        return;
    }

    printf("Receive mailbox 1\r\n");
    result[1] = recv_msg(receive_buf, 0x20);
    if (result[1] != RTX_OK) {
        printf("Could not receive from mailbox 1\r\n");
        return;
    }

    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    printf("Receive mailbox 1\r\n");
    result[2] = recv_msg(receive_buf, 0x20);
    if (result[2] != RTX_OK) {
        printf("Could not receive from mailbox 1\r\n");
        return;
    }

    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    printf("Receive mailbox 1\r\n");
    result[3] = recv_msg(receive_buf, 0x20);
    if (result[3] != RTX_OK) {
        printf("Could not receive from mailbox 1\r\n");
        return;
    }

    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    mem_dealloc(receive_buf);
    
    printf("Exiting task 1\r\n");
    tsk_exit();
}

void task2(void) {
    int result[2] = {-2, -2};

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(0x20);
    msg->length = 0x20;
    msg->sender_tid = (task_t)2;
    msg->type = DEFAULT;

    U8 *data = (U8*)msg + 6;
    U8 temp_string[] = "abcdefghijklmnopqrstuvwxy2";
    for(int i = 0; i < sizeof(temp_string)/sizeof(U8); i++) {
        *(data + i) = temp_string[i];
    }

    printf("Sending message to task 1 from task 2\r\n");
    result[0] = send_msg(1, msg);

    if (result[0] != RTX_OK) {
        printf("Could not send message to task 1's mailbox from 2\r\n");
        return;
    }

    printf("Sending message to task 1 from task 2\r\n");
    result[1] = send_msg(1, msg);

    if (result[1] != RTX_OK) {
        printf("Could not send message to task 1's mailbox from 2\r\n");
        return;
    }

    mem_dealloc((void *)msg);

    printf("Exiting task 2\r\n");
    tsk_exit();
}

void task3(void) {
    int result[1] = {-2};

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(0x20);
    msg->length = 0x20;
    msg->sender_tid = (task_t)3;
    msg->type = DEFAULT;

    U8 *data = (U8*)msg + 6;
    U8 temp_string[] = "abcdefghijklmnopqrstuvwxy3";
    for(int i = 0; i < sizeof(temp_string)/sizeof(U8); i++) {
        *(data + i) = temp_string[i];
    }

    printf("Sending message to task 1 from task 3\r\n");
    result[0] = send_msg(1, msg);

    if (result[0] != RTX_OK) {
        printf("Could not send message to task 1's mailbox from 3\r\n");
        return;
    }

    mem_dealloc((void *)msg);

    printf("Exiting task 3\r\n");
    tsk_exit();
}
