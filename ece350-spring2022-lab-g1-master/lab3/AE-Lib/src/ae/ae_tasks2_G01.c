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
#define NUM_INIT_TASKS  6       // number of tasks during initialization
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
void task4(void);
void task5(void);
void task6(void);

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    printf("G001-TS2: START\r\n");

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

    printf("Creating task 4\r\n");
    tasks[3].priv  = 0;
    tasks[3].ptask = &task4;
    tasks[3].u_stack_size = PROC_STACK_SIZE;
    tasks[3].prio = HIGH;

    printf("Creating task 5\r\n");
    tasks[4].priv  = 0;
    tasks[4].ptask = &task5;
    tasks[4].u_stack_size = PROC_STACK_SIZE;
    tasks[4].prio = HIGH;

    printf("Creating task 6\r\n");
    tasks[5].priv  = 0;
    tasks[5].ptask = &task6;
    tasks[5].u_stack_size = PROC_STACK_SIZE;
    tasks[5].prio = HIGH;
}

void task1(void) {
    int result[10] = {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2};
    void *receive_buf = mem_alloc(0x20);

    printf("Set task 3 prio LOW\r\n");
    result[0] = tsk_set_prio(3, LOW);

    if (result[0] != RTX_OK) {
        printf("Could not set task 3 prio to LOW\r\n");
        return;
    }

    printf("Set task 4 prio LOW\r\n");
    result[1] = tsk_set_prio(4, LOW);

    if (result[1] != RTX_OK) {
        printf("Could not set task 4 prio to LOW\r\n");
        return;
    }

    printf("Set task 5 prio LOW\r\n");
    result[2] = tsk_set_prio(5, LOW);

    if (result[2] != RTX_OK) {
        printf("Could not set task 5 prio to LOW\r\n");
        return;
    }

    printf("Set task 6 prio LOW\r\n");
    result[3] = tsk_set_prio(6, LOW);

    if (result[3] != RTX_OK) {
        printf("Could not set task 6 prio to LOW\r\n");
        return;
    }

    printf("Create mailbox of task 1 size 0x40\r\n");
    result[4] = mbx_create(0x40);

    if (result[4] != 1) {
        printf("Could not create the mailbox from 1\r\n");
        return;
    }

    printf("Yield from 1\r\n");
    result[5] = tsk_yield();

    if (result[5] != RTX_OK) {
        printf("Yield from 1 failure\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[6] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    if (result[6] != RTX_OK) {
        printf("Could not receive the message at mailbox 1\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[7] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    if (result[7] != RTX_OK) {
        printf("Could not receive the message at mailbox 1\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[8] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    if (result[8] != RTX_OK) {
        printf("Could not receive the message at mailbox 1\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[9] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    if (result[9] != RTX_OK) {
        printf("Could not receive the message at mailbox 1\r\n");
        return;
    }

    mem_dealloc(receive_buf);
    
    printf("Exiting task 1\r\n");
    tsk_exit();
}

void task2(void) {
    int result[10] = {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2};
    void *receive_buf = mem_alloc(0x20);

    printf("Create mailbox of task 2 size 0x40\r\n");
    result[0] = mbx_create(0x40);

    if (result[0] != 2) {
        printf("Could not create mailbox from 2\r\n");
        return;
    }

    printf("Set task 1 prio LOWEST\r\n");
    result[2] = tsk_set_prio(1, LOWEST);

    if (result[2] != RTX_OK) {
        printf("Could not set task 1 prio to LOWEST\r\n");
        return;
    }

    printf("Set task 2 prio LOWEST\r\n");
    result[3] = tsk_set_prio(2, LOWEST);

    if (result[3] != RTX_OK) {
        printf("Could not set task 2 prio to LOWEST\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[4] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);

    if (result[4] != RTX_OK) {
        printf("Could not receive the message at mailbox 2\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[5] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);


    if (result[5] != RTX_OK) {
        printf("Could not receive the message at mailbox 2\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[6] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);


    if (result[6] != RTX_OK) {
        printf("Could not receive the message at mailbox 2\r\n");
        return;
    }

    printf("Receive message *NB*\r\n");
    result[7] = recv_msg_nb(receive_buf, 0x20);
    printf("Received message -> RTX_MSG_HDR: type: %d, sender_tid: %d, length: %d, data: %s\r\n", ((RTX_MSG_HDR *)receive_buf)->type, ((RTX_MSG_HDR *)receive_buf)->sender_tid, ((RTX_MSG_HDR *)receive_buf)->length, (U8 *)receive_buf + MSG_HDR_SIZE);


    if (result[7] != RTX_OK) {
        printf("Could not receive the message at mailbox 2\r\n");
        return;
    }

    mem_dealloc(receive_buf);

    printf("Exiting task 2\r\n");
    tsk_exit();
}

void task3(void) {
    int result[4] = {-2, -2, -2, -2};

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
        printf("Could not send message to 1 from 3\r\n");
        return;
    }

    printf("Yield from 3\r\n");
    result[1] = tsk_yield();
    
    if (result[1] != RTX_OK) {
        printf("Yield from 3 failure\r\n");
        return;
    }

    printf("Sending message to task 1 from task 3 *NB*\r\n"); // should error
    result[2] = send_msg_nb(1, msg);

    if (result[2] != -1) {
        printf("??? %d\r\n", result[2]);
        return;
    }

    printf("Sending message to task 1 from task 3\r\n");
    result[3] = send_msg(1, msg);

    if (result[3] != RTX_OK) {
        printf("Could not send message to 1 from 3\r\n");
        return;
    }

    mem_dealloc((void *)msg);

    printf("Exiting task 3\r\n");
    tsk_exit();
}

void task4(void) {
    int result[5] = {-2, -2, -2, -2, -2};

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(0x20);
    msg->length = 0x20;
    msg->sender_tid = (task_t)4;
    msg->type = DEFAULT;

    U8 *data = (U8*)msg + 6;
    U8 temp_string[] = "abcdefghijklmnopqrstuvwxy4";
    for(int i = 0; i < sizeof(temp_string)/sizeof(U8); i++) {
        *(data + i) = temp_string[i];
    }

    printf("Sending message to task 1 from task 4\r\n");
    result[0] = send_msg(1, msg);

    if (result[0] != RTX_OK) {
        printf("Could not send message to 1 from 4\r\n");
        return;
    }

    printf("Yield from 4\r\n");
    result[1] = tsk_yield();

    if (result[1] != RTX_OK) {
        printf("Yield from 4 failure\r\n");
        return;
    }

    printf("Sending message to task 1 from task 4 *NB*\r\n"); // should error
    result[2] = send_msg_nb(1, msg);

    if (result[2] != -1) {
        printf("???\r\n");
        return;
    }

    printf("Sending message to task 1 from task 4\r\n");
    result[3] = send_msg(1, msg);

    if (result[3] != RTX_OK) {
        printf("Could not send message to 1 from 4\r\n");
        return;
    }

    mem_dealloc((void *)msg);

    printf("Exiting task 4\r\n");
    tsk_exit();
}

void task5(void) {
    int result[4] = {-2, -2, -2, -2};

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(0x20);
    msg->length = 0x20;
    msg->sender_tid = (task_t)5;
    msg->type = DEFAULT;

    U8 *data = (U8*)msg + 6;
    U8 temp_string[] = "abcdefghijklmnopqrstuvwxy5";
    for(int i = 0; i < sizeof(temp_string)/sizeof(U8); i++) {
        *(data + i) = temp_string[i];
    }

    printf("Sending message to task 2 from task 5\r\n");
    result[0] = send_msg(2, msg);

    if (result[0] != RTX_OK) {
        printf("Could not send message to 2 from 5\r\n");
        return;
    }

    printf("Yield from 5\r\n");
    result[1] = tsk_yield();

    if (result[1] != RTX_OK) {
        printf("Yield from 5 failure\r\n");
        return;
    }

    printf("Sending message to task 2 from task 5 *NB*\r\n"); // should error
    result[2] = send_msg_nb(2, msg);

    if (result[2] != -1) {
        printf("???\r\n");
        return;
    }

    printf("Sending message to task 2 from task 5\r\n");
    result[3] = send_msg(2, msg);

    if (result[3] != RTX_OK) {
        printf("Could not send message to 2 from 5\r\n");
        return;
    }

    mem_dealloc((void *)msg);

    printf("Exiting task 5\r\n");
    tsk_exit();
}

void task6(void) {
    int result[4] = {-2, -2, -2, -2};

    RTX_MSG_HDR *msg = (RTX_MSG_HDR *)mem_alloc(0x20);
    msg->length = 0x20;
    msg->sender_tid = (task_t)6;
    msg->type = DEFAULT;

    U8 *data = (U8*)msg + 6;
    U8 temp_string[] = "abcdefghijklmnopqrstuvwxy6";
    for(int i = 0; i < sizeof(temp_string)/sizeof(U8); i++) {
        *(data + i) = temp_string[i];
    }

    printf("Sending message to task 2 from task 6\r\n");
    result[0] = send_msg(2, msg);

    if (result[0] != RTX_OK) {
        printf("Could not send message to 2 from 6\r\n");
        return;
    }

    printf("Yield from 6\r\n");
    result[1] = tsk_yield();

    if (result[1] != RTX_OK) {
        printf("Yield from 6 failure\r\n");
        return;
    }

    printf("Sending message to task 2 from task 6 *NB*\r\n"); // should error
    result[2] = send_msg_nb(2, msg);

    if (result[2] != -1) {
        printf("???\r\n");
        return;
    }

    printf("Sending message to task 2 from task 6\r\n");
    result[3] = send_msg(2, msg);

    if (result[3] != RTX_OK) {
        printf("Could not send message to 2 from 6 - NB\r\n");
        return;
    }

    mem_dealloc((void *)msg);

    printf("Exiting task 6\r\n");
    tsk_exit();
}
