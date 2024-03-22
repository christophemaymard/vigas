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

#ifndef __GPGX_SOUND_YM2612_FM_OPN_H__
#define __GPGX_SOUND_YM2612_FM_OPN_H__

#include "xee/fnd/data_type.h"

#include "gpgx/sound/ym2612/fm_st.h"
#include "gpgx/sound/ym2612/fm_3slot.h"

namespace gpgx::sound {

//==============================================================================

//------------------------------------------------------------------------------

// OPN/A/B common state.
struct FM_OPN
{
  FM_ST  ST;                  /* general state */
  FM_3SLOT SL3;               /* 3 slot mode state */
  unsigned int pan[6 * 2];      /* fm channels output masks (0xffffffff = enable) */

  // EG.

  u32  eg_cnt;             /* global envelope generator counter */
  u32  eg_timer;           /* global envelope generator counter works at frequency = chipclock/144/3 */

  // LFO.

  u8   lfo_cnt;            /* current LFO phase (out of 128) */
  u32  lfo_timer;          /* current LFO phase runs at LFO frequency */
  u32  lfo_timer_overflow; /* LFO timer overflows every N samples (depends on LFO frequency) */
  u32  LFO_AM;             /* current LFO AM step */
  u32  LFO_PM;             /* current LFO PM step */
};

}

#endif // #ifndef __GPGX_SOUND_YM2612_FM_OPN_H__

