/**
 * software implementation of Yamaha FM sound generator (YM2612/YM3438)
 *
 * Original code (MAME fm.c)
 *
 * Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
 * Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
 *
 * Version 1.4 (final beta)
 *
 * Additional code & fixes by Eke-Eke for Genesis Plus GX
 */

#ifndef __GPGX_SOUND_YM2612_FM_ST_H__
#define __GPGX_SOUND_YM2612_FM_ST_H__

#include "xee/fnd/data_type.h"

namespace gpgx::sound {

//==============================================================================

//------------------------------------------------------------------------------

struct FM_ST
{
  u16 address;        // address register.
  u8 status;          // status flag.
  u32 mode;           // mode  CSM / 3SLOT.
  u8 fn_h;            // freq latch.
  s32 TA;             // timer a value.
  s32 TAL;            // timer a base.
  s32 TAC;            // timer a counter.
  s32 TB;             // timer b value.
  s32 TBL;            // timer b base.
  s32 TBC;            // timer b counter.
  s32 dt_tab[8][32];  // DeTune table.
};

}

#endif // #ifndef __GPGX_SOUND_YM2612_FM_ST_H__

