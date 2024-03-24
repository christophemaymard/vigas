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

#ifndef __GPGX_IC_YM2612_FM_CH_H__
#define __GPGX_IC_YM2612_FM_CH_H__

#include "xee/fnd/data_type.h"

#include "gpgx/ic/ym2612/fm_slot.h"

namespace gpgx::ic::ym2612 {

//==============================================================================

//------------------------------------------------------------------------------

struct FM_CH
{
  FM_SLOT  SLOT[4];     /* four SLOTs (operators) */

  u8   ALGO;         /* algorithm */
  u8   FB;           /* feedback shift */
  s32   op1_out[2];   /* op1 output for feedback */

  s32* connect1;    /* SLOT1 output pointer */
  s32* connect3;    /* SLOT3 output pointer */
  s32* connect2;    /* SLOT2 output pointer */
  s32* connect4;    /* SLOT4 output pointer */

  s32* mem_connect; /* where to put the delayed sample (MEM) */
  s32   mem_value;    /* delayed sample (MEM) value */

  s32   pms;          /* channel PMS */
  u8   ams;          /* channel AMS */

  u32  fc;           /* fnum,blk */
  u8   kcode;        /* key code */
  u32  block_fnum;   /* blk/fnum value (for LFO PM calculations) */
};

} // namespace gpgx::ic::ym2612

#endif // #ifndef __GPGX_IC_YM2612_FM_CH_H__

