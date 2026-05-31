/* Includes **************************************************************** */
#include "mcu.h"
#include "debug.h"
#include "fwver.h"
/* #include "delay.h" */
#include "cstring.h"

#define LINUX_OS
/* Private types *********************************************************** */

/* Private constants ******************************************************* */

/* Private macros ********************************************************** */

/* Private variables ******************************************************* */
DEBUG_INFO g_DEBUG_INFO;

/* Private function prototypes ********************************************* */
uint32_t  __attribute__((weak)) systim_get_tick()
{ 
    return 0;
}

/* Exported functions ****************************************************** */

void debug_init(USART_HAL * usart,uint32_t baudrate,char * app)
{
    uint8_t k;

    g_DEBUG_INFO.usart = usart;
    if ((g_DEBUG_INFO.usart != 0) && (g_DEBUG_INFO.usart->init != 0)) {
        g_DEBUG_INFO.usart->init(baudrate);
    }

	   debug_log(DAPPEND,"\n\n");
    debug_log(DAPPEND | DHEADER,"\n\nwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n");
    debug_log(DAPPEND | DHEADER,"w Developed by: MicroBitDesign LTD.\n");
    debug_log(DAPPEND | DHEADER,"w Srebrenik,Bosnia and Herzegovina\n");
    debug_log(DAPPEND | DHEADER,"w----------------------------------------------------------------\n");
    debug_log(DAPPEND | DHEADER,"w %s\nw\n",app);
    uint32_t fwver = c_firmware_version;
    uint16_t hwd_p1 = (fwver>>28)&0x0000000F;
    uint16_t hwd_p2 = (fwver>>24)&0x0000000F;
    debug_log(DAPPEND | DHEADER,"w hwver: %d.%d\n",hwd_p1,hwd_p2);
    debug_log(DAPPEND | DHEADER,"w fwver: %d.%d.%d.\n",(fwver>>16)&0x000000FF,(fwver>>8)&0x000000FF,(fwver)&0x000000FF);
    debug_log(DAPPEND | DHEADER,"w time:  %x\n",c_build_time);
    debug_log(DAPPEND | DHEADER,"w cpuid: ");
    for( k = 0; k <(MCU_UID_SIZE); k++)
    {
        debug_log(DAPPEND | DHEADER,"%xb",g_MCU_INFO.uid[k]);
    }
    debug_log(DAPPEND | DHEADER,"\n");
    debug_log(DAPPEND | DHEADER,"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n");
}



void debug_print(uint16_t sid, char * mod_name, char *str, ... )
{
#ifndef SYSTEM_DEBUG
	return;
#endif

	uint8_t decimal_places = 2;
	char rstr[40];													// 33 max -> 32 ASCII for 32 bits and NULL
	uint16_t k = 0;
	uint16_t arg_type;
	int64_t tmp64;
	int64_t * p_tmp64;
	char * p_char;
	va_list vl;
	uint32_t pidx = 0;

	uint8_t payload[1024];
#ifdef LINUX_OS

	if((sid & DAPPEND) == 0x0000)
	{
		// print SYSTIM
		rstr[0] = 0;
        uint32_t time = systim_get_tick();
        // getStr4NumMISC(PRINT_ARG_TYPE_HEXADECIMAL_WORD, &time, rstr);
		cstring_print(rstr,"%x",time);
		

        payload[pidx++]=('\e'); 
		payload[pidx++]=('[');
        payload[pidx++]=('3');
        payload[pidx++]=(DEBUG_COLOR_GREEN);
        payload[pidx++]=('m');
        payload[pidx++]=('[');
        for(k=0; k<8; k++)
        {
            payload[pidx++]=(rstr[k]);
        }
        payload[pidx++]=(']');



        payload[pidx++]=('\e');
        payload[pidx++]=('[');
        payload[pidx++]=('3');
        payload[pidx++]=('5');
        payload[pidx++]=('m');
        payload[pidx++]=('~');


        // print the module
        k = 0;
        while(mod_name[k] != '\0')
        {
            payload[pidx++]=(mod_name[k++]);
        }

        payload[pidx++]=(':');
        payload[pidx++]=(' ');
        payload[pidx++]=('\e');
        payload[pidx++]=('[');
        payload[pidx++]=('0');
        payload[pidx++]=('m');

    }
    if(sid & DERROR)
    {
        payload[pidx++]=('\e');
        payload[pidx++]=('[');
        payload[pidx++]=('3');
        payload[pidx++]=(DEBUG_COLOR_RED);
        payload[pidx++]=('m');
    }
    else if(sid & (DWARNING))
    {
        payload[pidx++]=('\e');
        payload[pidx++]=('[');
        payload[pidx++]=('3');
        payload[pidx++]=(DEBUG_COLOR_YELLOW);
        payload[pidx++]=('m');
    }
    else if(sid & (DHEADER))
    {
        payload[pidx++]=('\e');
        payload[pidx++]=('[');
        payload[pidx++]=('3');
        payload[pidx++]=(DEBUG_COLOR_CYAN);
        payload[pidx++]=('m');
    }
    else if(sid & (DNOTIFY))
    {
        payload[pidx++]=('\e');
        payload[pidx++]=('[');
        payload[pidx++]=('3');
        payload[pidx++]=(DEBUG_COLOR_GREEN);
        payload[pidx++]=('m');
    }

#endif


	k = 0;
	//va_start(vl, 10);													// always pass the last named parameter to va_start, for compatibility with older compilers
	va_start(vl, str);													// always pass the last named parameter to va_start, for compatibility with older compilers
	while(str[k] != 0x00)
	{
		if(str[k] == '%')
		{
			if(str[k+1] != 0x00)
			{
				switch(str[k+1])
				{
					case('b'):
					{
						if(str[k+2] == 'b')
						{
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_BINARY_BYTE);
						}
						else if(str[k+2] == 'h')
						{
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_BINARY_HALFWORD);
						}
						else if(str[k+2] == 'w')
						{
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_BINARY_WORD);
						}
						else
						{
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_BINARY_WORD);
							k--;
						}
						k++;
						p_tmp64 = &tmp64;
						break;
					}
					case('d'):
					{
						if(str[k+2] == 'b')
						{
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_DECIMAL_BYTE);
						}
						else if(str[k+2] == 'h')
						{
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_DECIMAL_HALFWORD);
						}
						else if(str[k+2] == 'w')
						{
							// word
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_DECIMAL_WORD);
						}
						else
						{
							// default word
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_DECIMAL_WORD);
							k--;
						}

						k++;
						p_tmp64 = &tmp64;
						break;
					}
					case('c'):
					{
						// character
						char tchar = va_arg(vl, int);
						payload[pidx++]=(tchar);
						arg_type = (ARG_TYPE_CHARACTER);
						break;
					}
					case('s'):
					{
						// string
						
						p_char = va_arg(vl, char *);
						uint32_t i = 0;
						while(p_char[i]!='\0')
						{
							payload[pidx++] = (p_char[i++]);
						}
						// g_DEBUG_INFO.usart->txString((uint8_t *)p_char);
						arg_type = (ARG_TYPE_STRING);
						break;
					}
					case('.'):
					{
						if((str[k+2] >= '0') && (str[k+2] <= '9'))
						{
							decimal_places = str[k+2] - '0';
						}
						k++;
						k++;
						typedef union
						{
							double d;
							uint64_t u;

						} tmp;
						tmp number;
						number.d = va_arg(vl, double);		
						tmp64 = number.u;
						p_tmp64 = &tmp64;
						arg_type = (ARG_TYPE_FLOAT);
						break;
					}
					case('f'):
					{
						typedef union
						{
							double d;
							uint64_t u;

						} tmp;
						tmp number;
						number.d = va_arg(vl, double);		
						tmp64 = number.u;
						p_tmp64 = &tmp64;
						arg_type = (ARG_TYPE_FLOAT);
						break;
					}
					case('x'):
					{
						if(str[k+2] == 'b')
						{
							tmp64 = (uint32_t)va_arg(vl, int);
							arg_type = (ARG_TYPE_HEXADECIMAL_BYTE);
						}
						else if(str[k+2] == 'h')
						{
							tmp64 = (uint32_t)va_arg(vl, int);
							arg_type = (ARG_TYPE_HEXADECIMAL_HALFWORD);
						}
						else if(str[k+2] == 'w')
						{
							// word
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_HEXADECIMAL_WORD);
						}
						else
						{
							tmp64 = va_arg(vl, int);
							arg_type = (ARG_TYPE_HEXADECIMAL_WORD);
							k--;
						}
						k++;
						p_tmp64 = &tmp64;
						break;
					}
					default:
					{
						tmp64 = 0;
						p_tmp64 = &tmp64;
						arg_type = (ARG_TYPE_UNKNOWN);
						break;
					}
				}
				if(arg_type&(ARG_TYPE_MASK_CHAR_STRING))
				{
					cstring_num_to_strf(rstr, p_tmp64, arg_type, decimal_places);
					// g_DEBUG_INFO.usart->txString((uint8_t *)rstr);

					uint32_t i = 0;
					while(rstr[i]!='\0')
					{
						payload[pidx++] = (rstr[i++]);
					}
				}
				k++;
			}
		}
		else
		{
			// not a '%' char -> print the char
			payload[pidx++]=(str[k]);
			if (str[k] == '\n')
			{
				payload[pidx++]=('\r');
			}

		}
		k++;
	}

	va_end(vl);

#ifdef LINUX_OS
	if(sid & 0xC000)
	{
		payload[pidx++]=('\e');
		payload[pidx++]=('[');
		payload[pidx++]=('0');
		payload[pidx++]=('m');
	}
#endif
	

    if ((g_DEBUG_INFO.usart != 0) && (g_DEBUG_INFO.usart->tx_data != 0)) {
        g_DEBUG_INFO.usart->tx_data(payload, pidx, 100000000);
    }
}

// void debug_print(uint16_t sid, char * mod_name, char *str, ... )
// {
// #ifndef SYSTEM_DEBUG
// 	return;
// #endif

// 	uint8_t decimal_places = 2;
// 	char rstr[40];													// 33 max -> 32 ASCII for 32 bits and NULL
// 	uint16_t k = 0;
// 	uint16_t arg_type;
// 	int64_t tmp64;
// 	int64_t * p_tmp64;
// 	char * p_char;
// 	va_list vl;

// #ifdef LINUX_OS

// 	if((sid & DAPPEND) == 0x0000)
// 	{
// 		// print SYSTIM
// 		rstr[0] = 0;
//         uint32_t time = systim_get_tick();
//         // getStr4NumMISC(PRINT_ARG_TYPE_HEXADECIMAL_WORD, &time, rstr);
// 		cstring_print(rstr,"%x",time);
//         g_DEBUG_INFO.usart->tx_byte('\e',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         g_DEBUG_INFO.usart->tx_byte('3',100000);
//         g_DEBUG_INFO.usart->tx_byte(DEBUG_COLOR_GREEN,100000);
//         g_DEBUG_INFO.usart->tx_byte('m',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         for(k=0; k<8; k++)
//         {
//             g_DEBUG_INFO.usart->tx_byte(rstr[k],100000);
//         }
//         g_DEBUG_INFO.usart->tx_byte(']',100000);



//         g_DEBUG_INFO.usart->tx_byte('\e',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         g_DEBUG_INFO.usart->tx_byte('3',100000);
//         g_DEBUG_INFO.usart->tx_byte('5',100000);
//         g_DEBUG_INFO.usart->tx_byte('m',100000);
//         g_DEBUG_INFO.usart->tx_byte('~',100000);


//         // print the module
//         k = 0;
//         while(mod_name[k] != '\0')
//         {
//             g_DEBUG_INFO.usart->tx_byte(mod_name[k++],100000);
//         }

//         g_DEBUG_INFO.usart->tx_byte(':',100000);
//         g_DEBUG_INFO.usart->tx_byte(' ',100000);
//         g_DEBUG_INFO.usart->tx_byte('\e',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         g_DEBUG_INFO.usart->tx_byte('0',100000);
//         g_DEBUG_INFO.usart->tx_byte('m',100000);

//     }
//     if(sid & DERROR)
//     {
//         g_DEBUG_INFO.usart->tx_byte('\e',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         g_DEBUG_INFO.usart->tx_byte('3',100000);
//         g_DEBUG_INFO.usart->tx_byte(DEBUG_COLOR_RED,100000);
//         g_DEBUG_INFO.usart->tx_byte('m',100000);
//     }
//     else if(sid & (DWARNING))
//     {
//         g_DEBUG_INFO.usart->tx_byte('\e',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         g_DEBUG_INFO.usart->tx_byte('3',100000);
//         g_DEBUG_INFO.usart->tx_byte(DEBUG_COLOR_YELLOW,100000);
//         g_DEBUG_INFO.usart->tx_byte('m',100000);
//     }
//     else if(sid & (DHEADER))
//     {
//         g_DEBUG_INFO.usart->tx_byte('\e',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         g_DEBUG_INFO.usart->tx_byte('3',100000);
//         g_DEBUG_INFO.usart->tx_byte(DEBUG_COLOR_CYAN,100000);
//         g_DEBUG_INFO.usart->tx_byte('m',100000);
//     }
//     else if(sid & (DNOTIFY))
//     {
//         g_DEBUG_INFO.usart->tx_byte('\e',100000);
//         g_DEBUG_INFO.usart->tx_byte('[',100000);
//         g_DEBUG_INFO.usart->tx_byte('3',100000);
//         g_DEBUG_INFO.usart->tx_byte(DEBUG_COLOR_GREEN,100000);
//         g_DEBUG_INFO.usart->tx_byte('m',100000);
//     }

// #endif


// 	k = 0;
// 	//va_start(vl, 10);													// always pass the last named parameter to va_start, for compatibility with older compilers
// 	va_start(vl, str);													// always pass the last named parameter to va_start, for compatibility with older compilers
// 	while(str[k] != 0x00)
// 	{
// 		if(str[k] == '%')
// 		{
// 			if(str[k+1] != 0x00)
// 			{
// 				switch(str[k+1])
// 				{
// 					case('b'):
// 					{
// 						if(str[k+2] == 'b')
// 						{
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_BINARY_BYTE);
// 						}
// 						else if(str[k+2] == 'h')
// 						{
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_BINARY_HALFWORD);
// 						}
// 						else if(str[k+2] == 'w')
// 						{
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_BINARY_WORD);
// 						}
// 						else
// 						{
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_BINARY_WORD);
// 							k--;
// 						}
// 						k++;
// 						p_tmp64 = &tmp64;
// 						break;
// 					}
// 					case('d'):
// 					{
// 						if(str[k+2] == 'b')
// 						{
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_DECIMAL_BYTE);
// 						}
// 						else if(str[k+2] == 'h')
// 						{
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_DECIMAL_HALFWORD);
// 						}
// 						else if(str[k+2] == 'w')
// 						{
// 							// word
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_DECIMAL_WORD);
// 						}
// 						else
// 						{
// 							// default word
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_DECIMAL_WORD);
// 							k--;
// 						}

// 						k++;
// 						p_tmp64 = &tmp64;
// 						break;
// 					}
// 					case('c'):
// 					{
// 						// character
// 						char tchar = va_arg(vl, int);
// 						g_DEBUG_INFO.usart->tx_byte(tchar,100000);
// 						arg_type = (ARG_TYPE_CHARACTER);
// 						break;
// 					}
// 					case('s'):
// 					{
// 						// string
// 						p_char = va_arg(vl, char *);
// 						g_DEBUG_INFO.usart->tx_string(p_char,100000);
// 						arg_type = (ARG_TYPE_STRING);
// 						break;
// 					}
// 					case('.'):
// 					{
// 						if((str[k+2] >= '0') && (str[k+2] <= '9'))
// 						{
// 							decimal_places = str[k+2] - '0';
// 						}
// 						k++;
// 						k++;
// 						typedef union
// 						{
// 							double d;
// 							uint64_t u;

// 						} tmp;
// 						tmp number;
// 						number.d = va_arg(vl, double);		
// 						tmp64 = number.u;
// 						p_tmp64 = &tmp64;
// 						arg_type = (ARG_TYPE_FLOAT);
// 						break;
// 					}
// 					case('f'):
// 					{
// 						typedef union
// 						{
// 							double d;
// 							uint64_t u;

// 						} tmp;
// 						tmp number;
// 						number.d = va_arg(vl, double);		
// 						tmp64 = number.u;
// 						p_tmp64 = &tmp64;
// 						arg_type = (ARG_TYPE_FLOAT);
// 						break;
// 					}
// 					case('x'):
// 					{
// 						if(str[k+2] == 'b')
// 						{
// 							tmp64 = (uint32_t)va_arg(vl, int);
// 							arg_type = (ARG_TYPE_HEXADECIMAL_BYTE);
// 						}
// 						else if(str[k+2] == 'h')
// 						{
// 							tmp64 = (uint32_t)va_arg(vl, int);
// 							arg_type = (ARG_TYPE_HEXADECIMAL_HALFWORD);
// 						}
// 						else if(str[k+2] == 'w')
// 						{
// 							// word
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_HEXADECIMAL_WORD);
// 						}
// 						else
// 						{
// 							tmp64 = va_arg(vl, int);
// 							arg_type = (ARG_TYPE_HEXADECIMAL_WORD);
// 							k--;
// 						}
// 						k++;
// 						p_tmp64 = &tmp64;
// 						break;
// 					}
// 					default:
// 					{
// 						tmp64 = 0;
// 						p_tmp64 = &tmp64;
// 						arg_type = (ARG_TYPE_UNKNOWN);
// 						break;
// 					}
// 				}
// 				if(arg_type&(ARG_TYPE_MASK_CHAR_STRING))
// 				{
// 					cstring_num_to_strf(rstr, p_tmp64, arg_type, decimal_places);
// 					g_DEBUG_INFO.usart->tx_string(rstr,100000);
// 				}
// 				k++;
// 			}
// 		}
// 		else
// 		{
// 			// not a '%' char -> print the char
// 			g_DEBUG_INFO.usart->tx_byte(str[k],100000);
// 			if (str[k] == '\n')
// 			{
// 				g_DEBUG_INFO.usart->tx_byte('\r',100000);
// 			}

// 		}
// 		k++;
// 	}

// 	va_end(vl);

// #ifdef LINUX_OS
// 	if(sid & 0xC000)
// 	{
// 		g_DEBUG_INFO.usart->tx_byte('\e',100000);
// 		g_DEBUG_INFO.usart->tx_byte('[',100000);
// 		g_DEBUG_INFO.usart->tx_byte('0',100000);
// 		g_DEBUG_INFO.usart->tx_byte('m',100000);
// 	}
// #endif
// }

void NMI_Handler(void)
{
#ifdef SYSTEM_DEBUG	
	debug_log(DERROR,"-> SYS: NMI_Handler\n");
#endif
	while(1);
}

void HardFault_Handler(void)
{
	__asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word HardFault_HandlerC    \n"
    );
	
}

void HardFault_HandlerC( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */

#ifdef SYSTEM_DEBUG	
	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	volatile uint32_t r12;
	volatile uint32_t lr; /* Link register. */
	volatile uint32_t pc; /* Program counter. */
	volatile uint32_t psr;/* Program status register. */

	
    r0 = pulFaultStackAddress[0];
    r1 = pulFaultStackAddress[1];
    r2 = pulFaultStackAddress[2];
    r3 = pulFaultStackAddress[3];

    r12 = pulFaultStackAddress[4];
    lr = pulFaultStackAddress[5];
    pc = pulFaultStackAddress[6];
    psr = pulFaultStackAddress[7];
	
	debug_log(DERROR,": HardFault_Handler \n");
	
	debug_log(DERROR,"r0\t[%x]\n",r0);
	debug_log(DERROR,"r1\t[%x]\n",r1);
	debug_log(DERROR,"r2\t[%x]\n",r2);
	debug_log(DERROR,"r3\t[%x]\n",r3);
	debug_log(DERROR,"r12\t[%x]\n",r12);
	debug_log(DERROR,"lr\t[%x]\n",lr);
	debug_log(DERROR,"pc\t[%x]\n",pc);
	debug_log(DERROR,"psr\t[%x]\n\n",psr);
	
	debug_log(DERROR,"bfar\t[%x]\n",(*((volatile unsigned long *)(0xE000ED38))));
	debug_log(DERROR,"cfsr\t[%x]\n",(*((volatile unsigned long *)(0xE000ED28))));
	debug_log(DERROR,"hfsr\t[%x]\n",(*((volatile unsigned long *)(0xE000ED2C))));
	debug_log(DERROR,"dfsr\t[%x]\n",(*((volatile unsigned long *)(0xE000ED30))));
	debug_log(DERROR,"afsr\t[%x]\n", (*((volatile unsigned long *)(0xE000ED3C))));

#endif	
    
    //NVIC_SystemReset();
    while(1);
}

void MemManage_Handler(void)
{
#ifdef SYSTEM_DEBUG	
	debug_log(DERROR,"-> SYS: MemManage_Handler\n");
#endif
	while(1);	
}

void BusFault_Handler(void)
{
#ifdef SYSTEM_DEBUG	
	debug_log(DERROR,"-> SYS: BusFault_Handler\n");
#endif
	while(1);	
}

void UsageFault_Handler(void)
{
#ifdef SYSTEM_DEBUG	
	debug_log(DERROR,"-> SYS: UsageFault_Handler\n");
#endif
	while(1);
}

void Error_Handler(void)
{
#ifdef SYSTEM_DEBUG	
	debug_log(DERROR,"-> SYS: Error_Handler\n");
#endif
	while(1);
}


/* Private functions ******************************************************* */


/* ***************************** END OF FILE ******************************* */
