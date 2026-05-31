/* Define to prevent recursive inclusion *********************************** */
#ifndef __USART_H
#define __USART_H
/* Includes **************************************************************** */
#include <stdarg.h>
#include "mcu.h"
#include "fifo.h"


/* Module configuration **************************************************** */
// #define usart_log(type,...)							debug_print(type,"USART",__VA_ARGS__)
#define usart_log(type,...)	

#define USARTx_RX_BUFFER_SIZE                           256
#define USART1_RX_BUFFER_SIZE                           256
#define USART2_RX_BUFFER_SIZE                           256
/* Exported constants ****************************************************** */

/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */
typedef struct usart_driver_t
{
    uint8_t state;
    FIFO rx_buffer;
} USART_DRIVER;

/* Exported variables ****************************************************** */
extern volatile USART_DRIVER g_USARTx_DRIVER;
extern USART_DRIVER g_USART1_DRIVER;
extern USART_DRIVER g_USART2_DRIVER;

/* Exported functions ****************************************************** */
int8_t usartx_init(uint32_t baudrate);
int8_t usartx_deinit(void);
int8_t usartx_tx_byte(uint8_t data, uint32_t timeout);
int8_t usartx_rx_byte(uint8_t * data, uint32_t timeout);
int8_t usartx_tx_data(uint8_t * data, uint32_t size, uint32_t timeout);
int8_t usartx_rx_data(uint8_t * data, uint32_t size, uint32_t timeout);
int8_t usartx_tx_string(char * str, uint32_t timeout);
int8_t usartx_set_baudrate(uint32_t baudrate);

int8_t usart1_init(uint32_t baudrate);
int8_t usart1_deinit(void);
int8_t usart1_tx_byte(uint8_t data, uint32_t timeout);
int8_t usart1_rx_byte(uint8_t * data, uint32_t timeout);
int8_t usart1_tx_data(uint8_t * data, uint32_t size, uint32_t timeout);
int8_t usart1_rx_data(uint8_t * data, uint32_t size, uint32_t timeout);
int8_t usart1_tx_string(char * str, uint32_t timeout);
int8_t usart1_set_baudrate(uint32_t baudrate);

int8_t usart2_init(uint32_t baudrate);
int8_t usart2_deinit(void);
int8_t usart2_tx_byte(uint8_t data, uint32_t timeout);
int8_t usart2_rx_byte(uint8_t * data, uint32_t timeout);
int8_t usart2_tx_data(uint8_t * data, uint32_t size, uint32_t timeout);
int8_t usart2_rx_data(uint8_t * data, uint32_t size, uint32_t timeout);
int8_t usart2_tx_string(char * str, uint32_t timeout);
int8_t usart2_set_baudrate(uint32_t baudrate);



#endif 


