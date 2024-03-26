/***************************************************************************************
 *  Genesis Plus
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

#include "gpgx/audio/audio_renderer.h"

#include "xee/mem/memory.h"

#include "osd.h"
#include "core/snd.h"
#include "core/state.h"
#include "core/system_cycle.h" // 
#include "core/system_hardware.h"
#include "core/io_ctrl.h" // For io_reg.
#include "core/cd_hw/cdd.h"
#include "core/cd_hw/pcm.h"

#include "gpgx/g_psg.h"
#include "gpgx/g_fm_synthesizer.h"
#include "gpgx/audio/effect/equalizer_3band.h"
#include "gpgx/audio/effect/null_fm_synthesizer.h"
#include "gpgx/ic/sn76489/sn76489.h"
#include "gpgx/ic/sn76489/sn76489_type.h"
#include "gpgx/ic/ym2413/ym2413.h"
#include "gpgx/ic/ym2612/ym2612.h"
#include "gpgx/ic/ym3438/ym3438.h"

namespace gpgx::audio {

//==============================================================================
// AudioRenderer

//------------------------------------------------------------------------------

AudioRenderer::AudioRenderer()
{
  m_fm_type = kFmTypeNone;

  xee::mem::Memset(m_fm_buffer, 0, sizeof(m_fm_buffer));

  llp = 0;
  rrp = 0;

  m_eq[0] = nullptr;
  m_eq[1] = nullptr;
}

//------------------------------------------------------------------------------

void AudioRenderer::Init()
{
  // Determine the type of FM synthesizer chip to initialize.
  s32 fm_type = 0;

  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD) {
    fm_type = config.ym3438 ? kFmTypeYm3438 : kFmTypeYm2612;
  } else {
    fm_type = config.ym2413 ? kFmTypeYm2413 : kFmTypeNull;
  }

  // Build a new FM synthesizer chip if the expected type to initialize is 
  // different from the current one.
  if (m_fm_type != fm_type) {
    RebuildFmSynthesizer(fm_type);
  }

  // Create PSG chip if necessary.
  if (!gpgx::g_psg) {
    gpgx::g_psg = new gpgx::ic::sn76489::Sn76489();
  }

  // Initialize PSG chip.
  gpgx::g_psg->psg_init((system_hw == SYSTEM_SG) ? gpgx::ic::sn76489::PSG_DISCRETE : gpgx::ic::sn76489::PSG_INTEGRATED);

  // Create equalizers if necessary.
  if (!m_eq[0]) {
    m_eq[0] = new gpgx::audio::effect::Equalizer3band();
    m_eq[1] = new gpgx::audio::effect::Equalizer3band();
  }
}

//------------------------------------------------------------------------------

void AudioRenderer::Destroy()
{
  if (m_eq[0]) {
    delete m_eq[0];
    m_eq[0] = nullptr;
  }

  if (m_eq[1]) {
    delete m_eq[1];
    m_eq[1] = nullptr;
  }

  if (!gpgx::g_psg) {
    delete gpgx::g_psg;
    gpgx::g_psg = nullptr;
  }

  if (gpgx::g_fm_synthesizer) {
    delete gpgx::g_fm_synthesizer;
    gpgx::g_fm_synthesizer = nullptr;
  }

  m_fm_type = kFmTypeNone;
}

//------------------------------------------------------------------------------

void AudioRenderer::ResetChips()
{
  // Reset FM synthesizer chip.
  gpgx::g_fm_synthesizer->Reset(m_fm_buffer);

  // Reset PSG chip.
  gpgx::g_psg->psg_reset();

  // Maybe the panning argument (0xff) should be determined as it is done in LoadContext().
  gpgx::g_psg->psg_config(0, config.psg_preamp, 0xff);
}

//------------------------------------------------------------------------------

void AudioRenderer::ResetLowPassFilter()
{
  // Reset last samples used in low-passs filter.
  llp = 0;
  rrp = 0;
}

//------------------------------------------------------------------------------

void AudioRenderer::ApplyEqualizationSettings()
{
  m_eq[0]->init_3band_state(config.low_freq, config.high_freq, snd.sample_rate);
  m_eq[1]->init_3band_state(config.low_freq, config.high_freq, snd.sample_rate);

  f64 lg = (f64)(config.lg) / 100.0;
  m_eq[0]->SetLowGainControl(lg);
  m_eq[1]->SetLowGainControl(lg);

  f64 mg = (f64)(config.mg) / 100.0;
  m_eq[0]->SetMiddleGainControl(mg);
  m_eq[1]->SetMiddleGainControl(mg);

  f64 hg = (f64)(config.hg) / 100.0;
  m_eq[0]->SetHighGainControl(hg);
  m_eq[1]->SetHighGainControl(hg);
}

//------------------------------------------------------------------------------

s32 AudioRenderer::Update(s16* output_buffer)
{
  u32 cycles = mcycles_vdp;

  // Run PSG chip until end of frame.
  gpgx::g_psg->psg_end_frame(cycles);

  // Run FM synthesizer chip until end of frame.
  gpgx::g_fm_synthesizer->EndFrame(cycles);

  // End of blip buffer time frame.
  snd.blips[0]->blip_end_frame(cycles);

  // The number of available samples.
  s32 size = snd.blips[0]->blip_samples_avail();

  // Mega CD sound hardware enabled ?
  if (snd.blips[1] && snd.blips[2]) {
    // sync PCM chip with other sound chips.
    pcm_update(size);

    // read CD-DA samples.
    cdd_update_audio(size);

#ifdef ALIGN_SND
    // return an aligned number of samples if required.
    size &= ALIGN_SND;
#endif

    // resample & mix FM/PSG, PCM & CD-DA streams to output buffer.
    snd.blips[0]->blip_mix_samples(snd.blips[1], snd.blips[2], output_buffer, size);
  } else {
#ifdef ALIGN_SND
    // return an aligned number of samples if required.
    size &= ALIGN_SND;
#endif

    // resample FM/PSG mixed stream to output buffer.
    snd.blips[0]->blip_read_samples(output_buffer, size);
  }

  // Audio filtering.
  if (config.filter) {
    int samples = size;
    s16* out = output_buffer;
    s32 l, r;

    // Use low-pass filtering ?
    if (config.filter & 1) {
      // single-pole low-pass filter (6 dB/octave).
      u32 factora = config.lp_range;
      u32 factorb = 0x10000 - factora;

      // restore previous sample.
      l = llp;
      r = rrp;

      do {
        // apply low-pass filter.
        l = l * factora + out[0] * factorb;
        r = r * factora + out[1] * factorb;

        // 16.16 fixed point.
        l >>= 16;
        r >>= 16;

        // update sound buffer.
        *out++ = l;
        *out++ = r;
      } while (--samples);

      // save last samples for next frame.
      llp = l;
      rrp = r;
    } 
    // Use equalization filtering ?
    else if (config.filter & 2) {
      do {
        // 3 Band EQ.
        l = m_eq[0]->do_3band(out[0]);
        r = m_eq[1]->do_3band(out[1]);

        // clipping (16-bit samples).
        if (l > 32767) l = 32767;
        else if (l < -32768) l = -32768;
        if (r > 32767) r = 32767;
        else if (r < -32768) r = -32768;

        // update sound buffer.
        *out++ = l;
        *out++ = r;
      } while (--samples);
    }
  }

  // Mono output mixing.
  if (config.mono) {
    s16 out;
    int samples = size;
    do {
      out = (output_buffer[0] + output_buffer[1]) / 2;
      *output_buffer++ = out;
      *output_buffer++ = out;
    } while (--samples);
  }

#ifdef LOGSOUND
  error("%d samples returned\n\n", size);
#endif

  return size;
}

//------------------------------------------------------------------------------

s32 AudioRenderer::LoadContext(u8* state)
{
  int bufferptr = 0;

  // Determine the type of FM synthesizer chip to initialize.
  s32 fm_type = 0;

  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD) {
    u8 config_ym3438;
    load_param(&config_ym3438, sizeof(config_ym3438));

    fm_type = config_ym3438 ? kFmTypeYm3438 : kFmTypeYm2612;
  } else {
    u8 config_ym2413;
    load_param(&config_ym2413, sizeof(config_ym2413));

    fm_type = config_ym2413 ? kFmTypeYm2413 : kFmTypeNull;
  }

  // Build a new FM synthesizer chip if the expected type to initialize is 
  // different from the current one.
  if (m_fm_type != fm_type) {
    RebuildFmSynthesizer(fm_type);
  }

  // Load the context of the FM synthesizer.
  // If it is "Null", nothing will be loaded.
  bufferptr += gpgx::g_fm_synthesizer->LoadContext(&state[bufferptr]);

  bufferptr += gpgx::g_psg->psg_context_load(&state[bufferptr]);

  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD) {
    gpgx::g_psg->psg_config(0, config.psg_preamp, 0xff);
  } else {
    gpgx::g_psg->psg_config(0, config.psg_preamp, io_reg[6]);
  }

  return bufferptr;
}

//------------------------------------------------------------------------------

s32 AudioRenderer::SaveContext(u8* state)
{
  int bufferptr = 0;

  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD) {
    save_param(&config.ym3438, sizeof(config.ym3438));
  } else {
    save_param(&config.ym2413, sizeof(config.ym2413));
  }

  // Save the context of the FM synthesizer.
  // If it is "Null", nothing will be saved.
  bufferptr += gpgx::g_fm_synthesizer->SaveContext(&state[bufferptr]);

  // Save the context of the PSG.
  bufferptr += gpgx::g_psg->psg_context_save(&state[bufferptr]);

  return bufferptr;
}

//------------------------------------------------------------------------------

void AudioRenderer::RebuildFmSynthesizer(s32 fm_type)
{
  if (gpgx::g_fm_synthesizer) {
    delete gpgx::g_fm_synthesizer;
    gpgx::g_fm_synthesizer = nullptr;
  }

  switch (fm_type) {
    case kFmTypeYm2413:
    {
      gpgx::g_fm_synthesizer = CreateYm2413FmSynthesizer();

      break;
    }
    case kFmTypeYm2612:
    {
      gpgx::g_fm_synthesizer = CreateYm2612FmSynthesizer();

      break;
    }
    case kFmTypeYm3438:
    {
      gpgx::g_fm_synthesizer = CreateYm3438FmSynthesizer();

      break;
    }
    default:
    {
      gpgx::g_fm_synthesizer = new gpgx::audio::effect::NullFmSynthesizer();

      break;
    }
  }

  // Reset the FM synthesizer.
  gpgx::g_fm_synthesizer->Reset(m_fm_buffer);

  // Update the type of the current FM synthesizer.
  m_fm_type = fm_type;
}

//------------------------------------------------------------------------------

gpgx::ic::ym2413::Ym2413* AudioRenderer::CreateYm2413FmSynthesizer()
{
  gpgx::ic::ym2413::Ym2413* ym2413 = new gpgx::ic::ym2413::Ym2413();

  ym2413->YM2413Init();

  // chip is running at ZCLK / 72 = MCLK / 15 / 72.
  ym2413->SetClockRatio(72 * 15);

  return ym2413;
}

//------------------------------------------------------------------------------

gpgx::ic::ym2612::Ym2612* AudioRenderer::CreateYm2612FmSynthesizer()
{
  gpgx::ic::ym2612::Ym2612* ym2612 = new gpgx::ic::ym2612::Ym2612();

  ym2612->YM2612Init();
  ym2612->YM2612Config(config.ym2612);

  // chip is running at sample clock.
  ym2612->SetClockRatio(gpgx::ic::ym2612::Ym2612::kYm2612ClockRatio * 24);

  return ym2612;
}

//------------------------------------------------------------------------------

gpgx::ic::ym3438::Ym3438* AudioRenderer::CreateYm3438FmSynthesizer()
{
  gpgx::ic::ym3438::Ym3438* ym3438 = new gpgx::ic::ym3438::Ym3438();

  ym3438->Init();

  // chip is running at internal clock.
  ym3438->SetClockRatio(gpgx::ic::ym2612::Ym2612::kYm2612ClockRatio);

  return ym3438;
}

} // namespace gpgx::audio

