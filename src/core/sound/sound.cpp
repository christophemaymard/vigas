/***************************************************************************************
 *  Genesis Plus
 *  Sound Hardware
 *
 *  Copyright (C) 1998-2003  Charles Mac Donald (original code)
 *  Copyright (C) 2007-2020  Eke-Eke (Genesis Plus GX)
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

#include "core/sound/sound.h"

#include "xee/fnd/compiler.h"
#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "osd.h"
#include "core/snd.h"
#include "core/system_hardware.h"
#include "core/state.h"

#include "gpgx/g_psg.h"
#include "gpgx/g_ym2413.h"
#include "gpgx/g_ym2612.h"
#include "gpgx/g_ym3438.h"
#include "gpgx/ic/sn76489/sn76489.h"
#include "gpgx/ic/ym2612/ym2612_type.h"

/* YM2612 internal clock = input clock / 6 = (master clock / 7) / 6 */
#define YM2612_CLOCK_RATIO (7*6)

/* FM output buffer (large enough to hold a whole frame at original chips rate) */
static int fm_buffer[1080 * 2 * 24];

static int fm_last[2];
static int *fm_ptr;

/* Cycle-accurate FM samples */
static int fm_cycles_ratio;
static int fm_cycles_start;
static int fm_cycles_count;
static int fm_cycles_busy;

/* YM chip function pointers */
static void (*YM_Update)(int *buffer, int length);
void (*fm_reset)(unsigned int cycles);
void (*fm_write)(unsigned int cycles, unsigned int address, unsigned int data);
unsigned int (*fm_read)(unsigned int cycles, unsigned int address);

/* Run FM chip until required M-cycles */
static XEE_INLINE void fm_update(int cycles)
{
  if (cycles > fm_cycles_count)
  {
    /* number of samples to run */
    int samples = (cycles - fm_cycles_count + fm_cycles_ratio - 1) / fm_cycles_ratio;

    /* run FM chip to sample buffer */
    YM_Update(fm_ptr, samples);

    /* update FM buffer pointer */
    fm_ptr += (samples * 2);

    /* update FM cycle counter */
    fm_cycles_count += (samples * fm_cycles_ratio);
  }
}

static void YM2612_Update(int* buffer, int length)
{
  gpgx::g_ym2612->YM2612Update(buffer, length);
}

static void YM2612_Reset(unsigned int cycles)
{
  /* synchronize FM chip with CPU */
  fm_update(cycles);

  /* reset FM chip */
  gpgx::g_ym2612->YM2612ResetChip();
  fm_cycles_busy = 0;
}

static void YM2612_Write(unsigned int cycles, unsigned int a, unsigned int v)
{
  /* detect DATA port write */
  if (a & 1)
  {
    /* synchronize FM chip with CPU */
    fm_update(cycles);

    /* set FM BUSY end cycle (discrete or ASIC-integrated YM2612 chip only) */
    if (config.ym2612 < gpgx::ic::ym2612::YM2612_ENHANCED)
    {
      fm_cycles_busy = (((cycles + YM2612_CLOCK_RATIO - 1) / YM2612_CLOCK_RATIO) + 32) * YM2612_CLOCK_RATIO;
    }
  }

  /* write FM register */
  gpgx::g_ym2612->YM2612Write(a, v);
}

static unsigned int YM2612_Read(unsigned int cycles, unsigned int a)
{
  /* FM status can only be read from (A0,A1)=(0,0) on discrete YM2612 */
  if ((a == 0) || (config.ym2612 > gpgx::ic::ym2612::YM2612_DISCRETE))
  {
    /* synchronize FM chip with CPU */
    fm_update(cycles);

    /* read FM status */
    if (cycles >= fm_cycles_busy)
    {
      /* BUSY flag cleared */
      return gpgx::g_ym2612->YM2612Read();
    }
    else
    {
      /* BUSY flag set */
      return gpgx::g_ym2612->YM2612Read() | 0x80;
    }
  }

  /* invalid FM status address */
  return 0x00;
}

static void YM2413_Update(int* buffer, int length)
{
  gpgx::g_ym2413->YM2413Update(buffer, length);
}

static void YM2413_Reset(unsigned int cycles)
{
  /* synchronize FM chip with CPU */
  fm_update(cycles);

  /* reset FM chip */
  gpgx::g_ym2413->YM2413ResetChip();
}

static void YM2413_Write(unsigned int cycles, unsigned int a, unsigned int v)
{
  /* detect DATA port write */
  if (a & 1)
  {
    /* synchronize FM chip with CPU */
    fm_update(cycles);
  }

  /* write FM register */
  gpgx::g_ym2413->YM2413Write(a, v);
}

static unsigned int YM2413_Read(unsigned int cycles, unsigned int a)
{
    return gpgx::g_ym2413->YM2413Read();
}

static void YM3438_Update(int *buffer, int length)
{
  gpgx::g_ym3438->Update(buffer, length);
}

static void YM3438_Reset(unsigned int cycles)
{
  /* synchronize FM chip with CPU */
  fm_update(cycles);

  /* reset FM chip */
  gpgx::g_ym3438->OPN2_Reset();
}

static void YM3438_Write(unsigned int cycles, unsigned int a, unsigned int v)
{
  /* synchronize FM chip with CPU */
  fm_update(cycles);

  /* write FM register */
  gpgx::g_ym3438->OPN2_Write(a, v);
}

static unsigned int YM3438_Read(unsigned int cycles, unsigned int a)
{
  /* synchronize FM chip with CPU */
  fm_update(cycles);

  /* read FM status */
  return gpgx::g_ym3438->OPN2_Read(a);
}

void sound_init( void )
{
  /* Initialize FM chip */
  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
  {
    /* YM2612 */
    if (config.ym3438)
    {
      /* Nuked OPN2 */
      gpgx::g_ym3438->Init();
      YM_Update = YM3438_Update;
      fm_reset = YM3438_Reset;
      fm_write = YM3438_Write;
      fm_read = YM3438_Read;

      /* chip is running at internal clock */
      fm_cycles_ratio = YM2612_CLOCK_RATIO;
    }
    else
    {
      /* MAME OPN2*/
      gpgx::g_ym2612->YM2612Init();
      gpgx::g_ym2612->YM2612Config(config.ym2612);
      YM_Update = YM2612_Update;
      fm_reset = YM2612_Reset;
      fm_write = YM2612_Write;
      fm_read = YM2612_Read;

      /* chip is running at sample clock */
      fm_cycles_ratio = YM2612_CLOCK_RATIO * 24;
    }
  }
  else
  {
    /* YM2413 */
    {
      gpgx::g_ym2413->YM2413Init();
      YM_Update = (config.ym2413 & 1) ? YM2413_Update : NULL;
      fm_reset = YM2413_Reset;
      fm_write = YM2413_Write;
      fm_read = YM2413_Read;

      /* chip is running at ZCLK / 72 = MCLK / 15 / 72 */
      fm_cycles_ratio = 72 * 15;
    }
  }

  /* Initialize PSG chip */
  gpgx::g_psg->psg_init((system_hw == SYSTEM_SG) ? gpgx::ic::sn76489::PSG_DISCRETE : gpgx::ic::sn76489::PSG_INTEGRATED);
}

void sound_reset(void)
{
  /* reset sound chips */
  fm_reset(0);
  gpgx::g_psg->psg_reset();
  gpgx::g_psg->psg_config(0, config.psg_preamp, 0xff);

  /* reset FM buffer ouput */
  fm_last[0] = fm_last[1] = 0;

  /* reset FM buffer pointer */
  fm_ptr = fm_buffer;
  
  /* reset FM cycle counters */
  fm_cycles_start = fm_cycles_count = 0;
}

int sound_update(unsigned int cycles)
{
  /* Run PSG chip until end of frame */
  gpgx::g_psg->psg_end_frame(cycles);

  /* FM chip is enabled ? */
  if (YM_Update)
  {
    int prev_l, prev_r, preamp, time, l, r, *ptr;

    /* Run FM chip until end of frame */
    fm_update(cycles);

    /* FM output pre-amplification */
    preamp = config.fm_preamp;

    /* FM frame initial timestamp */
    time = fm_cycles_start;

    /* Restore last FM outputs from previous frame */
    prev_l = fm_last[0];
    prev_r = fm_last[1];

    /* FM buffer start pointer */
    ptr = fm_buffer;

    /* flush FM samples */
    if (config.hq_fm)
    {
      /* high-quality Band-Limited synthesis */
      do
      {
        /* left & right channels */
        l = ((*ptr++ * preamp) / 100);
        r = ((*ptr++ * preamp) / 100);
        snd.blips[0]->blip_add_delta(time, l - prev_l, r - prev_r);
        prev_l = l;
        prev_r = r;

        /* increment time counter */
        time += fm_cycles_ratio;
      }
      while (time < cycles);
    }
    else
    {
      /* faster Linear Interpolation */
      do
      {
        /* left & right channels */
        l = ((*ptr++ * preamp) / 100);
        r = ((*ptr++ * preamp) / 100);
        snd.blips[0]->blip_add_delta_fast(time, l - prev_l, r - prev_r);
        prev_l = l;
        prev_r = r;

        /* increment time counter */
        time += fm_cycles_ratio;
      }
      while (time < cycles);
    }

    /* reset FM buffer pointer */
    fm_ptr = fm_buffer;

    /* save last FM output for next frame */
    fm_last[0] = prev_l;
    fm_last[1] = prev_r;

    /* adjust FM cycle counters for next frame */
    fm_cycles_count = fm_cycles_start = time - cycles;
    if (fm_cycles_busy > cycles)
    {
      fm_cycles_busy -= cycles;
    }
    else
    {
      fm_cycles_busy = 0;
    }
  }

  /* end of blip buffer time frame */
  snd.blips[0]->blip_end_frame(cycles);

  /* return number of available samples */
  return snd.blips[0]->blip_samples_avail();
}

int sound_context_save(u8 *state)
{
  int bufferptr = 0;
  
  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
  {
    save_param(&config.ym3438, sizeof(config.ym3438));
    if (config.ym3438)
    {
      bufferptr += gpgx::g_ym3438->SaveContext(&state[bufferptr]);
    }
    else
    {
      bufferptr += gpgx::g_ym2612->YM2612SaveContext(state + sizeof(config.ym3438));
    }
  }
  else
  {
    {
      bufferptr += gpgx::g_ym2413->SaveContext(&state[bufferptr]);
    }
  }

  bufferptr += gpgx::g_psg->psg_context_save(&state[bufferptr]);

  save_param(&fm_cycles_start,sizeof(fm_cycles_start));

  return bufferptr;
}

int sound_context_load(u8 *state)
{
  int bufferptr = 0;

  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
  {
    u8 config_ym3438;
    load_param(&config_ym3438, sizeof(config_ym3438));
    if (config_ym3438)
    {
      bufferptr += gpgx::g_ym3438->LoadContext(&state[bufferptr]);
    }
    else
    {
      bufferptr += gpgx::g_ym2612->YM2612LoadContext(state + sizeof(config_ym3438));
    }
  }
  else
  {
    {
      bufferptr += gpgx::g_ym2413->LoadContext(&state[bufferptr]);
    }
  }

  bufferptr += gpgx::g_psg->psg_context_load(&state[bufferptr]);

  load_param(&fm_cycles_start,sizeof(fm_cycles_start));
  fm_cycles_count = fm_cycles_start;

  return bufferptr;
}
