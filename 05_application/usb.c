#include "usb.h"
#include "usb_device.h"
#include "debug.h"
#include "usbd_cdc_if.h"


#define USB_INTERFACE_RX_BUFF_SIZE					1024
#define USB_CMD_PAYLOAD_BUFF_SIZE					128	

const char c_module_name[] = "USB";

char *data = "Test USB\n\r";

volatile uint8_t g_usb_interface_rx_buff[USB_INTERFACE_RX_BUFF_SIZE]; 

volatile USB_INFO g_USB_INFO;
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

void usb_tx_data(uint8_t * data,uint32_t size);


// void Error_Handler(void)
// {
// 	__disable_irq();
// 	debugUSB(DNONE, "error handler\n");
// 	while (1)
// 	{    
// 	}
// }

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

void SysTick_Handler(void)
{
	HAL_IncTick();
}

void initUSB(void)
{
    HAL_Init();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE(); 
    MX_USB_DEVICE_Init();
	
	g_USB_INFO.interface.name = (uint8_t *)c_module_name;
	g_USB_INFO.interface.rx_buff = (uint8_t *)g_usb_interface_rx_buff;
	g_USB_INFO.interface.rx_buff_size = (USB_INTERFACE_RX_BUFF_SIZE);
    
	
	// initCMD((CMD_DESC *)&g_USB_INFO.cmd,"USB",(uint8_t *)g_USB_INFO.interface.rx_buff,g_USB_INFO.interface.rx_buff_size,CMD_FRAGMENTATION|CMD_SYNC_ONE_BYTE);



	// debugUSB(DNONE,"init done\n");
	
// #define USB_UNIT_TEST
#ifdef USB_UNIT_TEST
	debugUSB(DNONE,"UNIT TEST\n");
	g_USB_INFO.timer = getSYSTIM();

	while(1)
	{
		debugUSB(DNONE,"tx-ed pkt \"Test USB\"\n");
		txUSB((uint8_t*)data, strlen(data));
		delay_ms(1000);
	}
#endif

}

void chkUSB(void)
{
	// chkCMD((CMD_DESC *)&g_USB_INFO.cmd,(IFACE_DESC *)&g_USB_INFO.interface);
}

void txUSB(uint8_t * data, uint16_t size)
{
	CDC_Transmit_FS((uint8_t *)data,size);
}


void txCMD(uint8_t cmd,uint8_t chunk,uint8_t * payload,uint8_t size, uint16_t uuid)
{

	// preamble 1B , cmd 1B, chunk 1B, size 1B, payload (size x bytes),csum 1B
	//------------------------------
	uint8_t cmd_payload_size = 59;
	//-----------------------------
    if(size >cmd_payload_size)
    {
        // debugCMD(DNONE, " tx failed [%d/%d]\n",size,cmd_payload_size);
        return;
    }
    uint8_t k = 0;
    uint8_t n = 0;
    uint8_t crc = 0x00;
	uint8_t buff[64];
// #define CMD_PACKAGE_SYNC_BYTE0_ENABLE  1
// #if CMD_PACKAGE_SYNCE_BYTE0_ENABLE == 1
    buff[k] = 0xA5;
    crc += buff[k];
    k++;
// #endif

    buff[k] = cmd;
    crc += buff[k];
    k++;

	buff[k] = chunk;
	crc += buff[k];
	k++;

    buff[k] = size;
    crc += buff[k];
    k++;
    for( n = 0; n < size; n++ )
    {
        buff[k]  = payload[n];
        crc += buff[k];
        k++;
    }
    crc = (~crc) + 1;
    buff[k] = crc;
    k++;
	
#ifdef CMD_DEBUG_VERBOSE
	debugCMD(DNONE,"Tx CMD: ");
	for(uint8_t i=0; i<k; i++)
	{
		debugCMD(DAPPEND,"%xb ",buff[i]);
	}
	debugCMD(DAPPEND,"\n");
#endif
    txUSB((uint8_t *)buff,k);
}



void usb_tx_data(uint8_t * data,uint32_t size)
{
	int res = USBD_BUSY;
	do
	{
		res = CDC_Transmit_FS((uint8_t *)data,size);
		/* code */
	} while (res!= USBD_OK);
	
}
