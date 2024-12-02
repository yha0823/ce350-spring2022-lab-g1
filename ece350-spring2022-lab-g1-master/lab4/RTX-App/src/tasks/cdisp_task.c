/**
 * @brief The Console Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "LPC17xx.h"
#include "uart_polling.h"
#include "printf.h"

void task_cdisp(void)
{
    LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
#ifdef DEBUG_0
    printf("Creating CDISP mailbox.\r\n");
#endif /* DEBUG_0 */  

    if (mbx_create(CON_MBX_SIZE) != RTX_ERR) {
#ifdef DEBUG_0
        printf("Successfully created the CDISP mailbox.\r\n");
#endif /* DEBUG_0 */  
    } else {
#ifdef DEBUG_0
        printf("Failed to create the CDISP mailbox.\r\n");
#endif /* DEBUG_0 */  
    }

    void *buffer = mem_alloc(CON_MBX_SIZE);

    while(1) {
        recv_msg(buffer, CON_MBX_SIZE);
        RTX_MSG_HDR *hdr_info = (RTX_MSG_HDR *)buffer;

        if (hdr_info->type != DISPLAY) {
            continue;
        }

        if (hdr_info->length - MSG_HDR_SIZE == 1) {
            pUart->THR = *((U8 *)buffer + MSG_HDR_SIZE);
        } else if (hdr_info->length - MSG_HDR_SIZE > 1) {
            U8 trigger_char = *((U8 *)buffer + MSG_HDR_SIZE);
            hdr_info->length--;

            U8 *temp = (U8 *)buffer;
            for(int i = MSG_HDR_SIZE; i < hdr_info->length; i++) {
                temp[i] = temp[i + 1];
            }

            send_msg(TID_UART, buffer);

            pUart->THR = trigger_char;
            pUart->IER |= IER_THRE;
        }
    }
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

