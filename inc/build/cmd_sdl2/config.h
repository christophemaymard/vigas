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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "xee/fnd/data_type.h"

#include "build/cmd_sdl2/main.h"

/****************************************************************************
 * Config Option 
 *
 ****************************************************************************/
typedef struct 
{
  u8 padtype;
} t_input_config;

typedef struct 
{
  u8 hq_fm;
  u8 filter;
  u8 hq_psg;
  u8 ym2612;
  u8 ym2413;
  u8 ym3438;
  u8 cd_latency;
  s16 psg_preamp;
  s16 fm_preamp;
  s16 cdda_volume;
  s16 pcm_volume;
  u32 lp_range;
  s16 low_freq;
  s16 high_freq;
  s16 lg;
  s16 mg;
  s16 hg;
  u8 mono;
  u8 system;
  u8 region_detect;
  u8 vdp_mode;
  u8 master_clock;
  u8 force_dtack;
  u8 addr_error;
  u8 bios;
  u8 lock_on;
  u8 add_on;
  u8 hot_swap;
  u8 invert_mouse;
  u8 gun_cursor[2];
  u8 overscan;
  u8 gg_extra;
  u8 ntsc;
  u8 lcd;
  u8 render;
  u8 enhanced_vscroll;
  u8 enhanced_vscroll_limit;
  t_input_config input[MAX_INPUTS];
} t_config;

/* Global variables */
extern t_config config;
extern void set_config_defaults(void);

#endif /* _CONFIG_H_ */

