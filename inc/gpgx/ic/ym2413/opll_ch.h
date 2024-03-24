/**
 * File: ym2413.c - software implementation of YM2413
 *                  FM sound generator type OPLL
 *
 * Copyright (C) 2002 Jarek Burczynski
 *
 * Version 1.0
 */

#ifndef __GPGX_IC_YM2413_OPLL_CH_H__
#define __GPGX_IC_YM2413_OPLL_CH_H__

#include "xee/fnd/data_type.h"

#include "gpgx/ic/ym2413/opll_slot.h"

namespace gpgx::ic::ym2413 {

//==============================================================================

//------------------------------------------------------------------------------

struct OPLL_CH
{
  OPLL_SLOT SLOT[2];

  // phase generator state.

  u32 block_fnum; // Block + fnum
  u32 fc;         // Freq. freqement base.
  u32 ksl_base;   // KeyScaleLevel Base step.
  u8 kcode;       // Key code (for key scaling).
  u8 sus;         // Sus on/off (release speed in percussive mode).
};

} // namespace gpgx::ic::ym2413

#endif // #ifndef __GPGX_IC_YM2413_OPLL_CH_H__

