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
    printf("G001-TS3: START\r\n");

    printf("Creating task 1\r\n");
    tasks[0].priv  = 0;
    tasks[0].ptask = &task1;
    tasks[0].u_stack_size = PROC_STACK_SIZE;
    tasks[0].prio = HIGH;

    printf("Creating task 2\r\n");
    tasks[1].priv  = 0;
    tasks[1].ptask = &task2;
    tasks[1].u_stack_size = PROC_STACK_SIZE;
    tasks[1].prio = HIGH;

    printf("Creating task 3\r\n");
    tasks[2].priv  = 0;
    tasks[2].ptask = &task3;
    tasks[2].u_stack_size = PROC_STACK_SIZE;
    tasks[2].prio = HIGH;
}

void task1(void) {
    int result[4] = {-2, -2, -2, -2};
    void *receive_buf = mem_alloc(0x40);

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(7);
    msg->length = 7;
    msg->sender_tid = (task_t)1;
    msg->type = KCD_REG;

    U8 *data = (U8*)msg + MSG_HDR_SIZE;
    U8 temp_string = '1';
    *data = temp_string;

    printf("Creating a mailbox at 1\r\n");
    result[0] = mbx_create(0x40);

    if (result[0] != 1) {
        printf("Failed to create a mailbox at 1\r\n");
        return;
    }

    printf("Sending KCD REG message to KCD task from task 1\r\n");
    result[1] = send_msg(9, msg);

    if (result[1] != RTX_OK) {
        printf("Failed to send KCD REG to KCD Mailbox at 1\r\n");
        return;
    }

    RTX_MSG_HDR *msg1 = (RTX_MSG_HDR *)mem_alloc(7);
    msg1->length = 7;
    msg1->sender_tid = (task_t)1;
    msg1->type = KCD_REG;

    U8 *data1 = (U8*)msg1 + MSG_HDR_SIZE;
    U8 temp_string1 = 'a';
    *data1 = temp_string1;

    printf("Sending KCD REG message to KCD task from task 1\r\n");
    result[2] = send_msg(9, msg1);

    if (result[2] != RTX_OK) {
        printf("Failed to send KCD REG to KCD Mailbox at 1\r\n");
        return;
    }

    while (TRUE) {
        result[3] = recv_msg(receive_buf, 0x40);

        if (result[3] != RTX_OK) {
            printf("Receive failed at task 1\r\n");
            return;
        }

        if (((RTX_MSG_HDR *)receive_buf)->type == KCD_CMD) {
            break;
        }
    }

    printf("RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);
        
    printf("Exiting from task 1\r\n");
    tsk_exit();
};

void task2(void) {
    int result[4] = {-2, -2, -2, -2};
    void *receive_buf = mem_alloc(KCD_MBX_SIZE);

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(7);
    msg->length = 7;
    msg->sender_tid = (task_t)2;
    msg->type = KCD_REG;

    U8 *data = (U8*)msg + MSG_HDR_SIZE;
    U8 temp_string = '2';
    *data = temp_string;

    printf("Creating a mailbox at 2\r\n");
    result[0] = mbx_create(0x40);

    if (result[0] != 2) {
        printf("Failed to create a mailbox at 2\r\n");
        return;
    }

    printf("Sending KCD REG message to KCD task from task 2\r\n");
    result[1] = send_msg(9, msg);

    if (result[1] != RTX_OK) {
        printf("Failed to send KCD REG to KCD Mailbox at 2\r\n");
        return;
    }

    RTX_MSG_HDR *msg1 = (RTX_MSG_HDR *)mem_alloc(7);
    msg1->length = 7;
    msg1->sender_tid = (task_t)2;
    msg1->type = KCD_REG;

    U8 *data1 = (U8*)msg1 + MSG_HDR_SIZE;
    U8 temp_string1 = 'b';
    *data1 = temp_string1;

    printf("Sending KCD REG message to KCD task from task 2\r\n");
    result[2] = send_msg(9, msg1);

    if (result[2] != RTX_OK) {
        printf("Failed to send KCD REG to KCD Mailbox at 2\r\n");
        return;
    }

    while (TRUE) {
        result[3] = recv_msg(receive_buf, 0x40);

        if (result[3] != RTX_OK) {
            printf("Receive failed at task 2\r\n");
            return;
        }

        if (((RTX_MSG_HDR *)receive_buf)->type == KCD_CMD) {
            break;
        }
    }

    printf("RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    printf("Exiting from task 2\r\n");
    tsk_exit();
};

void task3(void) {
    int result[4] = {-2, -2, -2, -2};
    void *receive_buf = mem_alloc(KCD_MBX_SIZE);

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(7);
    msg->length = 7;
    msg->sender_tid = (task_t)3;
    msg->type = KCD_REG;

    U8 *data = (U8*)msg + MSG_HDR_SIZE;
    U8 temp_string = '3';
    *data = temp_string;

    printf("Creating a mailbox at 3\r\n");
    result[0] = mbx_create(0x40);

    if (result[0] != 3) {
        printf("Failed to create a mailbox at 3\r\n");
        return;
    }

    printf("Sending KCD REG message to KCD task from task 3\r\n");
    result[1] = send_msg(9, msg);

    if (result[1] != RTX_OK) {
        printf("Failed to send KCD REG to KCD Mailbox at 3\r\n");
        return;
    }

    RTX_MSG_HDR *msg1 = (RTX_MSG_HDR *)mem_alloc(7);
    msg1->length = 7;
    msg1->sender_tid = (task_t)3;
    msg1->type = KCD_REG;

    U8 *data1 = (U8*)msg1 + MSG_HDR_SIZE;
    U8 temp_string1 = 'c';
    *data1 = temp_string1;

    printf("Sending KCD REG message to KCD task from task 3\r\n");
    result[2] = send_msg(9, msg1);

    if (result[2] != RTX_OK) {
        printf("Failed to send KCD REG to KCD Mailbox at 3\r\n");
        return;
    }

    while (TRUE) {
        result[3] = recv_msg(receive_buf, 0x40);

        if (result[3] != RTX_OK) {
            printf("Receive failed at task 3\r\n");
            return;
        }

        if (((RTX_MSG_HDR *)receive_buf)->type == KCD_CMD) {
            break;
        }
    }

    printf("RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    printf("Exiting from task 3\r\n");
    tsk_exit();
};
