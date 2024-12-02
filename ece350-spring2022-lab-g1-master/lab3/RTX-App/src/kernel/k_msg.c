/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTX LAB  
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *---------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------*/
 

/**************************************************************************//**
 * @file        k_msg.c
 * @brief       kernel message passing routines          
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 JUN
 *****************************************************************************/

#include "k_inc.h"
#include "k_rtx.h"
//#include "k_msg.h"

MAILBOX *mbx_lst[MAX_TASKS] = {0,0,0,0,0,0,0,0,0};          // an array of mailboxes

int add_message_to_mbx(task_t receiver_tid, const void *buf) {
    for(size_t j = 0; j < ((RTX_MSG_HDR *)buf)->length; j++) {
        *(mbx_lst[receiver_tid]->tail) = *((U8 *)buf + j);
        mbx_lst[receiver_tid]->tail++;

        if(mbx_lst[receiver_tid]->tail == (U8 *)mbx_lst[receiver_tid] + mbx_lst[receiver_tid]->size) {
            mbx_lst[receiver_tid]->tail = (U8 *)mbx_lst[receiver_tid] + sizeof(MAILBOX);
        }
    }

    BOOL preempt;

    if (((RTX_MSG_HDR *)buf)->sender_tid == TID_UART) {
        preempt = FALSE;
    } else {
        preempt = TRUE;
    }

    return unblock_recv_task(receiver_tid, preempt);
}

void cleanup_mbx(task_t tid) {
    k_mpool_dealloc(MPID_IRAM2, mbx_lst[tid]);
    mbx_lst[tid] = NULL;
    
    return;
}

int k_mbx_create(size_t size) {
#ifdef DEBUG_0
    printf("k_mbx_create: size = %u\r\n", size);
#endif /* DEBUG_0 */
    task_t mailbox_index = k_tsk_gettid();

    if (mailbox_index == TID_UART) {
        mailbox_index = 0;
    }

    if (mailbox_index != MAX_TASKS) { // This is for when we create the uart0 irq mailbox
        if (k_mbx_get(mailbox_index) != RTX_ERR) {
            errno = EEXIST;
            return RTX_ERR;
        } else if(size < MIN_MSG_SIZE) {
            errno = EINVAL;
            return RTX_ERR;
        }
    }

    MAILBOX *mbx = k_mpool_alloc(MPID_IRAM2, size + sizeof(MAILBOX));

    if (mbx == NULL) {
        errno = ENOMEM;
        return RTX_ERR;
    }

    mbx->head = (U8*)mbx + sizeof(MAILBOX);
    mbx->tail = (U8*)mbx + sizeof(MAILBOX);
    mbx->size = sizeof(MAILBOX) + size;
    mbx->is_empty = 1;
    mbx_lst[mailbox_index] = mbx;

    for(U8 *i = mbx->head; i < mbx->head + mbx->size; i++) {
        *i = 0;
    }

    return (int)mailbox_index;
}

int k_send_msg(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
    RTX_TASK_INFO useless;
    BOOL blocked = 0;
    int receiver_mid;

    if (receiver_tid == TID_UART) {
        receiver_mid = 0;
    } else {
        receiver_mid = receiver_tid;
    }

    if(receiver_tid != TID_UART && k_tsk_get(receiver_mid, &useless) == RTX_ERR && errno == EINVAL) {
        return RTX_ERR;
    } else if(buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    } else if(((RTX_MSG_HDR *)buf)->length < MIN_MSG_SIZE) {
        errno = EINVAL;
        return RTX_ERR;
    } else if(k_mbx_get(receiver_mid) == RTX_ERR) {
        errno = ENOENT;
        return RTX_ERR;
    } else if(((RTX_MSG_HDR *)buf)->length > mbx_lst[receiver_mid]->size) {
        errno = EMSGSIZE;
        return RTX_ERR;
    } else if (receiver_mid == ((RTX_MSG_HDR *)buf)->sender_tid) { // can't send to itself 
        return RTX_ERR;
    }
    
    RTX_TASK_INFO receiver_info_buf;
    if (receiver_tid != TID_UART && k_tsk_get(receiver_mid, &receiver_info_buf) != RTX_OK) {
        return RTX_ERR;
    }

    if (receiver_info_buf.state == DORMANT) { // receiver in DORMANT or BLK_SEND state prior to entering wait queue
        return RTX_ERR;
    }

    if (!mbx_lst[receiver_mid]->is_empty && ((RTX_MSG_HDR *)buf)->length > k_mbx_get(receiver_mid)) {
        int res = block_sending_mbx_queue(receiver_mid, ((RTX_MSG_HDR *)buf)->length);
        blocked = 1;
        if (res == RTX_ERR) {
            return res; // If this happens we broke something
        } else if(res == MAX_TASKS) {
            return RTX_ERR;
        }
    } else {
        // modify_sending_mbx_queue will set errno if there is an error
        int res = modify_sending_mbx_queue(1, receiver_mid, buf, &blocked);

        if (res == RTX_ERR) {
            return res; // If this happens we broke something
        } else if(res == MAX_TASKS) {
            return RTX_ERR;
        }
    }

    mbx_lst[receiver_mid]->is_empty = 0;
    add_message_to_mbx(receiver_mid, buf);

    if(blocked) {
        k_tsk_yield();
    }

    return RTX_OK;
}

int k_send_msg_nb(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg_nb: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
    RTX_TASK_INFO useless;

    if(k_tsk_get(receiver_tid, &useless) == -1 && errno == EINVAL) {
        return RTX_ERR;
    } else if(buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    } else if(((RTX_MSG_HDR *)buf)->length < MIN_MSG_SIZE) {
        errno = EINVAL;
        return RTX_ERR;
    } else if(k_mbx_get(receiver_tid) == -1) {
        errno = ENOENT;
        return RTX_ERR;
    } else if(((RTX_MSG_HDR *)buf)->length > mbx_lst[receiver_tid]->size) {
        errno = EMSGSIZE;
        return RTX_ERR;
    } else if (receiver_tid == ((RTX_MSG_HDR *)buf)->sender_tid) { // can't send to itself 
        return RTX_ERR;
    }

    RTX_TASK_INFO receiver_info_buf;
    if (k_tsk_get(receiver_tid, &receiver_info_buf) != RTX_OK) {
        return RTX_ERR;
    }

    if (receiver_info_buf.state == DORMANT) { // receiver in DORMANT state prior to entering wait queue
        return RTX_ERR;
    }
    
    if(!mbx_lst[receiver_tid]->is_empty && ((RTX_MSG_HDR *)buf)->length > k_mbx_get(receiver_tid)) {
        errno = ENOSPC;
        return RTX_ERR;
    } else {
        // modify_sending_mbx_queue will set errno if there is an error
        BOOL dummy = 0;
        int res = modify_sending_mbx_queue(0, receiver_tid, buf, &dummy);

        if (res == RTX_ERR) {
            return res;
        }
    }

    mbx_lst[receiver_tid]->is_empty = 0;
    return add_message_to_mbx(receiver_tid, buf);
}

int k_recv_msg(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
    
    task_t current_tid = k_tsk_gettid();

    if(buf == NULL){ 
        errno = EFAULT;
        return RTX_ERR;
    } else if(len < ((RTX_MSG_HDR *)(mbx_lst[current_tid]->head))->length) {
        errno = ENOSPC;
        return RTX_ERR;
    } else if (k_mbx_get(current_tid) == RTX_ERR) {
        errno = ENOENT;
        return RTX_ERR;
    }

    if (mbx_lst[current_tid]->is_empty) {
        block_recv_task(current_tid);
    }

    U32 length = ((RTX_MSG_HDR *)(mbx_lst[current_tid]->head))->length;

    for(size_t j = 0; j < length; j++) {
        *((U8 *)buf + j) = *(mbx_lst[current_tid]->head);
        *(mbx_lst[current_tid]->head) = 0;
        mbx_lst[current_tid]->head++;

        if(mbx_lst[current_tid]->head == (U8 *)mbx_lst[current_tid] + mbx_lst[current_tid]->size) {
            mbx_lst[current_tid]->head = (U8 *)mbx_lst[current_tid] + sizeof(MAILBOX);
        }
    }

    mbx_lst[current_tid]->is_empty = (mbx_lst[current_tid]->head == mbx_lst[current_tid]->tail);

    checking_sending_queue_for_task_corresponding_to_tid_mailbox_then_if_there_are_tasks_waiting_to_send_proceed_to_let_them_finish_the_send_as_long_as_there_is_enough_space_remaining_in_the_mailbox_and_ensure_the_sender_immediately_gives_control_back_to_receiver_after_sending_completes_and_loop_until_there_is_no_longer_enough_space_in_the_mailbox(current_tid);

    return RTX_OK;
}

int k_recv_msg_nb(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg_nb: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */

    task_t current_tid = k_tsk_gettid();

    if (current_tid == TID_UART) {
        current_tid = 0;
    }

    if(buf == NULL){ 
        errno = EFAULT;
        return RTX_ERR;
    } else if(len < ((RTX_MSG_HDR *)(mbx_lst[current_tid]->head))->length) {
        errno = ENOSPC;
        return RTX_ERR;
    } else if (k_mbx_get(current_tid) == -1) {
        errno = ENOENT;
        return RTX_ERR;
    }

    if (mbx_lst[current_tid]->is_empty) {
        errno = ENOMSG;
        return RTX_ERR;
    } else {
        U32 length = ((RTX_MSG_HDR *)(mbx_lst[current_tid]->head))->length;

        for(size_t j = 0; j < length; j++) {
            *((U8 *)buf + j) = *(mbx_lst[current_tid]->head);
            *(mbx_lst[current_tid]->head) = 0;
            mbx_lst[current_tid]->head++;

            if(mbx_lst[current_tid]->head == (U8 *)mbx_lst[current_tid] + mbx_lst[current_tid]->size) {
                mbx_lst[current_tid]->head = (U8 *)mbx_lst[current_tid] + sizeof(MAILBOX);
            }
        }
    }

    mbx_lst[current_tid]->is_empty = (mbx_lst[current_tid]->head == mbx_lst[current_tid]->tail);

    checking_sending_queue_for_task_corresponding_to_tid_mailbox_then_if_there_are_tasks_waiting_to_send_proceed_to_let_them_finish_the_send_as_long_as_there_is_enough_space_remaining_in_the_mailbox_and_ensure_the_sender_immediately_gives_control_back_to_receiver_after_sending_completes_and_loop_until_there_is_no_longer_enough_space_in_the_mailbox(current_tid);

    return RTX_OK;
}

int k_mbx_ls(task_t *buf, size_t count) {
#ifdef DEBUG_0
    printf("k_mbx_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
    if(buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    U32 index = 0;

    for (U32 i = 1; (index < count) && (i < MAX_TASKS); i++) {
        if (k_mbx_get((task_t)i) != RTX_ERR) {
            buf[index] = (task_t)i;
            index++;
        }
    }

    return index;
}

int k_mbx_get(task_t tid)
{
#ifdef DEBUG_0
    printf("k_mbx_get: tid=%u\r\n", tid);
#endif /* DEBUG_0 */

    if (mbx_lst[tid] == NULL) {
        errno = ENOENT;
        return RTX_ERR;
    } else if(mbx_lst[tid]->is_empty) {
        return mbx_lst[tid]->size;
    } else if(mbx_lst[tid]->head < mbx_lst[tid]->tail) {
        return mbx_lst[tid]->size - (size_t)(mbx_lst[tid]->tail - mbx_lst[tid]->head);
    } else {
        return (size_t)(mbx_lst[tid]->head - mbx_lst[tid]->tail);
    }
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

