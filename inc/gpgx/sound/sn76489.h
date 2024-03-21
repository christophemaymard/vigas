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

#ifndef __GPGX_SOUND_SN76489_H__
#define __GPGX_SOUND_SN76489_H__

#include "xee/fnd/data_type.h"

namespace gpgx::sound {

//==============================================================================

//------------------------------------------------------------------------------

enum PSG_TYPE
{
  PSG_DISCRETE,
  PSG_INTEGRATED
};

//------------------------------------------------------------------------------

class Sn76489
{
private:

  // internal clock = input clock : 16 = (master clock : 15) : 16
  //#define PSG_MCYCLES_RATIO (15*16)
  static constexpr s32 kMCyclesRatio = 15 * 16;

  // Maximal channel output (roughly adjusted to match VA4 MD1 PSG/FM balance 
  // with 1.5x amplification of PSG output).
  static constexpr s32 kMaxVolume = 2800;

  static const u8 noiseShiftWidth[2];
  static const u8 noiseBitMask[2];
  static const u8 noiseFeedback[10];
  static const u16 chanVolume[16];
public:
  Sn76489();

  void psg_init(PSG_TYPE type);
  void psg_reset();

  int psg_context_save(u8* state);
  int psg_context_load(u8* state);

  void psg_write(unsigned int clocks, unsigned int data);
  void psg_config(unsigned int clocks, unsigned int preamp, unsigned int panning);
  void psg_end_frame(unsigned int clocks);

private:
  void psg_update(unsigned int clocks);

private:
  int m_clocks;
  int m_latch;
  int m_zeroFreqInc;
  int m_noiseShiftValue;
  int m_noiseShiftWidth;
  int m_noiseBitMask;
  int m_regs[8];
  int m_freqInc[4];
  int m_freqCounter[4];
  int m_polarity[4];
  int m_chanDelta[4][2];
  int m_chanOut[4][2];
  int m_chanAmp[4][2];
};

} // namespace gpgx::sound

#endif // #ifndef __GPGX_SOUND_SN76489_H__

