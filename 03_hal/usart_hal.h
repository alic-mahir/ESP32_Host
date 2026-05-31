/**
* @file        usart_hal.h
* @author      semir-t 
* @date        January 2023
* @version     1.0.0
*/

/* Define to prevent recursive inclusion *********************************** */
#ifndef __UART_HAL_H
#define __UART_HAL_H

/* Includes **************************************************************** */
#include "mcu.h"
#include "usart.h"
#include "drivers.h"

/* Module configuration **************************************************** */

/* Exported constants ****************************************************** */

/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */
typedef struct usart_hal_t
{
    USART_DRIVER * driver;
    int8_t (* init)(uint32_t baudrate);
    int8_t (* deinit)(void);
    int8_t (* tx_byte)(uint8_t data,uint32_t timeout);
    int8_t (* rx_byte)(uint8_t * data,uint32_t timeout);
    int8_t (* tx_data)(uint8_t * data, uint32_t size,uint32_t timeout);
    int8_t (* rx_data)(uint8_t * data, uint32_t size,uint32_t timeout);
    int8_t (* tx_string)(char * data,uint32_t timeout);
	int8_t (* set_baudrate)(uint32_t baudrate);
    uint32_t baudrate;
} USART_HAL;

/* Exported variables ****************************************************** */

extern USART_HAL g_USARTx_HAL;
extern USART_HAL g_USART1_HAL;
extern USART_HAL g_USART2_HAL;

/* Exported functions ****************************************************** */

#endif 

