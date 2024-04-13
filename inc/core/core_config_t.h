/***************************************************************************************
 *  Genesis Plus
 *
 *  Copyright (C) 1998-2003  Charles Mac Donald (original code)
 *  Copyright (C) 2007-2024  Eke-Eke (Genesis Plus GX)
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************/

#ifndef __CORE_CORE_CONFIG_T_H__
#define __CORE_CORE_CONFIG_T_H__

#include "xee/fnd/data_type.h"

#include "core/input_hw/input.h" // For MAX_DEVICES.

//==============================================================================

//------------------------------------------------------------------------------

struct core_config_t
{
  // High quality FM synthesizer:
  // - 0 = OFF,
  // - 1 = ON
  u8 hq_fm;

  s16 fm_preamp;

  // Audio filtering:
  // - 0 = none,
  // - 1 = low-pass filter,
  // - 2 = 3 band equalization filter,
  u8 filter;

  // High quality PSG:
  // - 0 = OFF,
  // - 1 = ON
  u8 hq_psg;

  s16 psg_preamp;

  u8 ym2612;

  // YM2413:
  // - 0 = always OFF, 
  // - 1 = always ON,
  // - 2 = AUTO
  u8 ym2413;

  // YM3438:
  // - 0 = always OFF, 
  // - 1 = always ON
  u8 ym3438;

  u8 cd_latency;
  s16 cdda_volume;
  s16 pcm_volume;

  // Factor A of low-pass filter.
  u32 lp_range;

  // Low frequency of 3 band equalizer.
  s16 low_freq;

  // High frequency of 3 band equalizer.
  s16 high_freq;

  // Low gain of 3 band equalizer.
  s16 lg;

  // Middle gain of 3 band equalizer.
  s16 mg;

  // High gain of 3 band equalizer.
  s16 hg;

  // Mono output audio mixing:
  // - 0 = OFF,
  // - 1 = ON
  u8 mono;

  // Force console system:
  // - 0 = AUTO,
  // - SYSTEM_SG,
  // - SYSTEM_SGII,
  // - SYSTEM_SGII_RAM_EXT,
  // - SYSTEM_MARKIII,
  // - SYSTEM_SMS,
  // - SYSTEM_SMS2,
  // - SYSTEM_GG,
  // - SYSTEM_MD
  u8 system;

  // Force console region:
  // - 0 = AUTO,
  // - 1 = USA,
  // - 2 = EUROPE,
  // - 3 = JAPAN NTSC,
  // - 4 = JAPAN PAL
  u8 region_detect;

  // Force PAL / NTSC timings :
  // - 0 = AUTO,
  // - 1 = NTSC,
  // - 2 = PAL
  u8 vdp_mode;

  // Force PAL / NTSC master clock :
  // - 0 = AUTO,
  // - 1 = NTSC,
  // - 2 = PAL
  u8 master_clock; 

  u8 force_dtack;
  u8 addr_error;

  // - 0 = AUTO
  u8 bios;

  // Lock-on:
  // - 0 = OFF, 
  // - TYPE_GG = Game Genie, 
  // - TYPE_AR = (Pro) Action Replay, 
  // - TYPE_SK = Sonic & Knuckles
  u8 lock_on;

  // Add-on:
  // - 0 (HW_ADDON_AUTO) = AUTO, 
  // - HW_ADDON_MEGACD = Mega-CD, 
  // - HW_ADDON_MEGASD = Mega-SD, 
  // - HW_ADDON_NONE = None
  u8 add_on;

  // Overscan:
  // - 0 = no borders,
  // - 1 = vertical borders only,
  // - 2 = horizontal borders only,
  // - 3 = all borders
  u8 overscan;

  // Extended Game Gear screen:
  // - 0 = OFF,
  // - 1 = show extended Game Gear screen (256x192)
  u8 gg_extra;

  // - 0 = 0.8 fixed point
  u8 lcd;

  // - 1 = double resolution output (only when interlaced mode 2 is enabled)
  u8 render;
};

#endif // #ifndef __CORE_CORE_CONFIG_T_H__

