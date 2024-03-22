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

#ifndef __GPGX_SOUND_YM2612_FM_SLOT_H__
#define __GPGX_SOUND_YM2612_FM_SLOT_H__

#include "xee/fnd/data_type.h"

namespace gpgx::sound {

//==============================================================================

//------------------------------------------------------------------------------

// struct describing a single operator (SLOT).
struct FM_SLOT
{
  s32* DT;        /* detune          :dt_tab[DT]      */
  u8   KSR;        /* key scale rate  :3-KSR           */
  u32  ar;         /* attack rate                      */
  u32  d1r;        /* decay rate                       */
  u32  d2r;        /* sustain rate                     */
  u32  rr;         /* release rate                     */
  u8   ksr;        /* key scale rate  :kcode>>(3-KSR)  */
  u32  mul;        /* multiple        :ML_TABLE[ML]    */

  // Phase Generator.

  u32  phase;      /* phase counter */
  s32   Incr;       /* phase step */

  // Envelope Generator.

  u8   state;      /* phase type */
  u32  tl;         /* total level: TL << 3 */
  s32   volume;     /* envelope counter */
  u32  sl;         /* sustain level:sl_table[SL] */
  u32  vol_out;    /* current output from EG circuit (without AM from LFO) */

  u8  eg_sh_ar;    /*  (attack state)  */
  u8  eg_sel_ar;   /*  (attack state)  */
  u8  eg_sh_d1r;   /*  (decay state)   */
  u8  eg_sel_d1r;  /*  (decay state)   */
  u8  eg_sh_d2r;   /*  (sustain state) */
  u8  eg_sel_d2r;  /*  (sustain state) */
  u8  eg_sh_rr;    /*  (release state) */
  u8  eg_sel_rr;   /*  (release state) */

  u8  ssg;         /* SSG-EG waveform  */
  u8  ssgn;        /* SSG-EG negated output  */

  u8  key;         /* 0=last key was KEY OFF, 1=KEY ON */

  /* LFO */
  u32  AMmask;     /* AM enable flag */
};

}

#endif // #ifndef __GPGX_SOUND_YM2612_FM_SLOT_H__

