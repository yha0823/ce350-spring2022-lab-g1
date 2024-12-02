/**
 * @brief The Wall Clock Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "printf.h"

#define CHARACTERS_TO_DISPLAY_TIME 10

int convert_string_digit_to_int(U8 digit) {
    return digit - '0';
}

void send_not_valid_cmd(void) {
    void *send_error_buf = mem_alloc(MSG_HDR_SIZE + 21);
    ((RTX_MSG_HDR *)send_error_buf)->length = MSG_HDR_SIZE + 21;
    ((RTX_MSG_HDR *)send_error_buf)->sender_tid = TID_WCLCK;
    ((RTX_MSG_HDR *)send_error_buf)->type = DISPLAY;
    sprintf((char *)send_error_buf + MSG_HDR_SIZE, "Not a valid command\r\n");
    send_msg_nb(TID_CON, send_error_buf);
    mem_dealloc(send_error_buf);
}

void task_wall_clock(void)
{
    BOOL enable_clk = TRUE;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    void *send_buf = mem_alloc(MSG_HDR_SIZE + CHARACTERS_TO_DISPLAY_TIME);
    ((RTX_MSG_HDR *)send_buf)->length = MSG_HDR_SIZE + CHARACTERS_TO_DISPLAY_TIME;
    ((RTX_MSG_HDR *)send_buf)->sender_tid = TID_WCLCK;
    ((RTX_MSG_HDR *)send_buf)->type = DISPLAY;
    int res = mbx_create(KCD_MBX_SIZE);

    if (res == RTX_ERR) {
        printf("Failed to create wall clock mbx\r\n");
    }

    void *buf = mem_alloc(MSG_HDR_SIZE + 1);
    ((RTX_MSG_HDR *)buf)->length = MSG_HDR_SIZE + 1;
    ((RTX_MSG_HDR *)buf)->sender_tid = TID_WCLCK;
    ((RTX_MSG_HDR *)buf)->type = KCD_REG;

    *((U8 *)buf + MSG_HDR_SIZE) = 'W';
    res = send_msg_nb(TID_KCD, buf);
    mem_dealloc(buf);

    if (res == RTX_ERR) {
        printf("Failed to register the wall clock KCD command\r\n");
    }

    buf = mem_alloc(KCD_MBX_SIZE);

    TIMEVAL *timev = mem_alloc(sizeof(TIMEVAL));
    timev->sec = 1;
    timev->usec = 0;

    res = rt_tsk_set(timev);
    mem_dealloc(timev);

    if (res == RTX_ERR) {
        printf("Failed to set wall clock to rt task\r\n");
    }

    while(1) {
        rt_tsk_susp();

        if (enable_clk) {
            // Send msg to the cdisp task to display the new time
            sprintf((char *)send_buf + MSG_HDR_SIZE, "%02d:%02d:%02d\r\n", hours, minutes, seconds);
            res = send_msg_nb(TID_CON, send_buf);
					
            if (res == RTX_ERR) {
                printf("Failed to update the clock");
            }
        }

        // Update the clock
        if (minutes == 59 && seconds == 59) {
            hours = (hours + 1) % 24;
        }

        if (seconds == 59) {
            minutes = (minutes + 1) % 60;
        }

        seconds = (seconds + 1) % 60;

        res = recv_msg_nb(buf, KCD_MBX_SIZE);

        if (res == RTX_OK) {
            U8 *data = (U8 *)buf + MSG_HDR_SIZE;
            
            if (data[0] == 'W') {
                if (data[1] == 'R') {
                    // Reset the clock
                    hours = 0;
                    minutes = 0;
                    seconds = 0;
                    enable_clk = TRUE;
                } else if (data[1] == 'S') {
                    // Set the clock
                    if (((RTX_MSG_HDR *)buf)->length == 17 && data[2] == ' ' && data[5] == ':' && data[8] == ':') {
                        // Set the time if it's correct
                        int hour_d1 = convert_string_digit_to_int(data[3]);
                        int hour_d2 = convert_string_digit_to_int(data[4]);
                        int minute_d1 = convert_string_digit_to_int(data[6]);
                        int minute_d2 = convert_string_digit_to_int(data[7]);
                        int second_d1 = convert_string_digit_to_int(data[9]);
                        int second_d2 = convert_string_digit_to_int(data[10]);
                        int temp_hours = hour_d1 * 10 + hour_d2;
                        int temp_minutes = minute_d1 * 10 + minute_d2;
                        int temp_seconds = second_d1 * 10 + second_d2;

                        if (temp_hours < 24 && temp_minutes < 60 && temp_seconds < 60) {
                            hours = temp_hours;
                            minutes = temp_minutes;
                            seconds = temp_seconds;
                        } else {
                            void *send_error_buf = mem_alloc(MSG_HDR_SIZE + 18);
                            ((RTX_MSG_HDR *)send_error_buf)->length = MSG_HDR_SIZE + 18;
                            ((RTX_MSG_HDR *)send_error_buf)->sender_tid = TID_WCLCK;
                            ((RTX_MSG_HDR *)send_error_buf)->type = DISPLAY;
                            sprintf((char *)send_error_buf + MSG_HDR_SIZE, "Not a valid time\r\n");
                            send_msg_nb(TID_CON, send_error_buf);
                            mem_dealloc(send_error_buf);
                        }
                    } else {
                        send_not_valid_cmd();
                    }
                } else if (data[1] == 'T') {
                    // Remove the wall clock time
                    enable_clk = FALSE;
                } else {
                    send_not_valid_cmd();
                }
            } else {
                send_not_valid_cmd();
            }
        }
    }
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

