/**
* @file        fifo.h
* @author      semir-t
* @date        Januar 2023
* @version     1.0.1
*/

/* Define to prevent recursive inclusion *********************************** */
#ifndef __FIFO_H
#define __FIFO_H
/* Includes **************************************************************** */
#include "stdint.h"

/* Module configuration **************************************************** */


/* Exported constants ****************************************************** */
enum FIFO_TYPE
{
    FIFO_TYPE_CHAR,
    FIFO_TYPE_UINT8_T,
    FIFO_TYPE_UINT16_T,
    FIFO_TYPE_UINT32_T,
    FIFO_TYPE_INT8_T = 0x00,
    FIFO_TYPE_INT16_T,
    FIFO_TYPE_INT32_T,
    FIFO_TYPE_FLOAT,
    FIFO_TYPE_CUSTOM,
};


enum FIFO_SCAN_STATUS
{
    FIFO_SCAN_STATUS_CHAR_NOT_FOUND = 0x00,
    FIFO_SCAN_STATUS_CHAR_FOUND,
};


/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */
typedef struct fifo_base_t
{
    volatile void * buffer;
    volatile uint16_t widx;
    volatile uint16_t ridx;
    uint32_t size;
    uint32_t type_size;
} FIFO;


/* Exported variables ****************************************************** */

/* Exported functions ****************************************************** */
void fifo_init(FIFO * this,uint8_t type_size, void * buffer, uint32_t buffer_size);
void fifo_push(FIFO * this,void * data);
void fifo_pop(FIFO * this, void * data);
void fifo_at(FIFO * this,uint32_t index, void * data);
void fifo_flush(FIFO * this);
uint32_t fifo_data_available(FIFO * this);
uint32_t fifo_is_full(FIFO * this);
uint32_t fifo_is_empty(FIFO * this);


#endif 

