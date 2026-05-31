/* Define to prevent recursive inclusion *********************************** */
#ifndef __DEBUG_H
#define __DEBUG_H
/* Includes **************************************************************** */
#include <stdio.h>
#include <stdarg.h>
#include "usart_hal.h"

/* Module configuration **************************************************** */
#define debug_log(type, ...)					debug_print(type, "DBG", __VA_ARGS__)

#define DEVICE_ID_REG_SIZE					    12
#define DEVICE_ID_REG_ADDR						0x1FF1E800

#define SYSTEM_DEBUG
#define LINUX_OS

/* Exported constants ****************************************************** */
typedef enum
{
	DEBUG_COLOR_BLACK = 0x30,
	DEBUG_COLOR_RED,
	DEBUG_COLOR_GREEN,
	DEBUG_COLOR_YELLOW,
	DEBUG_COLOR_BLUE,
	DEBUG_COLOR_MAGENTA,
	DEBUG_COLOR_CYAN,
	DEBUG_COLOR_WHITE,
	DEBUG_COLOR_COUNT,
} DEBUG_TEXT_COLORS;

#define DNONE									0x0000
#define DAPPEND									0x0001
#define DWARNING								0x0002
#define DERROR									0x0004	
#define DNOTIFY									0x0008
#define DHEADER									0x0010  

#define DEBUG_CPU_NAME_SIZE						16



/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */
typedef struct 
{
    USART_HAL * usart;
} DEBUG_INFO;


/* Exported variables ****************************************************** */
extern DEBUG_INFO g_DEBUG_INFO;

/* Exported functions ****************************************************** */
void debug_init(USART_HAL * usart,uint32_t baudrate,char * app);
void debug_print(uint16_t sid, char * mod_name, char *str, ... );

#endif 

