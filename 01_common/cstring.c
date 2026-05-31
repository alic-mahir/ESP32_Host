/* Includes **************************************************************** */
#include "cstring.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

/* Private types *********************************************************** */

/* Private constants ******************************************************* */

/* Private macros ********************************************************** */

/* Private variables ******************************************************* */

/* Private function prototypes ********************************************* */

/* Exported functions ****************************************************** */
char * cstring_print(char * dstr, char * str, ... )
{
	char rstr[40];													// 33 max -> 32 ASCII for 32 bits and NULL
	uint16_t k = 0;
	uint16_t arg_type;
	int64_t tmp64;
	int64_t * p_tmp64;
	char * p_char;
	va_list vl;
    uint32_t cnt = 0;
	uint8_t decimal_places = 2;
    /* dstr[0] = '\0'; */
	k = 0;
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
                        dstr[cnt++] = tchar;
						arg_type = (ARG_TYPE_CHARACTER);
						break;
					}
					case('s'):
					{
						// string
						p_char = va_arg(vl, char *);
                        uint16_t n = 0;
                        while (p_char[n] != '\0')
                        {
                            dstr[cnt++] = p_char[n];
                            n++;
                            if (n == MAX_STRING_SIZE)
                            {
                                break;
                            }
                        }

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
                    uint16_t n = 0;
                    while (rstr[n] != '\0')
                    {
                        dstr[cnt++] = rstr[n];
                        n++;
                        if (n == MAX_STRING_SIZE)
                        {
                            break;
                        }
                    }

				}
				k++;
			}
		}
		else
		{
			// not a '%' char -> print the char
            dstr[cnt++] = str[k];
		}
		k++;
	}
	va_end(vl);
    dstr[cnt++] = '\0';
	return dstr;
}

char * cstring_num_to_strf(char * dstr, int64_t  * num, uint16_t type, uint8_t decimal_places)
{   
    int64_t * p_int32;
    uint8_t k;
    uint16_t m = 0;
    dstr[0] = 0x00;
    
	switch(type)
    {
		case(ARG_TYPE_BINARY_BYTE):
		{
			for(k=0;k<8;k++)
			{
				if((*num) & 0x00000080)
				{
					dstr[k] = '1';
				}
				else
				{
					dstr[k] = '0';
				}
				*num = (*num)<<1;
			}
			dstr[k] = 0x00;
			break;
		}
		case(ARG_TYPE_BINARY_HALFWORD):
		{
			for(k=0;k<16;k++)
			{
				if((*num) & 0x00008000)
				{
					dstr[k] = '1';
				}
				else
				{
					dstr[k] = '0';
				}
				*num = (*num)<<1;
			}
			dstr[k] = 0x00;
			break;
		}
		case(ARG_TYPE_BINARY_WORD):
		{
			for(k=0;k<32;k++)
			{
				if((*num) & 0x80000000)
				{
					dstr[k] = '1';
				}
				else
				{
					dstr[k] = '0';
				}
				*num = (*num)<<1;
			}
			dstr[k] = 0x00;
			break;
		}
		case(ARG_TYPE_DECIMAL_BYTE):
		case(ARG_TYPE_DECIMAL_HALFWORD):
		case(ARG_TYPE_DECIMAL_WORD):
		{
			p_int32 = (int64_t *)num;
        	m += cstring_int_to_str(&dstr[m],(int32_t)(*p_int32));
			break;
		}
		case(ARG_TYPE_CHARACTER):
		{
			break;
		}
		case(ARG_TYPE_STRING):
		{
			break;
		}
		case(ARG_TYPE_FLOAT):
		{
			double f_num = *((double *)num); 

            p_int32 = (int64_t *)num;
			uint32_t decimal_point = 1;
			uint8_t k = 0;
			for(k = 0; k < decimal_places; k++)
			{
				decimal_point *= 10;
			}
			f_num = roundf(f_num * decimal_point)/(decimal_point);
            int32_t decimal = f_num;
			int32_t fractional = decimal;
			if(decimal < 0)
			{
				f_num = f_num * (-1);
				fractional = fractional * (-1);
			}
            fractional = (f_num * decimal_point) - (fractional * decimal_point);

			m += cstring_int_to_str(&dstr[m],(int32_t)(decimal));
			dstr[m++] = '.';
			int32_t tmp = fractional;
			if(tmp == 0)
			{
				for( k = 0; k < (decimal_places - 1); k++)
				{
					dstr[m++] = '0';
				}
			}
			else
			{
				while((tmp < (decimal_point/10)))
				{
					dstr[m++] = '0';
					tmp *= 10;
				}

			}
			m += cstring_int_to_str(&dstr[m],(int32_t)(fractional));
			break;
		}
		case(ARG_TYPE_HEXADECIMAL_BYTE):
		{
			uint8_t t_rez;
            for (k=0;k<2;k++)
            {
                t_rez = ((*num) & 0x000000F0) >> 4;
                if (t_rez < 0x0A)
                {
                    dstr[m] = t_rez + 0x30;
                }
                else
                {
                    dstr[m] = t_rez + 0x41 - 0x0A;
                }
                (*num) = (*num) << 4;
                m++;
            }
            dstr[m] = 0x00;
			break;
		}
		case(ARG_TYPE_HEXADECIMAL_HALFWORD):
		{
			uint8_t t_rez;
            for (k=0;k<4;k++)
            {
                t_rez = ((*num) & 0x0000F000) >> 12;
                if (t_rez < 0x0A)
                {
                    dstr[m] = t_rez + 0x30;
                }
                else
                {
                    dstr[m] = t_rez + 0x41 - 0x0A;
                }
                (*num) = (*num) << 4;
                m++;
            }
            dstr[m] = 0x00;
			break;
		}
		case(ARG_TYPE_HEXADECIMAL_WORD):
		{
            uint8_t t_rez;
            for (k=0;k<8;k++)
            {
                t_rez = ((*num) & 0xF0000000) >> 28;
                if (t_rez < 0x0A)
                {
                    dstr[m] = t_rez + 0x30;
                }
                else
                {
                    dstr[m] = t_rez + 0x41 - 0x0A;
                }
                (*num) = (*num) << 4;
                m++;
            }
            dstr[m] = 0x00;
			break;
		}
		default:
		{
			dstr[0] = 0x00;
			break;
		}
	}
	return dstr;
}

int16_t cstring_int_to_str(char * dstr,int number)
{
	int i = 0;
	int isNegative = 0;

	if (number == 0)
	{
		dstr[i++] = '0';
		dstr[i] = '\0';
		return i;
	}

	if (number < 0) 
	{
		isNegative = 1;
		number = -number;
	}

	while (number != 0) 
	{
		int rem = number % 10;
		dstr[i++] = rem + '0';
		number = number / 10;
	}

	if (isNegative)
	{
		dstr[i++] = '-';
	}

	dstr[i] = '\0';

	cstring_reverse(dstr, i);
	return i;
}

void cstring_reverse(char * str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) 
	{
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}


char * cstring_cpy(char * dst, char * src)
{
	uint16_t k = 0;

	while(src[k] != '\0')
	{
		dst[k] = src[k];
		k++;
	}
	dst[k] = '\0';
	return dst;
}
char * cstring_cat(char * dst, char * src)
{
	uint16_t k = 0;
	uint16_t n = 0;
	
	while(dst[k] != '\0')
	{
		k++;
	}
	
	while(src[n] != '\0')
	{
		dst[k] = src[n];
		n++;
		k++;
	}
	dst[k] = '\0';
	return dst;
}
uint32_t cstring_len(char * str)
{
	uint32_t k = 0;
	while(str[k] != '\0')
	{
		k++;
	}
	return k;

}
int32_t cstring_cmp(char * str1, char * str2)
{
	while (*str1 && (*str1 == *str2)) 
	{
		str1++;
		str2++;
	}
	return *str1 - *str2;
}
/* Private functions ******************************************************* */

/* ***************************** END OF FILE ******************************* */


