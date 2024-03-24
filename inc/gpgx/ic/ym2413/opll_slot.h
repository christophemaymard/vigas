/**
 * File: ym2413.c - software implementation of YM2413
 *                  FM sound generator type OPLL
 *
 * Copyright (C) 2002 Jarek Burczynski
 *
 * Version 1.0
 */

#ifndef __GPGX_IC_YM2413_OPLL_SLOT_H__
#define __GPGX_IC_YM2413_OPLL_SLOT_H__

#include "xee/fnd/data_type.h"

namespace gpgx::ic::ym2413 {

//==============================================================================

//------------------------------------------------------------------------------

struct  OPLL_SLOT
{
  u32 ar; // attack rate: AR << 2
  u32 dr; // decay rate:  DR << 2
  u32 rr; // release rate:RR << 2
  u8 KSR; // key scale rate.
  u8 ksl; // keyscale level.
  u8 ksr; // key scale rate: kcode >> KSR.
  u8 mul; // multiple: mul_tab[ML]

  // Phase Generator.

  u32 phase;      // frequency counter.
  u32 freq;       // frequency counter step.
  u8 fb_shift;    // feedback shift value.
  s32 op1_out[2]; // slot1 output for feedback.

  // Envelope Generator.

  u8 eg_type;  // percussive/nonpercussive mode.
  u8 state;    // phase type.
  u32 TL;      // total level: TL << 2
  s32 TLL;     // adjusted now TL.
  s32 volume;  // envelope counter.
  u32 sl;      // sustain level: sl_tab[SL]

  u8 eg_sh_dp;    // (dump state)
  u8 eg_sel_dp;   // (dump state)
  u8 eg_sh_ar;    // (attack state)
  u16 eg_sel_ar;  // (attack state)
  u8 eg_sh_dr;    // (decay state)
  u8 eg_sel_dr;   // (decay state)
  u8 eg_sh_rr;    // (release state for non-perc.)
  u8 eg_sel_rr;   // (release state for non-perc.)
  u8 eg_sh_rs;    // (release state for perc.mode)
  u8 eg_sel_rs;   // (release state for perc.mode)

  u32 key; // 0 = KEY OFF, >0 = KEY ON

  // LFO.

  u32 AMmask; // LFO Amplitude Modulation enable mask.
  u8 vib;     // LFO Phase Modulation enable flag (active high).

  // waveform select.
  unsigned int wavetable;
};

} // namespace gpgx::ic::ym2413

#endif // #ifndef __GPGX_IC_YM2413_OPLL_SLOT_H__

