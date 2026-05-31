# STM32F407 ESP-Hosted Bare-Metal Skeleton

This project was cloned from your `mbdesig-usb-debug` structure and extended with:
- `02_drivers/spi_hosted.*`
- `03_hal/spi_hosted_hal.*`
- `05_application/wifi_hosted.*`
- `main.c` calls `wifi_init()` and `wifi_task()`.

## Pin mapping used in current code
- SPI1 SCK: `PA5`
- SPI1 MISO: `PA6`
- SPI1 MOSI: `PA7`
- SPI CS: `PA4`
- Slave reset: `PB0`

## Important note
Current `wifi_hosted.c` is a transport/state-machine skeleton with a lightweight command frame.
For a real ESP-Hosted slave session, the command payload must follow ESP-Hosted MCU RPC/protobuf framing.

## Where to edit credentials
- `05_application/wifi_hosted.h`
  - `WIFI_HOSTED_SSID`
  - `WIFI_HOSTED_PASSWORD`
