/* Define to prevent recursive inclusion *********************************** */
#ifndef __SPI_HOSTED_H
#define __SPI_HOSTED_H

/* Includes **************************************************************** */
#include "mcu.h"
#include "drivers.h"
#include "stm32f4xx_hal.h"

/* Module configuration **************************************************** */
#define SPI_HOSTED_TIMEOUT_MS                         100

/* Exported types ********************************************************** */
typedef struct spi_hosted_driver_t
{
    uint8_t state;
    SPI_TypeDef * instance;
    GPIO_TypeDef * cs_port;
    uint16_t cs_pin;
    GPIO_TypeDef * reset_port;
    uint16_t reset_pin;
    GPIO_TypeDef * hs_port;
    uint16_t hs_pin;
    GPIO_TypeDef * drdy_port;
    uint16_t drdy_pin;
} SPI_HOSTED_DRIVER;

/* Exported variables ****************************************************** */
extern SPI_HOSTED_DRIVER g_SPI_HOSTED_DRIVER;

/* Exported functions ****************************************************** */
int8_t spi_hosted_init(void);
int8_t spi_hosted_deinit(void);
int8_t spi_hosted_transfer(uint8_t * tx_data, uint8_t * rx_data, uint16_t size, uint32_t timeout_ms);
int8_t spi_hosted_reset_slave(void);

#endif
