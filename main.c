#include "mcu.h"
#include "delay.h"
#include "hal.h"
#include "debug.h"
#include "drivers.h"
#include "usb.h"
#include "wifi_hosted.h"

static void log_esp_slave_pinout(void)
{
	debug_log(DHEADER, "ESP slave pinout required:\n");
	debug_log(DAPPEND, "  SPI1_SCK   -> PA5\n");
	debug_log(DAPPEND, "  SPI1_MISO  -> PA6\n");
	debug_log(DAPPEND, "  SPI1_MOSI  -> PA7\n");
	debug_log(DAPPEND, "  SPI1_CS    -> PA4\n");
	debug_log(DAPPEND, "  ESP_RESET  -> PB0\n");
	debug_log(DAPPEND, "  ESP_HS     -> PB1  (handshake)\n");
	debug_log(DAPPEND, "  ESP_DRDY   -> PB2  (data ready)\n");
	debug_log(DAPPEND, "  GND        -> GND\n");
	debug_log(DAPPEND, "  VCC        -> 3V3\n");
	debug_log(DAPPEND, "HS/DRDY handshake + hosted init framing enabled.\n");



	// EPS pinout 
	// MOSI - 7 
	// MISO - 2 s
	// CLK - 6
	// CS - 8 
	// HS - 22
	// DR = 23 
}

int main(void)
{
	mcu_init();
	systim_init();
	initUSB();

	delay_ms(1000);
	debug_init(&g_USART2_HAL, 921600, "stm32f407 esp-hosted baremetal");
	debug_log(DNONE, "Init completed\n");
	log_esp_slave_pinout();

	if (esp_hosted_connect_to_slave() != DRIVER_STATUS_OK) {
		debug_log(DNONE, "esp_hosted_connect_to_slave failed\n");
	} else {
		esp_hosted_coprocessor_fwver_t fwver;
		if (esp_hosted_get_coprocessor_fwversion(&fwver) == DRIVER_STATUS_OK) {
			debug_log(DNONE, "Slave FW Version: %d.%d.%d\n",
					  fwver.major1, fwver.minor1, fwver.patch1);
		} else {
			debug_log(DWARNING, "failed to get slave fw version\n");
		}
	}

	// init bt controller
    if (esp_hosted_bt_controller_init() != DRIVER_STATUS_OK) {
        debug_log(DNONE, "failed to init bt controller\n");
    }


	// enable bt controller
    if (esp_hosted_bt_controller_enable() != DRIVER_STATUS_OK) {
        debug_log(DNONE, "failed to enable bt controller\n");
    }

	if(esp_hosted_ble_start_advertising("esp32-BLE") != DRIVER_STATUS_OK)
	{
		debug_log(DNONE,"failed to start advertising\n");
	}

	
	// if (wifi_init() != DRIVER_STATUS_OK) {
	// 	debug_log(DNONE, "wifi_init failed\n");
	// }

	while (1)
	{
		// wifi_task();
		delay_ms(10);
	}

	return 0;
}
