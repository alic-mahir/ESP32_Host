/**
 * @file        mcu.h
 * @author      Semir Tursunovic (semir-t) 
 * @date        Januar 2023
 * @version     1.0.0
 */

/* Define to prevent recursive inclusion *********************************** */
#ifndef __MCU_H
#define __MCU_H

/* Includes **************************************************************** */
#include "stm32f4xx.h"

/* Exported constants ****************************************************** */
#define MCU_UID_SIZE                                 12
#define MCU_UID_ADDR                                 UID_BASE


/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */
typedef struct mcu_module_t
{
    uint8_t uid[MCU_UID_SIZE];
} MCU_INFO;

/* Exported variables ****************************************************** */
extern volatile MCU_INFO g_MCU_INFO;

/* Exported functions ****************************************************** */
void mcu_init(void);
void sysclock_init(void);


#endif 
/* ***************************** END OF FILE ******************************* */

