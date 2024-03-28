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

#include "gpgx/audio/effect/fm_synthesizer_base.h"

#include "xee/mem/memory.h"

#include "core/core_config.h"
#include "core/snd.h"
#include "core/state.h"

namespace gpgx::audio::effect {

//==============================================================================
// FmSynthesizerBase

//------------------------------------------------------------------------------

FmSynthesizerBase::FmSynthesizerBase()
{
  m_fm_cycles_ratio = 1; // Must be different of 0.
  m_fm_cycles_start = 0;
  m_fm_cycles_count = 0;
  m_fm_cycles_busy = 0;

  m_fm_last[0] = 0;
  m_fm_last[1] = 0;

  m_fm_buffer = nullptr;
  m_fm_ptr = nullptr;
}

//------------------------------------------------------------------------------

void FmSynthesizerBase::SetClockRatio(int clock_ratio)
{
  m_fm_cycles_ratio = clock_ratio;
}

//------------------------------------------------------------------------------

void FmSynthesizerBase::Reset(int* buffer)
{
  // Synchronize FM chip with CPU and reset FM chip.
  SyncAndReset(0);

  // Reset FM buffer ouput.
  m_fm_last[0] = 0;
  m_fm_last[1] = 0;

  // Reset FM buffer pointer.
  m_fm_buffer = buffer;
  m_fm_ptr = buffer;

  // Reset FM cycle counters.
  m_fm_cycles_start = 0;
  m_fm_cycles_count = 0;
}

//------------------------------------------------------------------------------

// Run FM chip until end of frame.
void FmSynthesizerBase::EndFrame(unsigned int cycles)
{
  // Run FM chip until end of frame.
  Update(cycles);

  // FM output pre-amplification.
  int preamp = core_config.fm_preamp;

  // FM frame initial timestamp.
  int time = m_fm_cycles_start;

  // Restore last FM outputs from previous frame.
  int prev_l = m_fm_last[0];
  int prev_r = m_fm_last[1];

  // FM buffer start pointer.
  int* ptr = m_fm_buffer;

  int l = 0;
  int r = 0;

  // flush FM samples.
  if (core_config.hq_fm) {
    // high-quality Band-Limited synthesis.
    do {
      // left & right channels.
      l = ((*ptr++ * preamp) / 100);
      r = ((*ptr++ * preamp) / 100);
      snd.blips[0]->blip_add_delta(time, l - prev_l, r - prev_r);
      prev_l = l;
      prev_r = r;

      // increment time counter.
      time += m_fm_cycles_ratio;
    } while (time < cycles);
  } else {
    // faster Linear Interpolation.
    do {
      // left & right channels.
      l = ((*ptr++ * preamp) / 100);
      r = ((*ptr++ * preamp) / 100);
      snd.blips[0]->blip_add_delta_fast(time, l - prev_l, r - prev_r);
      prev_l = l;
      prev_r = r;

      // increment time counter.
      time += m_fm_cycles_ratio;
    } while (time < cycles);
  }

  // reset FM buffer pointer.
  m_fm_ptr = m_fm_buffer;

  // save last FM output for next frame.
  m_fm_last[0] = prev_l;
  m_fm_last[1] = prev_r;

  // adjust FM cycle counters for next frame.
  m_fm_cycles_count = m_fm_cycles_start = time - cycles;

  if (m_fm_cycles_busy > cycles) {
    m_fm_cycles_busy -= cycles;
  } else {
    m_fm_cycles_busy = 0;
  }
}

//------------------------------------------------------------------------------

int FmSynthesizerBase::SaveContext(unsigned char* state)
{
  int bufferptr = SaveChipContext(state);

  save_param(&m_fm_cycles_start, sizeof(m_fm_cycles_start));

  return bufferptr;
}

//------------------------------------------------------------------------------

int FmSynthesizerBase::LoadContext(unsigned char* state)
{
  int bufferptr = LoadChipContext(state);

  load_param(&m_fm_cycles_start, sizeof(m_fm_cycles_start));

  m_fm_cycles_count = m_fm_cycles_start;

  return bufferptr;
}

//------------------------------------------------------------------------------

// Run FM chip until required M-cycles.
void FmSynthesizerBase::Update(int cycles)
{
  if (cycles > m_fm_cycles_count) {
    // number of samples to run.
    int samples = (cycles - m_fm_cycles_count + m_fm_cycles_ratio - 1) / m_fm_cycles_ratio;

    // run FM chip to sample buffer.
    UpdateSampleBuffer(m_fm_ptr, samples);

    // update FM buffer pointer.
    m_fm_ptr += (samples * 2);

    // update FM cycle counter.
    m_fm_cycles_count += (samples * m_fm_cycles_ratio);
  }
}

} // namespace gpgx::audio::effect

