/*
 Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
 This file is part of Rozofs.

 Rozofs is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation, version 2.

 Rozofs is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see
 <http://www.gnu.org/licenses/>.
 */
 
#ifndef ROZOFS_STRING_H
#define ROZOFS_STRING_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

typedef enum _rozofs_alignment_e {
 rozofs_left_alignment,
 rozofs_right_alignment
} rozofs_alignment_e;




/*
** ===================== FID ==================================
*/





/*
**___________________________________________________________
** Parse a 2 character string representing an hexadecimal
** value.
**
** @param pChar   The starting of the 2 characters
** @param hexa    Where to return the hexadecimal value
**
** @retval The next place to parse in the string or NULL
**         when the 2 characters do not represent an hexadecimal 
**         value
*/
static inline char * rozofs_2char2uint8(char * pChar, uint8_t * hexa) {
  uint8_t val;
  
  if ((*pChar >= '0')&&(*pChar <= '9')) {
    val = *pChar++ - '0';
  }  
  else if ((*pChar >= 'a')&&(*pChar <= 'f')) {
    val = *pChar++ - 'a' + 10; 
  }
  else if ((*pChar >= 'A')&&(*pChar <= 'F')) {
    val = *pChar++ - 'A' + 10; 
  }
  else {
    return NULL;
  }
  
  val = val << 4;
  
  if ((*pChar >= '0')&&(*pChar <= '9')) {
    val += *pChar++ - '0';
  }  
  else if ((*pChar >= 'a')&&(*pChar <= 'f')) {
    val += *pChar++ - 'a' + 10; 
  }
  else if ((*pChar >= 'A')&&(*pChar <= 'F')) {
    val += *pChar++ - 'A' +10; 
  }
  else {
    return NULL;
  }  
  *hexa = val;
  return pChar;
}
/*
**___________________________________________________________
** Parse a string representing an FID
**
** @param pChar   The string containing the FID
** @param fid     The parsed FID
**
** @retval The next place to parse in the string after the FID 
**         or NULL when the string do not represent an FID
*/
static inline int rozofs_uuid_parse(char * pChar, uuid_t fid) {
  uint8_t *pFid = (uint8_t*) fid;

  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
     
  if (*pChar++ != '-') return -1;  
  
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  
  if (*pChar++ != '-') return -1;  

  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  
  if (*pChar++ != '-') return -1;  

  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;
  
  if (*pChar++ != '-') return -1;  
  
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;  
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;  
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;  
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;  
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;  
  pChar = rozofs_2char2uint8(pChar,pFid++);
  if (pChar == NULL) return -1;  
   
  return 0;
}

/*
**___________________________________________________________
** Get the a byte value and translate into 2 ASCII chars
**
** @param hexa    The byte value to display
** @param pChar   Where to write the ASCII translation
**
*/
static inline char * rozofs_u8_2_char(uint8_t hexa, char * pChar) {
  uint8_t high = hexa >> 4;
  if (high < 10) *pChar++ = high + '0';
  else           *pChar++ = (high-10) + 'a';
  
  hexa = hexa & 0x0F;
  if (hexa < 10) *pChar++ = hexa + '0';
  else           *pChar++ = (hexa-10) + 'a';
  
  return pChar;
}
/*
**___________________________________________________________
** Get a FID value and display it as a string. An end of string
** is inserted at the end of the string.
**
** @param fid     The FID value
** @param pChar   Where to write the ASCII translation
**
** @retval The end of string
*/
static inline void rozofs_uuid_unparse(uuid_t fid, char * pChar) {
  uint8_t * pFid = (uint8_t *) fid;
  
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);   
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  
  *pChar++ = '-';
  
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);
   
  *pChar++ = '-';
  
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  
  *pChar++ = '-';
  
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  
  *pChar++ = '-';
  
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);   
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);
  pChar = rozofs_u8_2_char(*pFid++,pChar);   

  *pChar = 0;  
}















/*
** ===================== STRINGS ==================================
*/



















/*
**___________________________________________________________
** Append a string and add a 0 at the end
**
**    sprintf(pChar,"%s",new_string) 
** -> rozofs_string_append(pChar, new_string)
**
** @param pChar       The string that is being built
** @param new_string  The string to append. Must have an ending 0
**
** @retval the size added to the built string
*/
static inline int rozofs_string_append(char * pChar, char * new_string) {
  int len=0;
  while ( (*pChar++ = *new_string++) != 0) len ++;
  return len;
}
/*
**___________________________________________________________
** Append a string, padd with ' ' on a given size
** and add a 0 at the end
**
**     sprintf(pChar,"%-12s",new_string) 
**  -> rozofs_string_padded_append(pChar, 12, rozofs_right_alignment,new_string)
**
**     sprintf(pChar,"%16s",new_string)
**  -> rozofs_string_padded_append(pChar, 16, rozofs_left_alignment,new_string)
**
** @param pChar       The string that is being built
** @param size        The total size to write to the built string
** @param alignment   Left/right alignment
** @param new_string  The string to append. Must have an ending 0
**
** @retval the size added to the string
*/
static inline int rozofs_string_padded_append(char * pChar, int size, rozofs_alignment_e alignment, char * new_string) {
  int i;
  int len = strlen(new_string);
  
  /*
  ** new_string is too big => truncate it
  */
  if (len > size) {
    strncpy(pChar,new_string,size);
    pChar[size] = 0;
    return size;
  }
  /*
  ** Put left alignment
  */
  if (alignment == rozofs_left_alignment) {
    for (i=0; i< (size-len) ; i++) {
      *pChar++ = ' ';
    }
  }
  /*
  ** Append string
  */
  for (i=0; i< len ; i++) {
    *pChar++ = *new_string++;
  } 
  /*
  ** Put right alignment
  */  
  if (alignment == rozofs_right_alignment) {
    for (i=0; i< (size-len) ; i++) {
      *pChar++ = ' ';
    }
  }   
  /*
  ** Add the end of string
  */
  *pChar = 0;
  return size;
}



/*
**___________________________________________________________
** Draw a line
** 
** @param column_len      array of column length
**
** @retval the size added to the string
*/
static inline char * rozofs_line(char * pChar, uint8_t * column_len) {
  int len;
  *pChar++ = '+';
  
  len = *column_len++;
  while(len) {
    
    while(len) {
      *pChar++ = '-';
      len--;
    }  
    *pChar++ = '+';
    
    len = *column_len++;
  }
  *pChar++ = '\n';
  *pChar = 0;
  return pChar;
}








/*
** ===================== INTERNAL FUNCT FOR 64bits VALUES  ======================
*/














/*
**___________________________________________________________
** For internal use
** Append a 64 bit number in decimal representation 
** with eventually a minus sign. Add a 0 at the end.
**
** @param pChar       The string that is being built
** @param sign        Whether a sign must be added (0/1)
** @param val         The 64bits unsigned (so positive)value
**
** @retval the size added to the string
*/
static inline int rozofs_64_append(char * pChar, int sign, uint64_t val) {
  int len=0;
  uint8_t  values[32];
  int i;
  
  /*
  ** Decompose the value in 10 base
  */
  len = 0;
  while (val) {
    values[len] = val % 10;
    val = val / 10;    
    len++;
  }
  
  /*
  ** Given value was 0
  */
  if (len == 0) {
    values[0] = 0;
    len = 1;
  }  

  /*
  ** Put sign if required
  */
  if (sign) *pChar++ = '-';
  
  /*
  ** Write the value
  */
  for (i=len-1; i>=0; i--) {
    *pChar++ = values[i]+ '0';
  } 
  /*
  ** Add the end of string
  */   
  *pChar = 0;
  return len+sign;
}
/*
**___________________________________________________________
** For internal use
** Append a 64 bits unsigned number with eventually a minus sign
** in decimal representation on a fixed size to a string. padd with
** ' ' and add a 0 at the end.
**
** @param pChar       The string that is being built
** @param size        The size to add to the string
** @param alignment   Left/right alignment
** @param sign        Whether a sign must be added
** @param val         The 64bits unsigned value
**
** @retval the size added to the strings
*/
static inline int rozofs_64_padded_append(char * pChar, int size, rozofs_alignment_e alignment, int sign, uint64_t val) {
  int len=0;
  uint8_t  values[32];
  int i;
  
  /*
  ** Decompose the value in 10 base
  */  
  len = 0;
  while (val) {
    values[len] = val % 10;
    val = val / 10;    
    len++;
  }
  /*
  ** Given value was 0
  */  
  if (len == 0) {
    values[0] = 0;
    len = 1;
  }  

  len += sign;
  
  /*
  ** Put left alignment
  */
  if (alignment == rozofs_left_alignment) {
    for (i=0; i< (size-len) ; i++) {
      *pChar++ = ' ';
    }
  } 
     
  /*
  ** Put sign if required
  */
  if (sign) *pChar++ = '-';

  /*
  ** Write the value
  */
  for (i=len-1-sign; i>=0; i--) {
    *pChar++ = values[i]+ '0';
  }
  if (len>size) {
    *pChar = 0;
    return len;
  }
  /*
  ** Put right alignment
  */
  if (alignment == rozofs_right_alignment) {
    for (i=0; i< (size-len) ; i++) {
      *pChar++ = ' ';
    }
  }     
  /*
  ** Add the end of string
  */   
  *pChar = 0;
  return size;
} 



















/*
** ===================== 32 bits FORMATING  ======================
*/


















/*
**___________________________________________________________
** Append an unsigned 32 bit number in decimal representation
** to a string. Add a 0 at the end.
**
**     sprintf(pChar,"%u",val) 
**  -> rozofs_u32_append(pChar, val)
**
** @param pChar       The string that is being built
** @param val         The 32bits unsigned value
**
** @retval the size added to the string
*/
static inline int rozofs_u32_append(char * pChar, uint32_t val) {
  return rozofs_64_append(pChar, 0, val);
}
/*
**___________________________________________________________
** Append a signed 32 bit number in decimal representation
** to a string. Add a 0 at the end.
**
**     sprintf(pChar,"%d",val) 
**  -> rozofs_i32_append(pChar, val)
**
** @param pChar       The string that is being built
** @param val         The 32bits signed value
**
** @retval the size added to the string
*/
static inline int rozofs_i32_append(char * pChar, int32_t val) {
  if (val<0) return rozofs_64_append(pChar, 1, -val);
  return rozofs_64_append(pChar, 0, val);
}
/*
**___________________________________________________________
** Append an unsigned 32 bit number in hexadecimal representation
** to a string. Add a 0 at the end.
**
**     sprintf(pChar,"%8.8x",val) 
**  -> rozofs_x32_append(pChar, val)
**
** @param pChar       The string that is being built
** @param val         The 32bits unsigned value
**
** @retval the size added to the string
*/
static inline int rozofs_x32_append(char * pChar, uint32_t val) {
  int i;
  uint32_t v;

  for (i=0; i<8; i++) {
    v = val & 0xF;
    val = val >> 4;
    if (v<10) pChar[7-i] = v + '0';
    else      pChar[7-i] = v + 'a' - 10;
  }
  /*
  ** Add the end of string
  */      
  pChar[8] = 0;
  return 8;
} 
/*
**___________________________________________________________
** Append a 32 bits signed number in decimal representation 
** on a fixed size to a string. padd with ' ' and add a 0 at 
** the end.
**
** sprintf(pChar,"%9d",val) 
**  -> rozofs_i32_append(pChar, 9, rozofs_left_alignment, val)
**
** sprintf(pChar,"%-10d",val) 
**  -> rozofs_i32_append(pChar, 10, rozofs_right_alignment, val)
**
** @param pChar       The string that is being built
** @param size        The size to add to the string
** @param alignment   Left/right alignment
** @param val         The 32bits signed value
**
** @retval the size added to the string
*/
static inline int rozofs_i32_padded_append(char * pChar, int size, rozofs_alignment_e alignment, int32_t val) {
  if (val < 0) return rozofs_64_padded_append(pChar, size, alignment, 1, -val);
  return rozofs_64_padded_append(pChar, size, alignment, 0, val);
} 
/*
**___________________________________________________________
** Append a 32 bits unsigned number in decimal representation 
** on a fixed size to a string. padd with ' ' and add a 0 at 
** the end.
**
** sprintf(pChar,"%10u",val) 
**  -> rozofs_u32_append(pChar, 10, rozofs_left_alignment, val)
**
** sprintf(pChar,"%-12u",val) 
**  -> rozofs_u32_append(pChar, 12, rozofs_right_alignment, val)
**
** @param pChar       The string that is being built
** @param size        The size to add to the string
** @param alignment   Left/right alignment
** @param val         The 32bits unsigned value
**
** @retval the size added to the string
*/
static inline int rozofs_u32_padded_append(char * pChar, int size, rozofs_alignment_e alignment, uint32_t val) {
  return rozofs_64_padded_append(pChar, size, alignment, 0, val);
} 





















/*
** ===================== 64 bits FORMATING  ======================
*/












/*
**___________________________________________________________
** Append an unsigned 64 bit number in decimal representation
** to a string. Add a 0 at the end.
**
**     sprintf(pChar,"%llu",val) 
**  -> rozofs_u64_append(pChar, val)
**
** @param pChar       The string that is being built
** @param val         The 64bits unsigned value
**
** @retval the size added to the string
*/
static inline int rozofs_u64_append(char * pChar, uint64_t val) {
  return rozofs_64_append(pChar, 0, val);
}
/*
**___________________________________________________________
** Append a signed 64 bit number in decimal representation
** to a string. Add a 0 at the end.
**
** sprintf(pChar,"%lld",val) 
**  -> rozofs_i64_append(pChar, val)
**
** @param pChar       The string that is being built
** @param val         The 32bits signed value
**
** @retval the size added to the string
*/
static inline int rozofs_i64_append(char * pChar, int64_t val) {
  if (val<0) return rozofs_64_append(pChar, 1, -val);
  return rozofs_64_append(pChar, 0, val);
}
/*
**___________________________________________________________
** Append an unsigned 64 bit number in hexadecimal representation
** to a string. Add a 0 at the end.
**
** sprintf(pChar,"%16.16llx",val) 
**  -> rozofs_x64_append(pChar, val)
**
** @param pChar       The string that is being built
** @param val         The 64bits unsigned value
**
** @retval the size added to the string
*/
static inline int rozofs_x64_append(char * pChar, uint64_t val) {
  int i;
  uint32_t v;

  for (i=0; i<16; i++) {
    v = val & 0xF;
    val = val >> 4;
    if (v<10) pChar[15-i] = v + '0';
    else      pChar[15-i] = v + 'a' - 10;
  }
  /*
  ** Add the end of string
  */      
  pChar[16] = 0;
  return 16;
} 
/*
**___________________________________________________________
** Append a 64 bits signed number in decimal representation 
** on a fixed size to a string. padd with ' ' and add a 0 at 
** the end.
**
** sprintf(pChar,"%7lld",val) 
**  -> rozofs_x32_append(pChar, 7, rozofs_left_alignment, val)
**
** sprintf(pChar,"%-10lld",val) 
**  -> rozofs_x32_append(pChar, 10, rozofs_right_alignment, val)
**
** @param pChar       The string that is being built
** @param size        The size to add to the string
** @param alignment   Left/right alignment
** @param val         The 64bits signed value
**
** @retval the size added to the string
*/
static inline int rozofs_i64_padded_append(char * pChar, int size, rozofs_alignment_e alignment, int64_t val) {
  if (val < 0) return rozofs_64_padded_append(pChar, size, alignment, 1, -val);
  return rozofs_64_padded_append(pChar, size, alignment, 0, val);
} 
/*
**___________________________________________________________
** Append a 64 bits unsigned number in decimal representation 
** on a fixed size to a string. padd with ' ' and add a 0 at 
** the end.
**
** sprintf(pChar,"%20llu",val) 
**  -> rozofs_u64_append(pChar, 20, rozofs_left_alignment, val)
**
** sprintf(pChar,"%-6llu",val) 
**  -> rozofs_u64_append(pChar, 6, rozofs_right_alignment, val)
**
** @param pChar       The string that is being built
** @param size        The size to add to the string
** @param alignment   Left/right alignment
** @param val         The 64bits unsigned value
**
** @retval the size added to the string
*/
static inline int rozofs_u64_padded_append(char * pChar, int size, rozofs_alignment_e alignment, uint64_t val) {
  return rozofs_64_padded_append(pChar, size, alignment, 0, val);
} 



#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif

