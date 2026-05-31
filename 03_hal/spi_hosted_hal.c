/* Includes **************************************************************** */
#include "spi_hosted_hal.h"

/* Exported variables ****************************************************** */
SPI_HOSTED_HAL g_SPI_HOSTED_HAL =
{
    .driver = &g_SPI_HOSTED_DRIVER,
    .init = spi_hosted_init,
    .deinit = spi_hosted_deinit,
    .transfer = spi_hosted_transfer,
    .reset_slave = spi_hosted_reset_slave,
};
