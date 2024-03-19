
#ifndef _TYPES_H_
#define _TYPES_H_

#include "xee/fnd/data_type.h"

typedef union
{
    u16 w;
    struct
    {
#ifdef LSB_FIRST
        u8 l;
        u8 h;
#else
        u8 h;
        u8 l;
#endif
    } byte;

} reg16_t;

#endif /* _TYPES_H_ */
