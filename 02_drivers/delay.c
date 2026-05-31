/* Includes **************************************************************** */
#include "delay.h"

/* Private types *********************************************************** */

/* Private constants ******************************************************* */

/* Private macros ********************************************************** */

/* Private variables ******************************************************* */
volatile uint32_t g_systim_timer = 0x00000000;

volatile uint32_t g_systim_value;
/* Private function prototypes ********************************************* */

/* Exported functions ****************************************************** */

void delay_ms(uint32_t delay)
{
  uint32_t time = systim_get_tick();
	while(1)
	{
		if(systim_has_time_elapsed(time,delay)==SYSTIM_TIMEOUT)
		{
			break;
		}
	}
}

void delay_us(uint32_t delay)
{

}

void systim_init(void)
{
	g_systim_value = 0;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->PSC = 72 - 1;

	TIM2->ARR = 1000;
	TIM2->CR1 = 0x0084;

	TIM2->CR2 = 0x0000;
	TIM2->CNT = 0x0000;
	TIM2->EGR |= TIM_EGR_UG;

	TIM2->DIER = 0x0001;

	NVIC_SetPriority(TIM2_IRQn, 4);
	NVIC_EnableIRQ(TIM2_IRQn);
	TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
	if(TIM2->SR & 0x0001)
	{
		g_systim_value++;
		TIM2->SR = 0x0000;
	}
}

uint32_t systim_get_tick(void)
{
    return  g_systim_value;
}

uint8_t systim_has_time_elapsed(uint32_t b_time, uint32_t period)
{
 uint32_t time = g_systim_value;
	if(time >= b_time)
	{
		if((time - b_time) >= period)
			return (SYSTIM_TIMEOUT);
		else
			return (SYSTIM_ALIVE);
	}
	else
	{
		uint32_t utmp32 = 0xFFFFFFFF - b_time;
		if((time + utmp32) >= period)
			return (SYSTIM_TIMEOUT);
		else
			return (SYSTIM_ALIVE);
	}
}

uint32_t systim_get_elapsed_time(uint32_t b_time)
{
  uint32_t rtc_time = (TIM2->CNT)/2;;
	if(rtc_time >= b_time)
	{
		return (rtc_time - b_time);
	}
	else
	{
		return (rtc_time + (0xFFFFFFFF - b_time));
	}
}

/* Private functions ******************************************************* */

/* ***************************** END OF FILE ******************************* */


