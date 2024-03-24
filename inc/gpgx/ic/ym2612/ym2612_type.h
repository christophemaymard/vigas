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

#ifndef __GPGX_IC_YM2612_YM2612_TYPE_H__
#define __GPGX_IC_YM2612_YM2612_TYPE_H__

namespace gpgx::ic::ym2612 {

//==============================================================================

//------------------------------------------------------------------------------

enum YM2612_TYPE
{
  YM2612_DISCRETE = 0,
  YM2612_INTEGRATED,
  YM2612_ENHANCED
};

} // namespace gpgx::ic::ym2612

#endif // #ifndef __GPGX_IC_YM2612_YM2612_TYPE_H__

