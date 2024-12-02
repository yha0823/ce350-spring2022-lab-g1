/** 
 * @brief The KCD Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "printf.h"

S32 index_of_char(U8 cmd_char);
int state_to_string(U8 *buffer, U8 state);
void print_state_to_cdisp(int starting_index, int valid_tsks_found, task_t *all_tasks_buffer);

void task_kcd(void)
{
    U8 cmd_not_found[] = "Command not found\r\n";
    U8 invalid_cmd[] = "Invalid command\r\n";
#ifdef DEBUG_0
    printf("Creating KCD mailbox.\r\n");
#endif /* DEBUG_0 */  

    if (mbx_create(KCD_MBX_SIZE) != RTX_ERR) {
#ifdef DEBUG_0
        printf("Successfully created the KCD mailbox.\r\n");
#endif /* DEBUG_0 */  
    } else {
#ifdef DEBUG_0
        printf("Failed to create the KCD mailbox.\r\n");
#endif /* DEBUG_0 */  
    }

    int number_of_chars_to_remove = 3;
    void *buffer = mem_alloc(KCD_CMD_BUF_SIZE);
    U32 size = 0;
    U8 *input = NULL;

    task_t task_of_command[62] = {0};

    while(1) {
        recv_msg(buffer, KCD_CMD_BUF_SIZE);
        RTX_MSG_HDR *msg_header = (RTX_MSG_HDR *)buffer;
        U8 input_char = *((U8*)buffer + sizeof(RTX_MSG_HDR));

        if (msg_header->type == KEY_IN) {
            void *uart_buf = mem_alloc(sizeof(RTX_MSG_HDR) + 1);
            RTX_MSG_HDR *uart_msg_header = (RTX_MSG_HDR *)uart_buf;
            uart_msg_header->type = DISPLAY;
            uart_msg_header->sender_tid = TID_KCD;
            uart_msg_header->length = sizeof(RTX_MSG_HDR) + 1;
            U8 *temp = (U8 *)uart_msg_header + sizeof(RTX_MSG_HDR);
            *temp = input_char;

            send_msg(TID_CON, uart_buf);

            if(input_char == '\r') {
                void *new_line_uart_buf = mem_alloc(sizeof(RTX_MSG_HDR) + 1);
                RTX_MSG_HDR *new_line_uart_msg_header = (RTX_MSG_HDR *)new_line_uart_buf;
                new_line_uart_msg_header->type = DISPLAY;
                new_line_uart_msg_header->sender_tid = TID_KCD;
                new_line_uart_msg_header->length = sizeof(RTX_MSG_HDR) + 1;
                U8 *new_line_temp = (U8 *)new_line_uart_msg_header + sizeof(RTX_MSG_HDR);
                *new_line_temp = '\n';

                send_msg(TID_CON, new_line_uart_buf);

                if (input[0] == '%' && size - number_of_chars_to_remove + sizeof(MSG_HDR_SIZE) < KCD_CMD_BUF_SIZE) {
                    if (input[1] == 'L') {
                        task_t *all_tasks_buffer = (task_t *)mem_alloc(sizeof(task_t) * MAX_TASKS);
                        for(size_t i = 0; i < MAX_TASKS; i++) {
                            all_tasks_buffer[i] = MAX_TASKS;
                        }

                        int valid_tsks_found = 0;

                        if (input[2] == 'T') {
                            valid_tsks_found = tsk_ls(all_tasks_buffer, MAX_TASKS);
                            print_state_to_cdisp(1, valid_tsks_found, all_tasks_buffer);
                        } else if (input[2] == 'M') {
                            valid_tsks_found = mbx_ls(all_tasks_buffer, MAX_TASKS);
                            print_state_to_cdisp(0, valid_tsks_found, all_tasks_buffer);
                        } else {
                            void *sending_buffer = mem_alloc(sizeof(RTX_MSG_HDR) + 19);
                            RTX_MSG_HDR *sending_msg_header = (RTX_MSG_HDR *)sending_buffer;
                            sending_msg_header->sender_tid = TID_KCD;
                            sending_msg_header->type = DISPLAY;
                            sending_msg_header->length = sizeof(RTX_MSG_HDR) + 19;
                            U8 *temp = (U8 *)sending_buffer + sizeof(RTX_MSG_HDR);

                            for(int i = 0; i < 19; i++) {
                                temp[i] = cmd_not_found[i];
                            }

                            send_msg(TID_CON, sending_buffer);
                            mem_dealloc(sending_buffer);
                        }

                        mem_dealloc(all_tasks_buffer);
                    } else {
                        task_t tid = task_of_command[index_of_char(input[1])];

                        if (tid == 0 || input[2] != ' ') {
                            void *sending_buffer = mem_alloc(sizeof(RTX_MSG_HDR) + 19);
                            RTX_MSG_HDR *sending_msg_header = (RTX_MSG_HDR *)sending_buffer;
                            sending_msg_header->sender_tid = TID_KCD;
                            sending_msg_header->type = DISPLAY;
                            sending_msg_header->length = sizeof(RTX_MSG_HDR) + 19;
                            U8 *temp = (U8 *)sending_buffer + sizeof(RTX_MSG_HDR);

                            for(int i = 0; i < 19; i++) {
                                temp[i] = cmd_not_found[i];
                            }

                            send_msg(TID_CON, sending_buffer);
                            mem_dealloc(sending_buffer);
                        } else {
                            void *sending_buffer = mem_alloc(sizeof(RTX_MSG_HDR) + size - number_of_chars_to_remove);
                            RTX_MSG_HDR *sending_msg_header = (RTX_MSG_HDR *)sending_buffer;
                            sending_msg_header->sender_tid = TID_KCD;
                            sending_msg_header->type = KCD_CMD;
                            sending_msg_header->length = sizeof(RTX_MSG_HDR) + size - number_of_chars_to_remove;
                            task_t receiver_tid = task_of_command[index_of_char(input[1])];
                            U8 *temp = (U8 *)sending_buffer + sizeof(RTX_MSG_HDR);
                            for(int i = 0; i < size - number_of_chars_to_remove; i++) {
                                temp[i] = input[i + number_of_chars_to_remove];
                            }

                            send_msg(receiver_tid, sending_buffer);
                        }
                    }
                } else {
                    void *sending_buffer = mem_alloc(sizeof(RTX_MSG_HDR) + 17);
                    RTX_MSG_HDR *sending_msg_header = (RTX_MSG_HDR *)sending_buffer;
                    sending_msg_header->sender_tid = TID_KCD;
                    sending_msg_header->type = DISPLAY;
                    sending_msg_header->length = sizeof(RTX_MSG_HDR) + 17;
                    U8 *temp = (U8 *)sending_buffer + sizeof(RTX_MSG_HDR);

                    for(int i = 0; i < 17; i++) {
                        temp[i] = invalid_cmd[i];
                    }

                    send_msg(TID_CON, sending_buffer);
                    mem_dealloc(sending_buffer);
                }

                size = 0;
                mem_dealloc(input);
                input = NULL;
            } else {
                size++;
                U8 *temp = (U8 *)mem_alloc(size);
                for(size_t j = 0; j < size; j++) {
                    if(j == size - 1) {
                        temp[j] = input_char;
                    } else {
                        temp[j] = input[j];
                    }
                }
                if(input != NULL) {
                    mem_dealloc(input);
                }
                input = temp;
            }
        } else if (msg_header->type == KCD_REG && index_of_char(input_char) != -1) {
            task_of_command[index_of_char(input_char)] = msg_header->sender_tid;
        }
    }
}

S32 index_of_char(U8 cmd_char) {
    if((U32)cmd_char < 48) {
        return -1;
    } else if((U32)cmd_char < 58) {
        return (U32)cmd_char - 48;
    } else if((U32)cmd_char < 65) {
        return -1;
    } else if((U32)cmd_char < 91) {
        return (U32)cmd_char - 65 + 10;
    } else if((U32)cmd_char < 97) {
        return -1;
    } else if((U32)cmd_char < 123) {
        return (U32)cmd_char - 97 + 10 + 26;
    } else {
        return -1;
    }
}

int state_to_string(U8 *buffer, U8 state) {
    char dormant[] = "DORMANT";
    char ready[] = "READY";
    char running[] = "RUNNING";
    char blk_send[] = "BLK_SEND";
    char blk_recv[] = "BLK_RECV";
    char suspended[] = "SUSPENDED";
    if(state == 0) {
        for(task_t n = 0; n < 8; n++) {
            buffer[n] = dormant[n];
        }
        return 8;
    } else if(state == 1) {
        for(task_t n = 0; n < 6; n++) {
            buffer[n] = ready[n];
        }
        return 6;
    } else if(state == 2) {
        for(task_t n = 0; n < 8; n++) {
            buffer[n] = running[n];
        }
        return 8;
    } else if(state == 3) {
        for(task_t n = 0; n < 9; n++) {
            buffer[n] = blk_send[n];
        }
        return 9;
    } else if(state == 4) {
        for(task_t n = 0; n < 9; n++) {
            buffer[n] = blk_recv[n];
        }
        return 9;
    } else if(state == 5) {
        for(task_t n = 0; n < 10; n++) {
            buffer[n] = suspended[n];
        }
        return 10;
    }
    return -1;
}

void print_state_to_cdisp(int starting_index, int valid_tsks_found, task_t *all_tasks_buffer) {
    int count = MSG_HDR_SIZE;
    void *full_buffer = mem_alloc(count);

    for(int i = starting_index; i < valid_tsks_found; i++) {
        RTX_TASK_INFO task_info;
        int res = tsk_get(all_tasks_buffer[i], &task_info);

        if (res == RTX_OK) {
            U8 *state_in_chars = (U8 *)mem_alloc(sizeof(U8) * 10);
            int size_of_state_string = state_to_string(state_in_chars, task_info.state);
            void *temp;

            if (starting_index == 1) {
                count += 17 + size_of_state_string - 1;

                temp = mem_alloc(count);

                sprintf((char *)temp + MSG_HDR_SIZE, "%sTID: %d, STATE: %s\r\n", (char *)full_buffer + MSG_HDR_SIZE, all_tasks_buffer[i], state_in_chars);
            } else {
                int free_space = mbx_get(all_tasks_buffer[i]);
                int digits = 1;

                for(int i = free_space; i >= 10; i /= 10) {
                    digits++;
                }

                count += 31 + size_of_state_string + digits - 1;

                temp = mem_alloc(count);

                sprintf((char *)temp + MSG_HDR_SIZE, "%sTID: %d, STATE: %s, FREE SPACE: %d\r\n", (char *)full_buffer + MSG_HDR_SIZE, all_tasks_buffer[i], state_in_chars, free_space);
            }

            if (full_buffer != NULL) {
                mem_dealloc(full_buffer);
            }

            full_buffer = temp;
            mem_dealloc(state_in_chars);
        }
    }

    RTX_MSG_HDR *msg_hdr = (RTX_MSG_HDR *)full_buffer;
    msg_hdr->type = DISPLAY;
    msg_hdr->sender_tid = TID_KCD;
    msg_hdr->length = count;
    send_msg(TID_CON, full_buffer);
    mem_dealloc(full_buffer);
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

