/* Define to prevent recursive inclusion *********************************** */
#ifndef __SPI_HOSTED_HAL_H
#define __SPI_HOSTED_HAL_H

/* Includes **************************************************************** */
#include "spi_hosted.h"

/* Exported types ********************************************************** */
typedef struct spi_hosted_hal_t
{
    SPI_HOSTED_DRIVER * driver;
    int8_t (* init)(void);
    int8_t (* deinit)(void);
    int8_t (* transfer)(uint8_t * tx_data, uint8_t * rx_data, uint16_t size, uint32_t timeout_ms);
    int8_t (* reset_slave)(void);
} SPI_HOSTED_HAL;

/* Exported variables ****************************************************** */
extern SPI_HOSTED_HAL g_SPI_HOSTED_HAL;

#endif
