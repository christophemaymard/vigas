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

#ifndef __GPGX_AUDIO_EFFECT_FM_SYNTHESIZER_BASE_H__
#define __GPGX_AUDIO_EFFECT_FM_SYNTHESIZER_BASE_H__

#include "gpgx/audio/effect/fm_synthesizer.h"

namespace gpgx::audio::effect {

//==============================================================================

//------------------------------------------------------------------------------

class FmSynthesizerBase : public IFmSynthesizer
{
public:
  FmSynthesizerBase();

  void SetClockRatio(int clock_ratio);

  // Implementation of IFmSynthesizer.

  void Reset(int* buffer);

  // Run FM chip until end of frame.
  void EndFrame(unsigned int cycles);

  int SaveContext(unsigned char* state);
  int LoadContext(unsigned char* state);

protected:
  // Run FM chip until required M-cycles.
  void Update(int cycles);

  virtual void UpdateSampleBuffer(int* buffer, int length) = 0;

  virtual int SaveChipContext(unsigned char* state) = 0;
  virtual int LoadChipContext(unsigned char* state) = 0;

protected:
  int m_fm_cycles_busy;

private:

  int m_fm_cycles_ratio; // The clock ratio (must be different of 0).
  int m_fm_cycles_start;
  int m_fm_cycles_count;

  int m_fm_last[2]; // Last FM output ([0]: left channel, [1]: right channel).

  int* m_fm_buffer;
  int* m_fm_ptr; // Buffer current pointer.
};

} // namespace gpgx::audio::effect

#endif // #ifndef __GPGX_AUDIO_EFFECT_FM_SYNTHESIZER_BASE_H__

