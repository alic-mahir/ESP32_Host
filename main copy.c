#include "main.h"

//########################################################
// NOTES:
//--------------------------------------------------------
// Programming interface is set using python script
// programmer-cmd.py
// Sript can be run using "make prog-cmd" command 
//
//	CMD example: 
//	------------------------------------------------------
// 	| preamble  |  cmd  | chucnk | size | payload | csum |
//	------------------------------------------------------
//	|   0xA5    |  0x00 |  0x00  | 0x00 |         | 0x5B |	
//	------------------------------------------------------
//########################################################


int main(void)
{
	initCLOCK();
	initDEBUG("CM3",0,"MICROBIT DESIGN - Universal flashing tool");
	initSYSTIM();
	initMUX();	
	initUSB();
	debugMAIN(DNONE,"init done, system running ...\n");

	// uint32_t btime = getSYSTIM();
	// uint32_t cnt = 0;
	
	while (1)
  	{
		chkUSB();
		// if(chk4TimeoutSYSTIM(btime,1000)==SYSTIM_TIMEOUT)
		// {
		// 	debugMAIN(DNONE,"CNT: %d\n",cnt++);
		// 	btime = getSYSTIM();
		// }
	}
} 