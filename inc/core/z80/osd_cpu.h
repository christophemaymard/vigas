/*******************************************************************************
*                                                                              *
*  Define size independent data types and operations.                          *
*                                                                              *
*******************************************************************************/

#ifndef OSD_CPU_H
#define OSD_CPU_H

#include "xee/fnd/data_type.h"

#undef TRUE
#undef FALSE
#define TRUE  1
#define FALSE 0

/******************************************************************************
 * Union of u8, u16 and u32 in native endianess of the target
 * This is used to access bytes and words in a machine independent manner.
 * The upper bytes h2 and h3 normally contain zero (16 bit CPU cores)
 * thus PAIR.d can be used to pass arguments to the memory system
 * which expects 'int' really.
 ******************************************************************************/

typedef union {
#ifdef LSB_FIRST
  struct { u8 l,h,h2,h3; } b;
  struct { u16 l,h; } w;
#else
  struct { u8 h3,h2,h,l; } b;
  struct { u16 h,l; } w;
#endif
  u32 d;
}  PAIR;

#endif  /* defined OSD_CPU_H */
