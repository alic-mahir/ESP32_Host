#ifndef __USB_H__
#define __USB_H__

#include <stdint.h>
#include <stdarg.h>
#include "mcu.h"
#include "debug.h"
#include "delay.h"

#include "usb_device.h"

#include "stm32f4xx_hal.h"

#define debugUSB(type, ...)                 debug_print(type,"USB",__VA_ARGS__);

#define USB_FS_MAX_PKT_SIZE                         64;

typedef struct
{
	uint8_t * name;					// points to null terminated 4 byte module name
	uint8_t * rx_buff;				
	uint16_t rx_buff_size;
	uint16_t rx_widx;
	uint16_t rx_ridx;
	
} IFACE_DESC;


typedef struct 
{
    uint8_t state;
    uint8_t status;    
    uint32_t timer;
    IFACE_DESC interface;


} USB_INFO;

extern volatile USB_INFO g_USB_INFO;

void initUSB(void);
void chkUSB(void);
void txUSB(uint8_t * data, uint16_t size);
void usb_tx_data(uint8_t * data, uint32_t size);

#endif
