/**
* @file        cstring.h
* @author      semir-t
* @date        Maj 2024
* @version     1.0.0
*/

/* Define to prevent recursive inclusion *********************************** */
#ifndef __CSTRING_H
#define __CSTRING_H
/* Includes **************************************************************** */
#include "mcu.h"

/* Module configuration **************************************************** */
#define MAX_STRING_SIZE					1024

/* Exported constants ****************************************************** */
enum ARG_TYPE
{
	ARG_TYPE_UNKNOWN = 0x0000,
	ARG_TYPE_BINARY_BYTE = 0x0001,
	ARG_TYPE_BINARY_HALFWORD = 0x0002,
	ARG_TYPE_BINARY_WORD = 0x0004,
	ARG_TYPE_DECIMAL_BYTE = 0x0008,
	ARG_TYPE_DECIMAL_HALFWORD = 0x0010,
	ARG_TYPE_DECIMAL_WORD = 0x0020,
	ARG_TYPE_CHARACTER = 0x0040,
	ARG_TYPE_STRING = 0x0080,
	ARG_TYPE_FLOAT = 0x0100,
	ARG_TYPE_HEXADECIMAL_BYTE = 0x0200,
	ARG_TYPE_HEXADECIMAL_HALFWORD = 0x0400,
	ARG_TYPE_HEXADECIMAL_WORD = 0x0800,
};

#define ARG_TYPE_MASK_CHAR_STRING			~((ARG_TYPE_CHARACTER)|(ARG_TYPE_STRING))


/* Exported macros ********************************************************* */

/* Exported types ********************************************************** */

/* Exported variables ****************************************************** */

/* Exported functions ****************************************************** */
char * cstring_print(char * dstr, char * str, ... );
char * cstring_num_to_strf(char * dstr, int64_t  * num, uint16_t type, uint8_t decimal_places);
int16_t cstring_int_to_str(char * dstr,int number);
void cstring_reverse(char * str, int length);
char * cstring_cpy(char * dst, char * src);
char * cstring_cat(char * dst, char * src);
uint32_t cstring_len(char * str);
int32_t cstring_cmp(char * rvalue, char * lvalue);

#endif 

