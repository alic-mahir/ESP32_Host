/**
* @file        delay.h
* @author      semir-t
* @date        Januar 2023
* @version     1.0.0
*/

/* Define to prevent recursive inclusion *********************************** */
#ifndef __DELAY_H
#define __DELAY_H
/* Includes **************************************************************** */
#include "mcu.h"

/* Module configuration **************************************************** */

/* Exported constants ****************************************************** */

enum SYSTIM
{
    SYSTIM_TIMEOUT = 0x00,
    SYSTIM_ALIVE,
};

/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */

/* Exported variables ****************************************************** */

/* Exported functions ****************************************************** */
void delay_ms(uint32_t delay);
void delay_us(uint32_t delay);

void systim_init(void);
uint32_t systim_get_tick(void);
uint8_t systim_has_time_elapsed(uint32_t b_time, uint32_t period);
uint32_t systim_get_elapsed_time(uint32_t b_time);



#endif 

