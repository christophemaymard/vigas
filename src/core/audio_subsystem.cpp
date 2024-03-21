/***************************************************************************************
 *  Genesis Plus
 *  Virtual System emulation
 *
 *  Support for 16-bit & 8-bit hardware modes
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

#include "core/audio_subsystem.h"

#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "osd.h"
#include "core/snd.h"
#include "core/system_clock.h"
#include "core/system_cycle.h"
#include "core/system_hardware.h"
#include "core/system_timing.h"
#include "core/vdp_ctrl.h"
#include "core/sound/sound.h"

#include "core/cd_hw/cdd.h"
#include "core/cd_hw/pcm.h"
#include "core/cd_hw/scd.h"

#include "gpgx/sound/blip_buffer.h"
#include "gpgx/sound/equalizer_3band.h"

//==============================================================================

//------------------------------------------------------------------------------

static gpgx::sound::Equalizer3band eq[2];
static s16 llp, rrp;

//==============================================================================

//------------------------------------------------------------------------------

int audio_init(int samplerate, f64 framerate)
{
  /* Shutdown first */
  audio_shutdown();

  /* Clear the sound data context */
  xee::mem::Memset(&snd, 0, sizeof(snd));

  /* Initialize Blip Buffers */
  snd.blips[0] = gpgx::sound::BlipBuffer::blip_new(samplerate / 10);
  if (!snd.blips[0]) {
    return -1;
  }

  /* Mega CD sound hardware */
  if (system_hw == SYSTEM_MCD) {
    /* allocate blip buffers */
    snd.blips[1] = gpgx::sound::BlipBuffer::blip_new(samplerate / 10);
    snd.blips[2] = gpgx::sound::BlipBuffer::blip_new(samplerate / 10);
    if (!snd.blips[1] || !snd.blips[2]) {
      audio_shutdown();
      return -1;
    }
  }

  /* Initialize resampler internal rates */
  audio_set_rate(samplerate, framerate);

  /* Set audio enable flag */
  snd.enabled = 1;

  /* Reset audio */
  audio_reset();

  return (0);
}

//------------------------------------------------------------------------------

void audio_set_rate(int samplerate, f64 framerate)
{
  /* Number of M-cycles executed per second. */
  /* All emulated chips are kept in sync by using a common oscillator (MCLOCK)            */
  /*                                                                                      */
  /* The original console would run exactly 53693175 M-cycles per sec (53203424 for PAL), */
  /* 3420 M-cycles per line and 262 (313 for PAL) lines per frame, which gives an exact   */
  /* framerate of 59.92 (49.70 for PAL) frames per second.                                */
  /*                                                                                      */
  /* Since audio samples are generated at the end of the frame, to prevent audio skipping */
  /* or lag between emulated frames, number of samples rendered per frame must be set to  */
  /* output samplerate (number of samples played per second) divided by input framerate   */
  /* (number of frames emulated per seconds).                                             */
  /*                                                                                      */
  /* On some systems, we may want to achieve 100% smooth video rendering by synchronizing */
  /* frame emulation with VSYNC, which frequency is generally not exactly those values.   */
  /* In that case, input framerate (number of frames emulated per seconds) is the same as */
  /* output framerate (number of frames rendered per seconds) by the host video hardware. */
  /*                                                                                      */
  /* When no framerate is specified, base clock is set to original master clock value.    */
  /* Otherwise, it is set to number of M-cycles emulated per line (fixed) multiplied by   */
  /* number of lines per frame (VDP mode specific) multiplied by input framerate.         */
  /*                                                                                      */
  f64 mclk = framerate ? (MCYCLES_PER_LINE * (vdp_pal ? 313 : 262) * framerate) : system_clock;

  /* For maximal accuracy, sound chips are running at their original rate using common */
  /* master clock timebase so they remain perfectly synchronized together, while still */
  /* being synchronized with 68K and Z80 CPUs as well. Mixed sound chip output is then */
  /* resampled to desired rate at the end of each frame, using Blip Buffer.            */
  snd.blips[0]->blip_set_rates(mclk, samplerate);

  /* Mega CD sound hardware enabled ? */
  if (snd.blips[1] && snd.blips[2]) {
    /* number of SCD master clocks run per second */
    mclk = (mclk / system_clock) * SCD_CLOCK;

    /* PCM core */
    pcm_init(mclk, samplerate);

    /* CDD core */
    cdd_init(samplerate);
  }

  /* Reinitialize internal rates */
  snd.sample_rate = samplerate;
  snd.frame_rate = framerate;
}

//------------------------------------------------------------------------------

void audio_reset(void)
{
  int i;

  /* Clear blip buffers */
  for (i = 0; i < 3; i++) {
    if (snd.blips[i]) {
      snd.blips[i]->blip_clear();
    }
  }

  /* Low-Pass filter */
  llp = 0;
  rrp = 0;

  /* 3 band EQ */
  audio_set_equalizer();
}

//------------------------------------------------------------------------------

void audio_set_equalizer(void)
{
  eq[0].init_3band_state(config.low_freq, config.high_freq, snd.sample_rate);
  eq[1].init_3band_state(config.low_freq, config.high_freq, snd.sample_rate);

  f64 lg = (f64)(config.lg) / 100.0;
  eq[0].SetLowGainControl(lg);
  eq[1].SetLowGainControl(lg);

  f64 mg = (f64)(config.mg) / 100.0;
  eq[0].SetMiddleGainControl(mg);
  eq[1].SetMiddleGainControl(mg);

  f64 hg = (f64)(config.hg) / 100.0;
  eq[0].SetHighGainControl(hg);
  eq[1].SetHighGainControl(hg);
}

//------------------------------------------------------------------------------

void audio_shutdown(void)
{
  int i;

  /* Delete blip buffers */
  for (i = 0; i < 3; i++) {
    if (snd.blips[i]) {
      snd.blips[i]->blip_delete();
      delete snd.blips[i];
      snd.blips[i] = nullptr;
    }
  }
}

//------------------------------------------------------------------------------

int audio_update(s16* buffer)
{
  /* run sound chips until end of frame */
  int size = sound_update(mcycles_vdp);

  /* Mega CD sound hardware enabled ? */
  if (snd.blips[1] && snd.blips[2]) {
    /* sync PCM chip with other sound chips */
    pcm_update(size);

    /* read CD-DA samples */
    cdd_update_audio(size);

#ifdef ALIGN_SND
    /* return an aligned number of samples if required */
    size &= ALIGN_SND;
#endif

    /* resample & mix FM/PSG, PCM & CD-DA streams to output buffer */
    snd.blips[0]->blip_mix_samples(snd.blips[1], snd.blips[2], buffer, size);
  } else {
#ifdef ALIGN_SND
    /* return an aligned number of samples if required */
    size &= ALIGN_SND;
#endif

    /* resample FM/PSG mixed stream to output buffer */
    snd.blips[0]->blip_read_samples(buffer, size);
  }

  /* Audio filtering */
  if (config.filter) {
    int samples = size;
    s16* out = buffer;
    s32 l, r;

    if (config.filter & 1) {
      /* single-pole low-pass filter (6 dB/octave) */
      u32 factora = config.lp_range;
      u32 factorb = 0x10000 - factora;

      /* restore previous sample */
      l = llp;
      r = rrp;

      do {
        /* apply low-pass filter */
        l = l * factora + out[0] * factorb;
        r = r * factora + out[1] * factorb;

        /* 16.16 fixed point */
        l >>= 16;
        r >>= 16;

        /* update sound buffer */
        *out++ = l;
        *out++ = r;
      } while (--samples);

      /* save last samples for next frame */
      llp = l;
      rrp = r;
    } else if (config.filter & 2) {
      do {
        /* 3 Band EQ */
        l = eq[0].do_3band(out[0]);
        r = eq[1].do_3band(out[1]);

        /* clipping (16-bit samples) */
        if (l > 32767) l = 32767;
        else if (l < -32768) l = -32768;
        if (r > 32767) r = 32767;
        else if (r < -32768) r = -32768;

        /* update sound buffer */
        *out++ = l;
        *out++ = r;
      } while (--samples);
    }
  }

  /* Mono output mixing */
  if (config.mono) {
    s16 out;
    int samples = size;
    do {
      out = (buffer[0] + buffer[1]) / 2;
      *buffer++ = out;
      *buffer++ = out;
    } while (--samples);
  }

#ifdef LOGSOUND
  error("%d samples returned\n\n", size);
#endif

  return size;
}
