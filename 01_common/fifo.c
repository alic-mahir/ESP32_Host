/* Includes **************************************************************** */
#include "fifo.h"
#include "debug.h"
#include <string.h>

/* Private types *********************************************************** */

/* Private constants ******************************************************* */

/* Private macros ********************************************************** */

/* Private variables ******************************************************* */

/* Private function prototypes ********************************************* */

/* Exported functions ****************************************************** */

void fifo_init(FIFO * this,uint8_t type_size, void * buffer, uint32_t buffer_size)
{
    this->buffer = buffer;
    this->size = buffer_size;
    this->type_size = type_size ;
    this->widx = 0;
    this->ridx = 0;
}

void fifo_push(FIFO * this, void * data) 
{
    if (!fifo_is_full(this)) 
    {
        // debug_log(DNONE,"[%d][%d][%d]\n",*((uint8_t *)data),this->widx * this->type_size,this->type_size);
        memcpy((void *)((uint8_t *)this->buffer + (this->widx * this->type_size)), data, this->type_size);
        this->widx++;
        if(this->widx >= this->size)
        {
            this->widx = 0;
        }
    } 
    else 
    {

    }
}

void fifo_pop(FIFO * this, void * data) 
{
    if (!fifo_is_empty(this)) 
    {
        memcpy(data, (const void *)((uint8_t *)this->buffer + (this->ridx * this->type_size)), this->type_size);
        this->ridx++;
        if(this->ridx >= this->size)
        {
            this->ridx = 0;
        }

    }
}

void fifo_at(FIFO * this,uint32_t index, void * data) 
{
    if (!fifo_is_empty(this)) 
    {
        memcpy(data, (const void *)((uint8_t *)this->buffer + (index * this->type_size)), this->type_size);
    }
}


void fifo_flush(FIFO * this) 
{
    this->widx = 0;
    this->ridx = 0;
    return ;
}

uint32_t fifo_data_available(FIFO * this)
{
    if(this->widx >= this->ridx)
    {
        return (this->widx - this->ridx);
    }
    else
    {
        return (this->size - this->ridx + this->widx );
    }
}

uint32_t fifo_is_full(FIFO * this) 
{
    uint32_t size = fifo_data_available(this);
    if(size == this->size)
    {
        return 1;
    }
    return 0;
}

uint32_t fifo_is_empty(FIFO * this) 
{
    return this->widx == this->ridx;
}

uint8_t fifo_inspect_byte(FIFO * this,uint16_t idx)
{
    uint16_t ridx = (uint16_t)(this->ridx + idx);
    if (ridx >= this->size) {
        ridx = (uint16_t)(ridx % this->size);
    }
    return ((volatile uint8_t *)this->buffer)[ridx];
}

uint8_t fifo_scan_for_char(FIFO * this,uint8_t c)
{
    uint8_t flag = FIFO_SCAN_STATUS_CHAR_NOT_FOUND;
    uint16_t ridx = this->ridx;
    while(ridx != this->widx)
    {
        // uint8_t data = this->buffer[ridx++];
        uint8_t data = 0;
        if(c == data)
        {
            flag = FIFO_SCAN_STATUS_CHAR_FOUND;
            break;
        }
        if(ridx >= this->size)
        {
            ridx = 0x00;
        }
    }
    return flag;
}



/* Private functions ******************************************************* */

/* ***************************** END OF FILE ******************************* */

