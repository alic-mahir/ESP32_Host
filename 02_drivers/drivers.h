/* Define to prevent recursive inclusion *********************************** */
#ifndef __DRIVERS_H
#define __DRIVERS_H
/* Includes **************************************************************** */
#include "mcu.h"

/* Module configuration **************************************************** */

/* Exported constants ****************************************************** */
enum DRIVER_STATUS
{
    DRIVER_STATUS_OK = 0,
    DRIVER_STATUS_ERROR = -1,
    DRIVER_STATUS_BUSY = -2,
    DRIVER_STATUS_TIMEOUT = - 3,
};

enum DRIVER_STATE
{
    DRIVER_STATE_DEINITALIZED = 0x00,
    DRIVER_STATE_INITIALIZED,
    DRIVER_STATE_BUSY,

    DRIVER_STATE_CNT,
};
/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */

/* Exported variables ****************************************************** */

/* Exported functions ****************************************************** */

#endif 

