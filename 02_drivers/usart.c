/* Includes **************************************************************** */
#include "drivers.h"
#include "usart.h"
#include "debug.h"

/* Private types *********************************************************** */

/* Private constants ******************************************************* */

/* Private macros ********************************************************** */

/* Private variables ******************************************************* */
volatile uint8_t g_usartx_rx_buffer[USARTx_RX_BUFFER_SIZE];
volatile USART_DRIVER g_USARTx_DRIVER = 
{
    .state = DRIVER_STATE_DEINITALIZED,
    .rx_buffer = 
    {
        .buffer = (uint8_t *) g_usartx_rx_buffer,
        .size = USARTx_RX_BUFFER_SIZE,
        .widx = 0,
        .ridx = 0
    }
};

volatile uint8_t g_usart1_rx_buffer[USART1_RX_BUFFER_SIZE];
USART_DRIVER g_USART1_DRIVER; 

volatile uint8_t g_usart2_rx_buffer[USART2_RX_BUFFER_SIZE];
USART_DRIVER g_USART2_DRIVER; 



/* Private function prototypes ********************************************* */

/* Exported functions ****************************************************** */
int8_t usartx_init(uint32_t baudrate)
{
#ifdef PORT

	return DRIVER_STATUS_OK;
#endif
    (void)baudrate;
    return DRIVER_STATUS_ERROR;
}
int8_t usartx_deinit(void)
{
#ifdef PORT

	return DRIVER_STATUS_OK;
#endif
    return DRIVER_STATUS_ERROR;
}
int8_t usartx_tx_byte(uint8_t data, uint32_t timeout)
{
#ifdef PORT
	return DRIVER_STATUS_OK;
#endif
    (void)data;
    (void)timeout;
    return DRIVER_STATUS_ERROR;
} 
int8_t usartx_rx_byte(uint8_t * data, uint32_t timeout)
{
#ifdef PORT
	return DRIVER_STATUS_OK;
#endif
    (void)data;
    (void)timeout;
    return DRIVER_STATUS_ERROR;
}
int8_t usartx_tx_data(uint8_t * data, uint32_t size, uint32_t timeout)
{
#ifdef PORT

	return DRIVER_STATUS_OK;
#endif
    (void)data;
    (void)size;
    (void)timeout;
    return DRIVER_STATUS_ERROR;
}
int8_t usartx_rx_data(uint8_t * data, uint32_t size, uint32_t timeout)
{
#ifdef PORT

	return DRIVER_STATUS_OK;
#endif
    (void)data;
    (void)size;
    (void)timeout;
    return DRIVER_STATUS_ERROR;
}
int8_t usartx_tx_string(char * str, uint32_t timeout)
{
#ifdef PORT

	return DRIVER_STATUS_OK;
#endif
    (void)str;
    (void)timeout;
    return DRIVER_STATUS_ERROR;
}
int8_t usartx_set_baudrate(uint32_t baudrate)
{
#ifdef PORT
	return DRIVER_STATUS_OK;
#endif
    (void)baudrate;
    return DRIVER_STATUS_ERROR;
}

int8_t usart2_init(uint32_t baudrate)
{
    fifo_init((FIFO *)&g_USARTx_DRIVER.rx_buffer, sizeof(uint8_t), (void *)g_usart1_rx_buffer, USART2_RX_BUFFER_SIZE);
	
    //wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// USART2: PA2 -> TX & PA3 -> RX
	//------------------------------------------------------------------ 
	
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; 									
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN; 									
	GPIOA->MODER |= (GPIO_MODER_MODER2_1)|(GPIO_MODER_MODER3_1); 		
	GPIOA->AFR[0] |= 0x00007700;										
	
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR2_1;							  
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR3_1;							  
															
	__DMB();
	uint32_t baudrate_reg = 0x00000271;
	switch(baudrate)
	{
		case(921600):
		{
			baudrate_reg = 0x0000002D;
			break;
		}
	}

	USART2->BRR = baudrate_reg;
    USART2->CR1 = (USART_CR1_RE) | (USART_CR1_TE);
    USART2->CR1 |= (USART_CR1_UE);

    return DRIVER_STATUS_OK;
}
int8_t usart2_deinit(void)
{

	return DRIVER_STATUS_OK;
}
int8_t usart2_tx_byte(uint8_t data, uint32_t timeout)
{
	uint32_t k;
    uint32_t status;
    uint32_t flag = DRIVER_STATUS_TIMEOUT;
    for(k=0;k<(timeout);k++)
    {
        status = USART2->SR;
        if(status & USART_SR_TC)
        {
            flag = DRIVER_STATUS_OK;
            USART2->DR = data;
            break;
        }
    }

    return flag;
} 

int8_t usart2_rx_byte(uint8_t * data, uint32_t timeout)
{
	return DRIVER_STATUS_OK;
}
int8_t usart2_tx_data(uint8_t * data, uint32_t size, uint32_t timeout)
{
    for(uint32_t k = 0; k<size; k++)
    {
        usart2_tx_byte(data[k],timeout);
    }
	return DRIVER_STATUS_OK;
}
int8_t usart2_rx_data(uint8_t * data, uint32_t size, uint32_t timeout)
{
#ifdef PORT

	return DRIVER_STATUS_OK;
#endif
    (void)data;
    (void)size;
    (void)timeout;
    return DRIVER_STATUS_ERROR;
}
int8_t usart2_tx_string(char * str, uint32_t timeout)
{
	uint16_t k = 0;
	while (str[k] != '\0')
    {
        usart2_tx_byte(str[k],timeout);
        if (str[k] == '\n')
        {
            usart2_tx_byte('\r',timeout);
        }
        k++;
    }
    return DRIVER_STATUS_OK;
}
int8_t usart2_set_baudrate(uint32_t baudrate)
{
	return DRIVER_STATUS_OK;
}

void USART2_IRQHandler(void)
{
#ifdef PORT
    if(USART2->SR & USART_SR_RXNE)
    {   
        uint8_t data = USART1->DR;
        fifo_push(&g_USART1_DRIVER.rx_buffer,data);
        USART2->SR &= ~(USART_SR_RXNE); 
    }
#endif
    USART2->SR = 0x00000000;
}




/* Private functions ******************************************************* */

/* ***************************** END OF FILE ******************************* */

