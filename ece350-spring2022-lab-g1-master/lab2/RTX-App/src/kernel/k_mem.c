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
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01.lab2 hi Dohyun :D Brian :D
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

#include "k_inc.h"
#include "k_mem.h"

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
                              |                           |     |
                              |---------------------------|     |
                              |                           |     |
                              |      other data           |     |
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
             g_k_stacks[15]-->|---------------------------|     |
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
 *                            GLOBAL VARIABLES
 *===========================================================================
 */
// kernel stack size, referred by startup_a9.s
const U32 g_k_stack_size = KERN_STACK_SIZE;
// task proc space stack size in bytes, referred by system_a9.c
const U32 g_p_stack_size = PROC_STACK_SIZE;

// task kernel stacks
// U32 g_k_stacks[MAX_TASKS][KERN_STACK_SIZE >> 2] __attribute__((aligned(8)));
U32 *g_k_stacks[MAX_TASKS] = {NULL};

U32 free_list_array_RAM1[8];
U8 binary_bit_array_RAM1[32];
U32 free_list_array_RAM2[11];
U8 binary_bit_array_RAM2[256];

U32 k_lvls_RAM1;
U32 k_lvls_RAM2;

U32 size_RAM1;
U32 size_RAM2;

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

/* note list[n] is for blocks with order of n */
/**
 * @brief return needed address for the requested index
 */

size_t coalesce_binary_bit_array(U32 **free_list_array, U8 **passed_binary_bit_array, U32 index_check, U32 current_level, U32 *list_index);

U32 pow(U32 base, U32 exp) {
    U32 output = 1;

    if (exp == 0) {
        // Do not give a base of 0 and exponent of 0

        return 1;
    }

    for (size_t i = 0; i < exp; i++) {
        output *= base;
    }

    return output;
}

U32 log(U32 base, U32 num) {
    U32 count = 0;

    while (num != 1) {
        num /= base;
        count++;
    }
    
    return count;
}

U32 needed_address(U32 ram_start, U32 level, U32 ram_pool_size, U32 list_index) {
    return ram_start + list_index * (ram_pool_size / pow(2, level));
}

mpool_t k_mpool_create (int algo, U32 start, U32 end)
{
    mpool_t mpid = MPID_IRAM1;

#ifdef DEBUG_0
    printf("k_mpool_init: algo = %d\r\n", algo);
    printf("k_mpool_init: RAM range: [0x%x, 0x%x].\r\n", start, end);
#endif /* DEBUG_0 */    
    
    if (algo != BUDDY ) {
        errno = EINVAL;
        return RTX_ERR;
    }
    

    U32 blocks;
    blocks = (end - start + 1)/MIN_BLK_SIZE;
    
    if (start == RAM1_START) {
        if(end - start + 1 > RAM1_SIZE){// if not enough space for the creating, prints error
            errno = ENOMEM;
            return RTX_ERR;
        }
        size_RAM1 = end - start + 1;
        k_lvls_RAM1 = log(2, blocks);

        free_list_array_RAM1[0] = RAM1_START;
        FreeListNode *first_node = (FreeListNode *)free_list_array_RAM1[0];
        first_node->list_index = 0;
        first_node->prev = NULL;
        first_node->next = NULL;

        for (size_t i = 1; i <= k_lvls_RAM1; i++) {
            free_list_array_RAM1[i] = NULL;
        }

        for (size_t i = 0; i < 16; i++) {
            binary_bit_array_RAM1[i] = 0;
        }
    } else if (start == RAM2_START) {
        if(end - start + 1> RAM2_SIZE){// if not enough space for the creating, prints error
            errno = ENOMEM;
            return RTX_ERR;
        }
        mpid = MPID_IRAM2;
        size_RAM2 = end - start + 1;
        k_lvls_RAM2 = log(2, blocks);

        free_list_array_RAM2[0] = RAM2_START;
        FreeListNode *first_node = (FreeListNode *)free_list_array_RAM2[0];
        first_node->list_index = 0;
        first_node->prev = NULL;
        first_node->next = NULL;

        for (size_t i = 1; i <= k_lvls_RAM2; i++) {
            free_list_array_RAM2[i] = NULL;
        }
        
        for (size_t i = 0; i < 128; i++) {
            binary_bit_array_RAM2[i] = 0;
        }
    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    return mpid;
}

void *k_mpool_alloc (mpool_t mpid, size_t size)
{
#ifdef DEBUG_0
    printf("k_mpool_alloc: mpid = %d, size = %d, 0x%x\r\n", mpid, size, size);
#endif /* DEBUG_0 */
    U32 allocated_address = NULL;
    U32 too_big = TRUE;
    U32 needed_level = 0;
    U32 current_size;
    U32 *free_list_array;
    U8 *binary_bit_array;
    U32 ram_start;
    U32 ram_pool_size;

    if(mpid != MPID_IRAM1 && mpid != MPID_IRAM2){ // this is performed as mpid is invalid if not 1 or 0
        errno = EINVAL;
        return NULL; 
    }

    if (mpid == MPID_IRAM1) {
        current_size = size_RAM1;
        free_list_array = free_list_array_RAM1;
        binary_bit_array = binary_bit_array_RAM1;
        ram_start = RAM1_START;
        ram_pool_size = size_RAM1;
    } else {
        current_size = size_RAM2;
        free_list_array = free_list_array_RAM2;
        binary_bit_array = binary_bit_array_RAM2;
        ram_start = RAM2_START;
        ram_pool_size = size_RAM2;
    }

    if (ram_pool_size < size) {
        errno = ENOMEM;
        return NULL;
    }

    while (too_big) {
        if (current_size / 2 < size || current_size / 2 < MIN_BLK_SIZE) {
            too_big = FALSE;       
        } else {
            current_size /= 2;
            needed_level++;
        }
    }

    for (int i = needed_level; i >= 0; i--) {
        if (free_list_array[i] == NULL) {
            if (i == 0) {
                errno = ENOMEM;
                return NULL;
            }
            continue;
        } else {
            FreeListNode *current_node = (FreeListNode *)free_list_array[i];

            if (i == needed_level) {
                allocated_address = free_list_array[i];
                free_list_array[i] = (U32)current_node->next;
                binary_bit_array[(pow(2, i) - 1 + current_node->list_index) / 8] |= 1 << ((pow(2, i) - 1 + current_node->list_index) % 8);

                if (current_node->next != NULL) {
                    current_node->next->prev = NULL;
                }
            } else {
                for(size_t j = i; j < needed_level; j++) {
                    U32 working_index = pow(2, j) - 1 + current_node->list_index;
                    binary_bit_array[working_index/8] |= 1 << (working_index % 8);

                    U32 f_child_index = current_node->list_index*2 + 1;
                    U32 working_child_index = current_node->list_index*2;
                    binary_bit_array[(pow(2, j + 1) - 1 + working_child_index) / 8] |= 1 << ((pow(2, j + 1) - 1 + working_child_index) % 8);

                    FreeListNode *next_node = (FreeListNode *)needed_address(ram_start, j + 1, ram_pool_size, f_child_index);
                    next_node->list_index = f_child_index;
                    next_node->next = NULL;
                    next_node->prev = NULL;
                    free_list_array[j + 1] = (U32)next_node;

                    if (i == j) {
                        free_list_array[j] = (U32)current_node->next;
                    }

                    if (current_node->next != NULL) {
                        current_node->next->prev = NULL;
                    }
                    
                    if (j == needed_level - 1) {
                        allocated_address = needed_address(ram_start, j + 1, ram_pool_size, working_child_index);
                    } else {
                        current_node = (FreeListNode *)needed_address(ram_start, j + 1, ram_pool_size, working_child_index);
                        current_node->list_index = working_child_index;
                        current_node->prev = NULL;
                        current_node->next = NULL;
                    }

                }
            }

            break;
        }
    }
    
    return ((void *) allocated_address);
}

int k_mpool_dealloc(mpool_t mpid, void *ptr)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d, ptr = 0x%x\r\n", mpid, ptr);
#endif /* DEBUG_0 */

    
    if(mpid != MPID_IRAM1 && mpid != MPID_IRAM2){ // this is performed as mpid is invalid if not 1 or 0
        errno = EINVAL;
        return RTX_ERR; 
    }

    U32 *free_list_array;
    U8 *binary_bit_array;
    U32 k_lvls;
    U32 pointer = (U32) ptr;
    U32 ram_start;
    U32 ram_end;
    U32 free_address;
    U32 pool_size;

    if (mpid == MPID_IRAM1) {
        free_list_array = free_list_array_RAM1;
        binary_bit_array = binary_bit_array_RAM1;
        k_lvls = k_lvls_RAM1;
        ram_start = RAM1_START;
        ram_end = RAM1_END;
        pool_size = size_RAM1;
    } else {
        free_list_array = free_list_array_RAM2;
        binary_bit_array = binary_bit_array_RAM2;
        k_lvls = k_lvls_RAM2;
        ram_start = RAM2_START;
        ram_end = RAM2_END;
        pool_size = size_RAM2;
    }

    if(pointer > ram_end || pointer < ram_start){
        errno = EFAULT;
        return RTX_ERR;
    }//trying to check if ptr is pointing outside of ram_start and ram_end for ram1/ram2

    for (int i = k_lvls; i >= 0; i--) {
        U32 index_check = pow(2, i) + ((pointer - ram_start) / pow(2, (k_lvls - i) + MIN_BLK_SIZE_LOG2)) - 1;
        if (binary_bit_array[index_check/8] & (1 << (index_check % 8))) { //check if the bit of the corresponding address pointer in the bit array is 1. 
            // binary_bit_array[index_check/8] &= (bit_0_array[index_check % 8]);

            U32 list_index;

            // Breaking the law here, we're changing new_list_index inside of the function
            size_t coalesce_result_level = coalesce_binary_bit_array(&free_list_array, &binary_bit_array, index_check, i, &list_index);

            FreeListNode *temp_ref = (FreeListNode *)free_list_array[coalesce_result_level];
            while (temp_ref != NULL && temp_ref->list_index < list_index && temp_ref->next != NULL) {
                temp_ref = temp_ref->next;
            }

            free_address = needed_address(ram_start, coalesce_result_level, pool_size, list_index);
            if(free_list_array[coalesce_result_level] == NULL) { //only block in list
                FreeListNode *current_node = (FreeListNode *)free_address;
                free_list_array[coalesce_result_level] = (U32)current_node;
                current_node->next = NULL;
                current_node->prev = NULL;
                current_node->list_index = list_index;
            } else if(temp_ref->next == NULL && temp_ref->list_index < list_index) { //last block in list
                temp_ref->next = (FreeListNode *)free_address;
                temp_ref->next->prev = temp_ref;
                temp_ref->next->next = NULL;
                temp_ref->next->list_index = list_index;
            } else if(temp_ref == (FreeListNode *)free_list_array[coalesce_result_level]) { //first block in list
                temp_ref->prev = (FreeListNode *)free_address;
                temp_ref->prev->next = temp_ref;
                temp_ref->prev->prev = NULL;
                temp_ref->prev->list_index = list_index;
                free_list_array[coalesce_result_level] = free_address;
            } else { //one of the middle blocks in list
                FreeListNode* temp_prev = temp_ref->prev;
                temp_prev->next = (FreeListNode *)free_address;
                temp_ref->prev = (FreeListNode *)free_address;
                temp_ref->prev->prev = temp_prev;
                temp_ref->prev->next = temp_ref;
                temp_ref->prev->list_index = list_index;
            }

            break;
        } else {
            continue; // if not 1, continue
        }
    }

    return RTX_OK; 
}

size_t coalesce_binary_bit_array(U32 **free_list_array, U8 **passed_binary_bit_array, U32 index_check, U32 current_level, U32 *list_index) {
    U32 buddy_index = index_check;

    if (index_check % 2) {
        buddy_index++;        
    } else {
        buddy_index--;
    }
    
    (*passed_binary_bit_array)[index_check / 8] ^= 1 << (index_check % 8); //assign corresponding bit in the bit array to 0, indicating that it is going to be freed

    if (current_level > 0 && !((*passed_binary_bit_array)[buddy_index / 8] & (1 << (buddy_index % 8)))) {
        FreeListNode *node = (FreeListNode *)(*free_list_array)[current_level];
        while(node != NULL && node->list_index != buddy_index - pow(2, current_level) + 1) {
            node = node->next;
        }

        if (node != NULL) {
            if(node->prev == NULL && node->next == NULL) {//only element
                (*free_list_array)[current_level] = 0;
            } else if (node->prev == NULL) {//first element
                node->next->prev = NULL;
                (*free_list_array)[current_level] = (U32)node->next;
            } else if (node->next == NULL) {//last element
                node->prev->next = NULL;
            } else {//middle element
                node->prev->next = node->next;
                node->next->prev = node->prev;
            }
        }
        
        return coalesce_binary_bit_array(free_list_array, passed_binary_bit_array, (U32)(pow(2, current_level - 1)) - 1 + ((index_check - (U32)(pow(2, current_level)) + 1) / 2), current_level - 1, list_index);
    } else {
        *list_index = index_check - pow(2, current_level) + 1;
        return current_level;
    }
}

int k_mpool_dump (mpool_t mpid)
{
#ifdef DEBUG_0
    printf("k_mpool_dump: mpid = %d\r\n", mpid);
#endif /* DEBUG_0 */

    U32 *free_list_array;
    U32 k_lvls;
    U32 found_memory_count = 0;
    // U32 ram_start;
    U32 pool_size;

    if (mpid == MPID_IRAM1) {
        free_list_array = free_list_array_RAM1;
        // *binary_bit_array = binary_bit_array_RAM1;
        k_lvls = k_lvls_RAM1;
        // ram_start = RAM1_START;
        // ram_end = RAM1_END;
        pool_size = size_RAM1;
    } else if (mpid == MPID_IRAM2) {
        free_list_array = free_list_array_RAM2;
        // *binary_bit_array = binary_bit_array_RAM2;
        k_lvls = k_lvls_RAM2;
        // ram_start = RAM2_START;
        // ram_end = RAM2_END;
        pool_size = size_RAM2;
    } else {
        return 0;
    }
    
    for (int i = k_lvls; i >= 0; i--) {
        FreeListNode *temp = (FreeListNode *)free_list_array[i];
        while (temp != NULL) {
            printf("0x%08x: 0x%x\r\n", temp, pool_size / pow(2, i));

            found_memory_count++;
            temp = temp->next;
        }
    }

    printf("%d free memory block(s) found\r\n", found_memory_count);
    
    return found_memory_count;
}
 
int k_mem_init(int algo)
{
#ifdef DEBUG_0
    printf("k_mem_init: algo = %d\r\n", algo);
#endif /* DEBUG_0 */
        
    if ( k_mpool_create(algo, RAM1_START, RAM1_END) < 0 ) {
        return RTX_ERR;
    }
    
    if ( k_mpool_create(algo, RAM2_START, RAM2_END) < 0 ) {
        return RTX_ERR;
    }
    
    return RTX_OK;
}

/**
 * @brief allocate kernel stack statically
 */
U32* k_alloc_k_stack(task_t tid)
{
    
    if ( tid >= MAX_TASKS) {
        errno = EAGAIN;
        return NULL;
    }

    if(g_k_stacks[0] == NULL) {
        for(size_t i = 0; i < MAX_TASKS; i++) {
            g_k_stacks[i] = (U32 *)k_mpool_alloc(MPID_IRAM2, KERN_STACK_SIZE >> 2);
        }
    }

    U32 *sp = g_k_stacks[tid+1];
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

