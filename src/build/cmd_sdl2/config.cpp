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

#include "build/cmd_sdl2/config.h"

#include "core/core_config.h"
#include "core/input_hw/input.h" // For input and MAX_DEVICES.
#include "gpgx/ic/ym2612/ym2612_type.h"

app_config_t app_config;

void set_config_defaults(void)
{
  /* sound options */
  core_config.psg_preamp     = 150;
  core_config.fm_preamp      = 100;
  core_config.cdda_volume    = 100;
  core_config.pcm_volume     = 100;
  core_config.hq_fm          = 1;
  core_config.hq_psg         = 1;
  core_config.filter         = 1;
  core_config.low_freq       = 200;
  core_config.high_freq      = 8000;
  core_config.lg             = 100;
  core_config.mg             = 100;
  core_config.hg             = 100;
  core_config.lp_range       = 0x9999; /* 0.6 in 0.16 fixed point */
  core_config.ym2612         = gpgx::ic::ym2612::YM2612_DISCRETE;
  core_config.ym2413         = 2; /* = AUTO (0 = always OFF, 1 = always ON) */
  core_config.ym3438         = 0;
  core_config.mono           = 0;

  /* system options */
  core_config.system         = 0; /* = AUTO (or SYSTEM_SG, SYSTEM_SGII, SYSTEM_SGII_RAM_EXT, SYSTEM_MARKIII, SYSTEM_SMS, SYSTEM_SMS2, SYSTEM_GG, SYSTEM_MD) */
  core_config.region_detect  = 0; /* = AUTO (1 = USA, 2 = EUROPE, 3 = JAPAN/NTSC, 4 = JAPAN/PAL) */
  core_config.vdp_mode       = 0; /* = AUTO (1 = NTSC, 2 = PAL) */
  core_config.master_clock   = 0; /* = AUTO (1 = NTSC, 2 = PAL) */
  core_config.force_dtack    = 0;
  core_config.addr_error     = 1;
  core_config.bios           = 0;
  core_config.lock_on        = 0; /* = OFF (or TYPE_SK, TYPE_GG & TYPE_AR) */
  core_config.add_on         = 0; /* = HW_ADDON_AUTO (or HW_ADDON_MEGACD, HW_ADDON_MEGASD & HW_ADDON_ONE) */
  core_config.cd_latency     = 1;

  /* display options */
  core_config.overscan = 0;  /* 3 = all borders (0 = no borders , 1 = vertical borders only, 2 = horizontal borders only) */
  core_config.gg_extra = 0;  /* 1 = show extended Game Gear screen (256x192) */
  core_config.lcd      = 0;  /* 0.8 fixed point */

  /* controllers options */
  input.system[0]       = SYSTEM_GAMEPAD;
  input.system[1]       = SYSTEM_GAMEPAD;
  app_config.gun_cursor[0]  = 1;
  app_config.gun_cursor[1]  = 1;
  app_config.invert_mouse   = 0;
}
