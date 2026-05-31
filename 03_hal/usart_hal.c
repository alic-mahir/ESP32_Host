/* Includes **************************************************************** */
#include "usart_hal.h"
#include "drivers.h"

/* Private types *********************************************************** */

/* Private constants ******************************************************* */

/* Private macros ********************************************************** */

/* Private variables ******************************************************* */
USART_HAL g_USARTx_HAL = 
{
    .driver = 0,
    .init = 0,
    .tx_byte = 0,
    .rx_byte = 0,
    .tx_data = 0,
    .rx_data = 0,
    .tx_string = 0,
	.set_baudrate = 0,
};

// USART_HAL g_USART1_HAL = 
// {
//     .driver = (USART_DRIVER *) &g_USART1_DRIVER,
//     .init = usart1_init,
//     .tx_byte = usart1_tx_byte,
//     .rx_byte = usart1_rx_byte,
//     .tx_data = usart1_tx_data,
//     .rx_data = usart1_rx_data,
//     .tx_string = usart1_tx_string,
// 	.set_baudrate = usart1_set_baudrate,
// };

USART_HAL g_USART2_HAL = 
{
    .driver = (USART_DRIVER *) &g_USART2_DRIVER,
    .init = usart2_init,
    .tx_byte = usart2_tx_byte,
    .rx_byte = usart2_rx_byte,
    .tx_data = usart2_tx_data,
    .rx_data = usart2_rx_data,
    .tx_string = usart2_tx_string,
	.set_baudrate = usart2_set_baudrate,
};


/* Private function prototypes ********************************************* */

/* Exported functions ****************************************************** */

/* Private functions ******************************************************* */

/* ***************************** END OF FILE ******************************* */



