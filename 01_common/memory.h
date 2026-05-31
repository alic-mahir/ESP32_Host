/**
* @file        memory.h
* @author      semir-t
* @date        Januar 2024
* @version     1.0.0
*/

/* Define to prevent recursive inclusion *********************************** */
#ifndef __MEMORY_H
#define __MEMORY_H
/* Includes **************************************************************** */
#include "mcu.h"
#include "debug.h"

/* Module configuration **************************************************** */
#define memory_log(type,...)                     debug_print(type,"MEM",__VA_ARGS__) 
// #define memory_log(type,...) 
//
/* Exported constants ****************************************************** */
enum MEMORY_INIT
{
    MEMORY_INIT_UNCALLED = 0x00,
    MEMORY_INIT_SUCCESS,
    MEMORY_INIT_FAIL,
};


enum MEMORY_STATUS
{
    MEMORY_STATUS_OK = 0,
    MEMORY_STATUS_ERROR = -1,
};

#define MEMORY_SKIP_WAIT4COMPLETION					0
#define MEMORY_WAIT4COMPLETION						1

/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */
typedef struct
{
	int8_t state;
	int8_t status;
	uint32_t timer;

	int8_t (*init)(void);
	int8_t (*deinit)(void);
	int8_t (*read)(uint32_t addr, uint8_t * data, uint32_t size);
	int8_t (*write)(uint32_t addr, uint8_t * data, uint32_t size, uint8_t waitflag);
	int8_t (*erase)(uint32_t addr, uint8_t waitflag);
	int8_t (*is_busy)(void);
	uint32_t erase_size;
	uint32_t write_size;
	uint32_t read_size;

} MEMORY_DRIVER;

/* Exported variables ****************************************************** */

/* Exported functions ****************************************************** */
int8_t memory_unit_test(MEMORY_DRIVER * driver,char * driver_type,uint32_t addr);

#endif 

