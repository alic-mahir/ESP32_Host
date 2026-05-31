/* Includes **************************************************************** */
#include "spi_hosted.h"
#include "debug.h"

/* Private macros ********************************************************** */
#define SPI_HOSTED                                    SPI1
#define SPI_HOSTED_CLK_ENABLE()                       __HAL_RCC_SPI1_CLK_ENABLE()
#define SPI_HOSTED_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI_HOSTED_RESET_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOB_CLK_ENABLE()

#define SPI_HOSTED_SCK_PIN                            GPIO_PIN_5
#define SPI_HOSTED_MISO_PIN                           GPIO_PIN_6
#define SPI_HOSTED_MOSI_PIN                           GPIO_PIN_7
#define SPI_HOSTED_GPIO_PORT                          GPIOA
#define SPI_HOSTED_AF                                 GPIO_AF5_SPI1

#define SPI_HOSTED_CS_PIN                             GPIO_PIN_4
#define SPI_HOSTED_CS_GPIO_PORT                       GPIOA

#define SPI_HOSTED_RESET_PIN                          GPIO_PIN_0
#define SPI_HOSTED_RESET_GPIO_PORT                    GPIOB

#define SPI_HOSTED_HS_PIN                             GPIO_PIN_1
#define SPI_HOSTED_HS_GPIO_PORT                       GPIOB

#define SPI_HOSTED_DRDY_PIN                           GPIO_PIN_2
#define SPI_HOSTED_DRDY_GPIO_PORT                     GPIOB

#ifndef SPI_HOSTED_LOG_RAW_TX
#define SPI_HOSTED_LOG_RAW_TX                         0
#endif

#ifndef SPI_HOSTED_LOG_RAW_RX
#define SPI_HOSTED_LOG_RAW_RX                         0
#endif

/* Private functions ******************************************************* */
static int8_t spi_hosted_wait_drdy_level(GPIO_PinState level, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while (HAL_GPIO_ReadPin(g_SPI_HOSTED_DRIVER.drdy_port, g_SPI_HOSTED_DRIVER.drdy_pin) != level) {
        if ((HAL_GetTick() - start) >= timeout_ms) {
            return DRIVER_STATUS_TIMEOUT;
        }
    }

    return DRIVER_STATUS_OK;
}

static int8_t spi_hosted_wait_hs_level(GPIO_PinState level, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while (HAL_GPIO_ReadPin(g_SPI_HOSTED_DRIVER.hs_port, g_SPI_HOSTED_DRIVER.hs_pin) != level) {
        if ((HAL_GetTick() - start) >= timeout_ms) {
            return DRIVER_STATUS_TIMEOUT;
        }
    }

    return DRIVER_STATUS_OK;
}

static void spi_hosted_log_tx_raw(const uint8_t *tx_data, uint16_t size)
{
#if SPI_HOSTED_LOG_RAW_TX
    char line[72];
    uint16_t i = 0;

    if (size == 0U) {
        debug_log(DAPPEND, "SPI TX RAW [0]: <empty>\n");
        return;
    }

    debug_log(DAPPEND, "SPI TX RAW [%u]:\n", (unsigned int)size);

    while (i < size) {
        int written = snprintf(line, sizeof(line), "  %04u:", (unsigned int)i);
        uint16_t j = 0;

        while ((j < 16U) && ((i + j) < size) && (written > 0) && (written < (int)sizeof(line))) {
            written += snprintf(&line[written], sizeof(line) - (size_t)written, " %02X",
                                tx_data ? tx_data[i + j] : 0xFFU);
            j++;
        }

        debug_log(DAPPEND, "%s\n", line);
        i = (uint16_t)(i + 16U);
    }
#else
    (void)tx_data;
    (void)size;
#endif
}

static void spi_hosted_log_rx_raw(const uint8_t *rx_data, uint16_t size)
{
#if SPI_HOSTED_LOG_RAW_RX
    char line[72];
    uint16_t i = 0;

    if (rx_data == NULL) {
        debug_log(DAPPEND, "SPI RX RAW: <not captured>\n");
        return;
    }

    if (size == 0U) {
        debug_log(DAPPEND, "SPI RX RAW [0]: <empty>\n");
        return;
    }

    debug_log(DAPPEND, "SPI RX RAW [%u]:\n", (unsigned int)size);

    while (i < size) {
        int written = snprintf(line, sizeof(line), "  %04u:", (unsigned int)i);
        uint16_t j = 0;

        while ((j < 16U) && ((i + j) < size) && (written > 0) && (written < (int)sizeof(line))) {
            written += snprintf(&line[written], sizeof(line) - (size_t)written, " %02X",
                                rx_data[i + j]);
            j++;
        }

        debug_log(DAPPEND, "%s\n", line);
        i = (uint16_t)(i + 16U);
    }
#else
    (void)rx_data;
    (void)size;
#endif
}

static void spi_hosted_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    SPI_HOSTED_GPIO_CLK_ENABLE();
    SPI_HOSTED_RESET_GPIO_CLK_ENABLE();

    gpio.Pin = SPI_HOSTED_SCK_PIN | SPI_HOSTED_MISO_PIN | SPI_HOSTED_MOSI_PIN;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = SPI_HOSTED_AF;
    HAL_GPIO_Init(SPI_HOSTED_GPIO_PORT, &gpio);

    gpio.Pin = SPI_HOSTED_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SPI_HOSTED_CS_GPIO_PORT, &gpio);

    gpio.Pin = SPI_HOSTED_RESET_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SPI_HOSTED_RESET_GPIO_PORT, &gpio);

    gpio.Pin = SPI_HOSTED_HS_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SPI_HOSTED_HS_GPIO_PORT, &gpio);

    gpio.Pin = SPI_HOSTED_DRDY_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SPI_HOSTED_DRDY_GPIO_PORT, &gpio);

    HAL_GPIO_WritePin(SPI_HOSTED_CS_GPIO_PORT, SPI_HOSTED_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SPI_HOSTED_RESET_GPIO_PORT, SPI_HOSTED_RESET_PIN, GPIO_PIN_SET);
}

/* Exported variables ****************************************************** */
SPI_HOSTED_DRIVER g_SPI_HOSTED_DRIVER = {
    .state = DRIVER_STATE_DEINITALIZED,
    .instance = SPI_HOSTED,
    .cs_port = SPI_HOSTED_CS_GPIO_PORT,
    .cs_pin = SPI_HOSTED_CS_PIN,
    .reset_port = SPI_HOSTED_RESET_GPIO_PORT,
    .reset_pin = SPI_HOSTED_RESET_PIN,
    .hs_port = SPI_HOSTED_HS_GPIO_PORT,
    .hs_pin = SPI_HOSTED_HS_PIN,
    .drdy_port = SPI_HOSTED_DRDY_GPIO_PORT,
    .drdy_pin = SPI_HOSTED_DRDY_PIN,
};

/* Exported functions ****************************************************** */
int8_t spi_hosted_init(void)
{
    uint32_t cr1 = 0;

    SPI_HOSTED_CLK_ENABLE();
    spi_hosted_gpio_init();

    g_SPI_HOSTED_DRIVER.instance->CR1 &= ~SPI_CR1_SPE;
    g_SPI_HOSTED_DRIVER.instance->CR2 = 0;

    cr1 |= SPI_CR1_MSTR;                /* Master mode */
    cr1 |= SPI_CR1_CPOL | SPI_CR1_CPHA; /* SPI mode 3 (CPOL=1, CPHA=1) */
    cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;   /* Software NSS management */
    cr1 |= SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0; /* fPCLK / 128 ~= 656 kHz (PCLK2=84 MHz) */
    g_SPI_HOSTED_DRIVER.instance->CR1 = cr1;

    g_SPI_HOSTED_DRIVER.instance->CR1 |= SPI_CR1_SPE;

    g_SPI_HOSTED_DRIVER.state = DRIVER_STATE_INITIALIZED;
    return DRIVER_STATUS_OK;
}

int8_t spi_hosted_deinit(void)
{
    g_SPI_HOSTED_DRIVER.instance->CR1 &= ~SPI_CR1_SPE;
    g_SPI_HOSTED_DRIVER.state = DRIVER_STATE_DEINITALIZED;
    return DRIVER_STATUS_OK;
}

int8_t spi_hosted_transfer(uint8_t * tx_data, uint8_t * rx_data, uint16_t size, uint32_t timeout_ms)
{
    uint16_t i = 0;
    uint32_t start = HAL_GetTick();
    uint8_t wait_for_drdy = 1U;

    if (g_SPI_HOSTED_DRIVER.state != DRIVER_STATE_INITIALIZED) {
        return DRIVER_STATUS_ERROR;
    }

    if (size == 0U) {
        return DRIVER_STATUS_OK;
    }

    /* For hosted dummy polls (if_type == ESP_MAX_IF), slave must indicate data by DRDY.
     * For host-originated command/data frames, allow TX when HS is active even if DRDY is low. */
    if ((tx_data != 0) && (size >= 1U)) {
        if ((tx_data[0] & 0x0FU) != 0x08U) {
            wait_for_drdy = 0U;
        }
    }

    spi_hosted_log_tx_raw(tx_data, size);

    if (spi_hosted_wait_hs_level(GPIO_PIN_SET, timeout_ms) != DRIVER_STATUS_OK) {
        debug_log(DWARNING, "SPI timeout waiting HS active\n");
        return DRIVER_STATUS_TIMEOUT;
    }

    if (wait_for_drdy != 0U) {
        if (spi_hosted_wait_drdy_level(GPIO_PIN_SET, timeout_ms) != DRIVER_STATUS_OK) {
            debug_log(DWARNING, "SPI HS/DRDY timeout waiting DRDY high\n");
            return DRIVER_STATUS_TIMEOUT;
        }
    }

    HAL_GPIO_WritePin(g_SPI_HOSTED_DRIVER.cs_port, g_SPI_HOSTED_DRIVER.cs_pin, GPIO_PIN_RESET);

    for (i = 0; i < size; i++) {
        while ((g_SPI_HOSTED_DRIVER.instance->SR & SPI_SR_TXE) == 0U) {
            if ((HAL_GetTick() - start) >= timeout_ms) {
                HAL_GPIO_WritePin(g_SPI_HOSTED_DRIVER.cs_port, g_SPI_HOSTED_DRIVER.cs_pin, GPIO_PIN_SET);
                return DRIVER_STATUS_TIMEOUT;
            }
        }

        *(__IO uint8_t *)&g_SPI_HOSTED_DRIVER.instance->DR = tx_data ? tx_data[i] : 0xFFU;

        while ((g_SPI_HOSTED_DRIVER.instance->SR & SPI_SR_RXNE) == 0U) {
            if ((HAL_GetTick() - start) >= timeout_ms) {
                HAL_GPIO_WritePin(g_SPI_HOSTED_DRIVER.cs_port, g_SPI_HOSTED_DRIVER.cs_pin, GPIO_PIN_SET);
                return DRIVER_STATUS_TIMEOUT;
            }
        }

        if (rx_data) {
            rx_data[i] = *(__IO uint8_t *)&g_SPI_HOSTED_DRIVER.instance->DR;
        } else {
            (void)*(__IO uint8_t *)&g_SPI_HOSTED_DRIVER.instance->DR;
        }
    }

    HAL_GPIO_WritePin(g_SPI_HOSTED_DRIVER.cs_port, g_SPI_HOSTED_DRIVER.cs_pin, GPIO_PIN_SET);

    spi_hosted_log_rx_raw(rx_data, size);

    return DRIVER_STATUS_OK;
}

int8_t spi_hosted_reset_slave(void)
{
    HAL_GPIO_WritePin(g_SPI_HOSTED_DRIVER.reset_port, g_SPI_HOSTED_DRIVER.reset_pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(g_SPI_HOSTED_DRIVER.reset_port, g_SPI_HOSTED_DRIVER.reset_pin, GPIO_PIN_SET);
    HAL_Delay(200);
    return DRIVER_STATUS_OK;
}
