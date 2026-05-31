/* Includes **************************************************************** */
#include "memory.h"
#include "debug.h"
#include "delay.h"

/* Private types *********************************************************** */

/* Private constants ******************************************************* */

/* Private macros ********************************************************** */

/* Private variables ******************************************************* */

/* Private function prototypes ********************************************* */
static int8_t memory_verify(uint8_t * rdata, uint8_t * wdata, uint16_t size);
/* Exported functions ****************************************************** */
int8_t memory_unit_test(MEMORY_DRIVER * driver,char * driver_type,uint32_t addr)
{
	memory_log(DNONE,"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n");
	memory_log(DNONE,"w %s UNIT TEST\n",driver_type);
	memory_log(DNONE,"w<wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n");

	uint8_t status = driver->init();
	if(status == DRIVER_STATUS_OK)
	{
		uint8_t wdata[4096];
		uint8_t rdata[4096];
		uint32_t n;
		uint32_t res = 0;
		
		uint32_t ptimer = systim_get_tick();
		status = driver->erase(addr,1);
		memory_log(DNONE,"erase CMD R: [%xb] completed in %d ms\n", status, systim_get_elapsed_time(ptimer));
		
		
		for(n=0;n<(driver->erase_size);n++)
		{
			wdata[n] = 0xFF;
			rdata[n] = 0x00;
		}
		driver->read(addr, rdata, driver->erase_size);
		res += memory_verify(rdata, wdata, driver->erase_size);

		if(res)
		{
			memory_log(DERROR,"erase verification failed R: %d\n",res);
			while(1);
		}
		else
		{
			memory_log(DNONE|DNOTIFY,"erase verification OK\n");
		}
		
		res = 0;
		n = 0;
		ptimer = systim_get_tick();
		for(n=0;n<driver->write_size;n++)
		{
			wdata[n] = n;
		}

		res += driver->write(addr,wdata,driver->write_size,1);
		memory_log(DNONE,"write CMD R: [%xb] completed in %d ms\n", res, systim_get_elapsed_time(ptimer));

		res = 0;
		n = 0;
		ptimer = systim_get_tick();
		for(n=0;n<driver->write_size;n++)
		{
			wdata[n] = n;
		}

		status += driver->read(addr, rdata, driver->write_size);
		res += memory_verify(rdata, wdata,driver->write_size);
		memory_log(DNONE,"read CMD R: [%xb] completed in %d ms\n", status, systim_get_elapsed_time(ptimer));
		
		if(res)
		{
			memory_log(DERROR,"read verification failed R: %d\n",res);
			while(1);
		}
		else
		{
			memory_log(DNONE|DNOTIFY,"read verification OK\n");
		}
	}
	return status;
}


/* Private functions ******************************************************* */
int8_t memory_verify(uint8_t * rdata, uint8_t * wdata, uint16_t size)
{
	uint32_t k;
	for(k=0;k<size;k++)
	{
		if(rdata[k] != wdata[k])
		{
			memory_log(DERROR, "invalid data %d [%xb - %xb]\n", k,rdata[k], wdata[k]);
			return DRIVER_STATUS_ERROR;
		}
	}
	return DRIVER_STATUS_OK;
}

/* ***************************** END OF FILE ******************************* */


