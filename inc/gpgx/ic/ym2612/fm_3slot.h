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

#ifndef __GPGX_IC_YM2612_FM_3SLOT_H__
#define __GPGX_IC_YM2612_FM_3SLOT_H__

#include "xee/fnd/data_type.h"

namespace gpgx::ic::ym2612 {

//==============================================================================

//------------------------------------------------------------------------------

// OPN 3slot struct.
struct FM_3SLOT
{
  u32  fc[3];          // fnum3,blk3: calculated.
  u8   fn_h;           // freq3 latch.
  u8   kcode[3];       // key code.
  u32  block_fnum[3];  // current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode).
  u8   key_csm;        // CSM mode Key-ON flag.
};

} // namespace gpgx::ic::ym2612

#endif // #ifndef __GPGX_IC_YM2612_FM_3SLOT_H__

