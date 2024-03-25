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

#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "osd.h"
#include "core/snd.h"
#include "core/system_hardware.h"
#include "core/state.h"

#include "gpgx/g_psg.h"
#include "gpgx/g_fm_synthesizer.h"
#include "gpgx/audio/effect/null_fm_synthesizer.h"
#include "gpgx/ic/sn76489/sn76489_type.h"
#include "gpgx/ic/ym2413/ym2413.h"
#include "gpgx/ic/ym2612/ym2612.h"
#include "gpgx/ic/ym2612/ym2612_type.h"
#include "gpgx/ic/ym3438/ym3438.h"

/* FM output buffer (large enough to hold a whole frame at original chips rate) */
static int fm_buffer[1080 * 2 * 24];

// Creates and initializes a YM2413 FM synthesizer. 
static gpgx::ic::ym2413::Ym2413* sound_create_ym2413()
{
  gpgx::ic::ym2413::Ym2413* ym2413 = new gpgx::ic::ym2413::Ym2413();

  ym2413->YM2413Init();

  // chip is running at ZCLK / 72 = MCLK / 15 / 72.
  ym2413->SetClockRatio(72 * 15);

  // Reset the FM synthesizer.
  ym2413->Reset(fm_buffer);

  return ym2413;
}

// Creates and initializes a YM2612 (MAME OPN2) FM synthesizer. 
static gpgx::ic::ym2612::Ym2612* sound_create_ym2612()
{
  gpgx::ic::ym2612::Ym2612* ym2612 = new gpgx::ic::ym2612::Ym2612();

  ym2612->YM2612Init();
  ym2612->YM2612Config(config.ym2612);

  // chip is running at sample clock.
  ym2612->SetClockRatio(gpgx::ic::ym2612::Ym2612::kYm2612ClockRatio * 24);

  // Reset the FM synthesizer.
  ym2612->Reset(fm_buffer);

  return ym2612;
}

// Creates and initializes a YM3438 (Nuked OPN2) FM synthesizer. 
static gpgx::ic::ym3438::Ym3438* sound_create_ym3438()
{
  gpgx::ic::ym3438::Ym3438* ym3438 = new gpgx::ic::ym3438::Ym3438();

  ym3438->Init();

  // chip is running at internal clock.
  ym3438->SetClockRatio(gpgx::ic::ym2612::Ym2612::kYm2612ClockRatio);

  // Reset the FM synthesizer.
  ym3438->Reset(fm_buffer);

  return ym3438;
}

void sound_init( void )
{
  // Delete current FM synthesizer if present.
  if (gpgx::g_fm_synthesizer) {
    delete gpgx::g_fm_synthesizer;
    gpgx::g_fm_synthesizer = nullptr;
  }

  // Initialize FM synthesizer.
  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD) {
    // YM3438.
    if (config.ym3438) {
      gpgx::g_fm_synthesizer = sound_create_ym3438();
    } else {
      gpgx::g_fm_synthesizer = sound_create_ym2612();
    }
  } else {
    // YM2413.
    if (config.ym2413) {
      gpgx::g_fm_synthesizer = sound_create_ym2413();
    } else {
      // Define "Null" as the FM synthesizer.
      gpgx::g_fm_synthesizer = new gpgx::audio::effect::NullFmSynthesizer();
    }
  }

  /* Initialize PSG chip */
  gpgx::g_psg->psg_init((system_hw == SYSTEM_SG) ? gpgx::ic::sn76489::PSG_DISCRETE : gpgx::ic::sn76489::PSG_INTEGRATED);
}

void sound_reset(void)
{
  /* reset sound chips */
  gpgx::g_fm_synthesizer->Reset(fm_buffer);
  gpgx::g_psg->psg_reset();
  gpgx::g_psg->psg_config(0, config.psg_preamp, 0xff);
}

int sound_update(unsigned int cycles)
{
  /* Run PSG chip until end of frame */
  gpgx::g_psg->psg_end_frame(cycles);

  // Run FM synthesizer chip until end of frame.
  gpgx::g_fm_synthesizer->EndFrame(cycles);

  /* end of blip buffer time frame */
  snd.blips[0]->blip_end_frame(cycles);

  /* return number of available samples */
  return snd.blips[0]->blip_samples_avail();
}

int sound_context_save(u8 *state)
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

  bufferptr += gpgx::g_psg->psg_context_save(&state[bufferptr]);

  return bufferptr;
}

int sound_context_load(u8 *state)
{
  int bufferptr = 0;

  // Delete current FM synthesizer if present (that should be the case).
  if (gpgx::g_fm_synthesizer) {
    delete gpgx::g_fm_synthesizer;
    gpgx::g_fm_synthesizer = nullptr;
  }

  // Create, initialize and define the current FM synthesizer.
  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD) {
    u8 config_ym3438;
    load_param(&config_ym3438, sizeof(config_ym3438));

    if (config_ym3438) {
      gpgx::g_fm_synthesizer = sound_create_ym3438();
    } else {
      gpgx::g_fm_synthesizer = sound_create_ym2612();
    }
  } else {
    u8 config_ym2413;
    load_param(&config_ym2413, sizeof(config_ym2413));

    if (config_ym2413) {
      gpgx::g_fm_synthesizer = sound_create_ym2413();
    } else {
      gpgx::g_fm_synthesizer = new gpgx::audio::effect::NullFmSynthesizer();
    }
  }

  // Load the context of the FM synthesizer.
  // If it is "Null", nothing will be loaded.
  bufferptr += gpgx::g_fm_synthesizer->LoadContext(&state[bufferptr]);

  bufferptr += gpgx::g_psg->psg_context_load(&state[bufferptr]);

  // gpgx::g_psg->psg_config() is called in state_load().

  return bufferptr;
}
