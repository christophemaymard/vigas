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

#include <new> // For std::nothrow.

#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "osd.h"
#include "core/snd.h"
#include "core/system_clock.h"
#include "core/system_cycle.h"
#include "core/system_hardware.h"
#include "core/system_timing.h"
#include "core/vdp_ctrl.h"

#include "core/cd_hw/cdd.h" // For cdd_init()
#include "core/cd_hw/pcm.h" // For pcm_init()
#include "core/cd_hw/scd.h" // For SCD_CLOCK

#include "gpgx/g_audio_renderer.h"
#include "gpgx/audio/audio_renderer.h"
#include "gpgx/audio/blip_buffer.h"

//==============================================================================

//------------------------------------------------------------------------------

int audio_init(int samplerate, f64 framerate)
{
  /* Shutdown first */
  audio_shutdown();

  /* Clear the sound data context */
  xee::mem::Memset(&snd, 0, sizeof(snd));

  /* Initialize Blip Buffers */
  snd.blips[0] = gpgx::audio::BlipBuffer::blip_new(samplerate / 10);
  if (!snd.blips[0]) {
    return -1;
  }

  /* Mega CD sound hardware */
  if (system_hw == SYSTEM_MCD) {
    /* allocate blip buffers */
    snd.blips[1] = gpgx::audio::BlipBuffer::blip_new(samplerate / 10);
    snd.blips[2] = gpgx::audio::BlipBuffer::blip_new(samplerate / 10);
    if (!snd.blips[1] || !snd.blips[2]) {
      audio_shutdown();
      return -1;
    }
  }

  /* Initialize resampler internal rates */
  audio_set_rate(samplerate, framerate);

  /* Set audio enable flag */
  snd.enabled = 1;

  // Create and initialize the audio renderer.
  gpgx::g_audio_renderer = new (std::nothrow) gpgx::audio::AudioRenderer();

  if (!gpgx::g_audio_renderer) {
    audio_shutdown();

    return -1;
  }

  gpgx::g_audio_renderer->Init();

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
  gpgx::g_audio_renderer->ResetLowPassFilter();

  /* 3 band EQ */
  gpgx::g_audio_renderer->ApplyEqualizationSettings();
}

//------------------------------------------------------------------------------

void audio_shutdown(void)
{
  int i;

  // Delete the audio renderer.
  if (gpgx::g_audio_renderer) {
    gpgx::g_audio_renderer->Destroy();
    delete gpgx::g_audio_renderer;
    gpgx::g_audio_renderer = nullptr;
  }

  /* Delete blip buffers */
  for (i = 0; i < 3; i++) {
    if (snd.blips[i]) {
      snd.blips[i]->blip_delete();
      delete snd.blips[i];
      snd.blips[i] = nullptr;
    }
  }
}

