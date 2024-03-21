/**
 *  Genesis Plus
 *  PSG sound chip (SN76489A compatible)
 *
 *  Support for discrete chip & integrated (ASIC) clones
 *
 *  Noise implementation based on http://www.smspower.org/Development/SN76489#NoiseChannel
 *
 *  Copyright (C) 2016-2017 Eke-Eke (Genesis Plus GX)
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
 */

#include "gpgx/sound/sn76489.h"

#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "osd.h"
#include "core/snd.h"
#include "core/state.h"

namespace gpgx::sound {

//==============================================================================
// Sn76489

//------------------------------------------------------------------------------

const u8 Sn76489::noiseShiftWidth[2] = { 14,15 };
const u8 Sn76489::noiseBitMask[2] = { 0x6,0x9 };
const u8 Sn76489::noiseFeedback[10] = { 0,1,1,0,1,0,0,1,1,0 };

const u16 Sn76489::chanVolume[16] = {
  kMaxVolume,               //  MAX
  kMaxVolume * 0.794328234, // -2dB
  kMaxVolume * 0.630957344, // -4dB
  kMaxVolume * 0.501187233, // -6dB
  kMaxVolume * 0.398107170, // -8dB
  kMaxVolume * 0.316227766, // -10dB
  kMaxVolume * 0.251188643, // -12dB
  kMaxVolume * 0.199526231, // -14dB
  kMaxVolume * 0.158489319, // -16dB
  kMaxVolume * 0.125892541, // -18dB
  kMaxVolume * 0.1,         // -20dB
  kMaxVolume * 0.079432823, // -22dB
  kMaxVolume * 0.063095734, // -24dB
  kMaxVolume * 0.050118723, // -26dB
  kMaxVolume * 0.039810717, // -28dB
  0                             //  OFF
};

//------------------------------------------------------------------------------

Sn76489::Sn76489()
{
  m_clocks = 0;
  m_latch = 0;
  m_zeroFreqInc = 0;
  m_noiseShiftValue = 0;
  m_noiseShiftWidth = 0;
  m_noiseBitMask = 0;
  xee::mem::Memset(&m_regs[0], 0, sizeof(m_regs));
  xee::mem::Memset(&m_freqInc[0], 0, sizeof(m_freqInc));
  xee::mem::Memset(&m_freqCounter[0], 0, sizeof(m_freqCounter));
  xee::mem::Memset(&m_polarity[0], 0, sizeof(m_polarity));
  xee::mem::Memset(&m_chanDelta[0], 0, sizeof(m_chanDelta));
  xee::mem::Memset(&m_chanOut[0], 0, sizeof(m_chanOut));
  xee::mem::Memset(&m_chanAmp[0], 0, sizeof(m_chanAmp));
}

//------------------------------------------------------------------------------

void Sn76489::psg_init(PSG_TYPE type)
{
  // Initialize stereo amplification (default).
  for (int i = 0; i < 4; i++) {
    m_chanAmp[i][0] = 100;
    m_chanAmp[i][1] = 100;
  }

  // Initialize Tone zero frequency increment value.
  m_zeroFreqInc = ((type == PSG_DISCRETE) ? 0x400 : 0x1) * kMCyclesRatio;

  // Initialize Noise LSFR type.
  m_noiseShiftWidth = noiseShiftWidth[type];
  m_noiseBitMask = noiseBitMask[type];
}

//------------------------------------------------------------------------------

void Sn76489::psg_reset()
{
  // power-on state (verified on 315-5313A & 315-5660 integrated version only).
  for (int i = 0; i < 4; i++) {
    m_regs[i * 2] = 0;
    m_regs[i * 2 + 1] = 0;
    m_freqInc[i] = (i < 3) ? (m_zeroFreqInc) : (16 * kMCyclesRatio);
    m_freqCounter[i] = 0;
    m_polarity[i] = -1;
    m_chanDelta[i][0] = 0;
    m_chanDelta[i][1] = 0;
    m_chanOut[i][0] = 0;
    m_chanOut[i][1] = 0;
  }

  // tone #2 attenuation register is latched on power-on (verified on 315-5313A integrated version only).
  m_latch = 3;

  // reset noise shift register.
  m_noiseShiftValue = 1 << m_noiseShiftWidth;

  // reset internal M-cycles clock counter.
  m_clocks = 0;
}

//------------------------------------------------------------------------------

int Sn76489::psg_context_save(u8* state)
{
  int bufferptr = 0;

  save_param(&m_clocks, sizeof(m_clocks));
  save_param(&m_latch, sizeof(m_latch));
  save_param(&m_noiseShiftValue, sizeof(m_noiseShiftValue));
  save_param(m_regs, sizeof(m_regs));
  save_param(m_freqInc, sizeof(m_freqInc));
  save_param(m_freqCounter, sizeof(m_freqCounter));
  save_param(m_polarity, sizeof(m_polarity));
  save_param(m_chanOut, sizeof(m_chanOut));

  return bufferptr;
}

//------------------------------------------------------------------------------

int Sn76489::psg_context_load(u8* state)
{
  int delta[2] = { 0, 0 };

  // initialize delta with current noise channel output.
  if (m_noiseShiftValue & 1) {
    delta[0] = -m_chanOut[3][0];
    delta[1] = -m_chanOut[3][1];
  } else {
    delta[0] = 0;
    delta[1] = 0;
  }

  int i;

  // add current tone channels output.
  for (i = 0; i < 3; i++) {
    if (m_polarity[i] > 0) {
      delta[0] -= m_chanOut[i][0];
      delta[1] -= m_chanOut[i][1];
    }
  }

  int bufferptr = 0;

  load_param(&m_clocks, sizeof(m_clocks));
  load_param(&m_latch, sizeof(m_latch));
  load_param(&m_noiseShiftValue, sizeof(m_noiseShiftValue));
  load_param(m_regs, sizeof(m_regs));
  load_param(m_freqInc, sizeof(m_freqInc));
  load_param(m_freqCounter, sizeof(m_freqCounter));
  load_param(m_polarity, sizeof(m_polarity));
  load_param(m_chanOut, sizeof(m_chanOut));

  // add noise channel output variation.
  if (m_noiseShiftValue & 1) {
    delta[0] += m_chanOut[3][0];
    delta[1] += m_chanOut[3][1];
  }

  // add tone channels output variation.
  for (i = 0; i < 3; i++) {
    if (m_polarity[i] > 0) {
      delta[0] += m_chanOut[i][0];
      delta[1] += m_chanOut[i][1];
    }
  }

  // update mixed channels output.
  if (config.hq_psg) {
    snd.blips[0]->blip_add_delta(m_clocks, delta[0], delta[1]);
  } else {
    snd.blips[0]->blip_add_delta_fast(m_clocks, delta[0], delta[1]);
  }

  return bufferptr;
}

//------------------------------------------------------------------------------

void Sn76489::psg_write(unsigned int clocks, unsigned int data)
{
  // PSG chip synchronization.
  if (clocks > m_clocks) {
    // run PSG chip until current timestamp.
    psg_update(clocks);

    // update internal M-cycles clock counter.
    m_clocks += ((clocks - m_clocks + kMCyclesRatio - 1) / kMCyclesRatio) * kMCyclesRatio;
  }

  int index = 0;

  if (data & 0x80) {
    // latch register index (1xxx----).
    m_latch = index = (data >> 4) & 0x07;
  } else {
    // restore latched register index.
    index = m_latch;
  }

  switch (index) {
    case 0:
    case 2:
    case 4: // Tone channels frequency.
    {
      // recalculate frequency register value.
      if (data & 0x80) {
        // update 10-bit register LSB (1---xxxx).
        data = (m_regs[index] & 0x3f0) | (data & 0x0f);
      } else {
        // update 10-bit register MSB (0-xxxxxx).
        data = (m_regs[index] & 0x00f) | ((data & 0x3f) << 4);
      }

      // update channel M-cycle counter increment.
      if (data) {
        m_freqInc[index >> 1] = data * kMCyclesRatio;
      } else {
        // zero value behaves the same as a value of 1 on integrated version (0x400 on discrete version).
        m_freqInc[index >> 1] = m_zeroFreqInc;
      }

      // update noise channel counter increment if required.
      if ((index == 4) && ((m_regs[6] & 0x03) == 0x03)) {
        m_freqInc[3] = m_freqInc[2];
      }

      break;
    }

    case 6: // Noise control.
    {
      // noise signal generator frequency (-----?xx).
      int noiseFreq = (data & 0x03);

      if (noiseFreq == 0x03) {
        // noise generator is controlled by tone channel #3 generator.
        m_freqInc[3] = m_freqInc[2];
        m_freqCounter[3] = m_freqCounter[2];
      } else {
        // noise generator is running at separate frequency.
        m_freqInc[3] = (0x10 << noiseFreq) * kMCyclesRatio;
      }

      // check current noise shift register output.
      if (m_noiseShiftValue & 1) {
        // high to low transition will be applied at next internal cycle update.
        m_chanDelta[3][0] -= m_chanOut[3][0];
        m_chanDelta[3][1] -= m_chanOut[3][1];
      }

      // reset noise shift register value (noise channel output is forced low).
      m_noiseShiftValue = 1 << m_noiseShiftWidth;;

      break;
    }

    case 7: // Noise channel attenuation.
    {
      int chanOut[2];

      // convert 4-bit attenuation value (----xxxx) to 16-bit volume value.
      data = chanVolume[data & 0x0f];

      // channel pre-amplification.
      chanOut[0] = (data * m_chanAmp[3][0]) / 100;
      chanOut[1] = (data * m_chanAmp[3][1]) / 100;

      // check noise shift register output.
      if (m_noiseShiftValue & 1) {
        // channel output is high, volume variation will be applied at next internal cycle update.
        m_chanDelta[3][0] += (chanOut[0] - m_chanOut[3][0]);
        m_chanDelta[3][1] += (chanOut[1] - m_chanOut[3][1]);
      }

      // update channel volume.
      m_chanOut[3][0] = chanOut[0];
      m_chanOut[3][1] = chanOut[1];

      break;
    }

    default: // Tone channels attenuation.
    {
      int chanOut[2];

      // channel number (0-2).
      int i = index >> 1;

      // convert 4-bit attenuation value (----xxxx) to 16-bit volume value.
      data = chanVolume[data & 0x0f];

      // channel pre-amplification.
      chanOut[0] = (data * m_chanAmp[i][0]) / 100;
      chanOut[1] = (data * m_chanAmp[i][1]) / 100;

      // check tone generator polarity.
      if (m_polarity[i] > 0) {
        // channel output is high, volume variation will be applied at next internal cycle update.
        m_chanDelta[i][0] += (chanOut[0] - m_chanOut[i][0]);
        m_chanDelta[i][1] += (chanOut[1] - m_chanOut[i][1]);
      }

      // update channel volume.
      m_chanOut[i][0] = chanOut[0];
      m_chanOut[i][1] = chanOut[1];

      break;
    }
  }

  // save register value.
  m_regs[index] = data;
}

//------------------------------------------------------------------------------

void Sn76489::psg_config(unsigned int clocks, unsigned int preamp, unsigned int panning)
{
  // PSG chip synchronization.
  if (clocks > m_clocks) {
    // run PSG chip until current timestamp.
    psg_update(clocks);

    // update internal M-cycles clock counter.
    m_clocks += ((clocks - m_clocks + kMCyclesRatio - 1) / kMCyclesRatio) * kMCyclesRatio;
  }

  for (int i = 0; i < 4; i++) {
    // channel internal volume.
    int volume = m_regs[i * 2 + 1];

    // update channel stereo amplification.
    m_chanAmp[i][0] = preamp * ((panning >> (i + 4)) & 1);
    m_chanAmp[i][1] = preamp * ((panning >> (i + 0)) & 1);

    // tone channels.
    if (i < 3) {
      // check tone generator polarity.
      if (m_polarity[i] > 0) {
        // channel output is high, volume variation will be applied at next internal cycle update.
        m_chanDelta[i][0] += (((volume * m_chanAmp[i][0]) / 100) - m_chanOut[i][0]);
        m_chanDelta[i][1] += (((volume * m_chanAmp[i][1]) / 100) - m_chanOut[i][1]);
      }
    }

    // noise channel.
    else {
      // check noise shift register output.
      if (m_noiseShiftValue & 1) {
        // channel output is high, volume variation will be applied at next internal cycle update.
        m_chanDelta[3][0] += (((volume * m_chanAmp[3][0]) / 100) - m_chanOut[3][0]);
        m_chanDelta[3][1] += (((volume * m_chanAmp[3][1]) / 100) - m_chanOut[3][1]);
      }
    }

    // update channel volume.
    m_chanOut[i][0] = (volume * m_chanAmp[i][0]) / 100;
    m_chanOut[i][1] = (volume * m_chanAmp[i][1]) / 100;
  }
}

//------------------------------------------------------------------------------

void Sn76489::psg_end_frame(unsigned int clocks)
{
  if (clocks > m_clocks) {
    // run PSG chip until current timestamp.
    psg_update(clocks);

    // update internal M-cycles clock counter.
    m_clocks += ((clocks - m_clocks + kMCyclesRatio - 1) / kMCyclesRatio) * kMCyclesRatio;
  }

  // adjust internal M-cycles clock counter for next frame.
  m_clocks -= clocks;

  // adjust channels time counters for next frame.
  for (int i = 0; i < 4; ++i) {
    m_freqCounter[i] -= clocks;
  }
}

//------------------------------------------------------------------------------

void Sn76489::psg_update(unsigned int clocks)
{
  int timestamp = 0;
  int polarity = 0;

  for (int i = 0; i < 4; i++) {
    // apply any pending channel volume variations.
    if (m_chanDelta[i][0] | m_chanDelta[i][1]) {
      // update channel output.
      if (config.hq_psg) {
        snd.blips[0]->blip_add_delta(m_clocks, m_chanDelta[i][0], m_chanDelta[i][1]);
      } else {
        snd.blips[0]->blip_add_delta_fast(m_clocks, m_chanDelta[i][0], m_chanDelta[i][1]);
      }

      // clear pending channel volume variations.
      m_chanDelta[i][0] = 0;
      m_chanDelta[i][1] = 0;
    }

    // timestamp of next transition.
    timestamp = m_freqCounter[i];

    // current channel generator polarity.
    polarity = m_polarity[i];

    // Tone channels.
    if (i < 3) {
      // process all transitions occurring until current clock timestamp.
      while (timestamp < clocks) {
        // invert tone generator polarity.
        polarity = -polarity;

        // update channel output.
        if (config.hq_psg) {
          snd.blips[0]->blip_add_delta(timestamp, polarity * m_chanOut[i][0], polarity * m_chanOut[i][1]);
        } else {
          snd.blips[0]->blip_add_delta_fast(timestamp, polarity * m_chanOut[i][0], polarity * m_chanOut[i][1]);
        }

        // timestamp of next transition.
        timestamp += m_freqInc[i];
      }
    }

    // Noise channel.
    else {
      // current noise shift register value.
      int shiftValue = m_noiseShiftValue;

      // process all transitions occurring until current clock timestamp.
      while (timestamp < clocks) {
        // invert noise generator polarity.
        polarity = -polarity;

        // noise register is shifted on positive edge only.
        if (polarity > 0) {
          // current shift register output.
          int shiftOutput = shiftValue & 0x01;

          // White noise (-----1xx).
          if (m_regs[6] & 0x04) {
            // shift and apply XOR feedback network.
            shiftValue = (shiftValue >> 1) | (noiseFeedback[shiftValue & m_noiseBitMask] << m_noiseShiftWidth);
          }

          // Periodic noise (-----0xx).
          else {
            // shift and feedback current output.
            shiftValue = (shiftValue >> 1) | (shiftOutput << m_noiseShiftWidth);
          }

          // shift register output variation.
          shiftOutput = (shiftValue & 0x1) - shiftOutput;

          // update noise channel output.
          if (config.hq_psg) {
            snd.blips[0]->blip_add_delta(timestamp, shiftOutput * m_chanOut[3][0], shiftOutput * m_chanOut[3][1]);
          } else {
            snd.blips[0]->blip_add_delta_fast(timestamp, shiftOutput * m_chanOut[3][0], shiftOutput * m_chanOut[3][1]);
          }
        }

        // timestamp of next transition.
        timestamp += m_freqInc[3];
      }

      // save shift register value.
      m_noiseShiftValue = shiftValue;
    }

    // save timestamp of next transition.
    m_freqCounter[i] = timestamp;

    // save channel generator polarity.
    m_polarity[i] = polarity;
  }
}

} // namespace gpgx::sound

