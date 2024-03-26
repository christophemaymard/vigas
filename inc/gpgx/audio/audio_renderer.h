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

#ifndef __GPGX_AUDIO_AUDIO_RENDERER_H__
#define __GPGX_AUDIO_AUDIO_RENDERER_H__

#include "xee/fnd/data_type.h"

#include "gpgx/audio/effect/equalizer_3band.h"
#include "gpgx/audio/effect/fm_synthesizer.h"
#include "gpgx/ic/ym2413/ym2413.h"
#include "gpgx/ic/ym2612/ym2612.h"
#include "gpgx/ic/ym3438/ym3438.h"

namespace gpgx::audio {

//==============================================================================

//------------------------------------------------------------------------------

/**
 * Audio renderer.
 */
class AudioRenderer
{
private:
  static constexpr s32 kFmTypeNone = 0;   // None
  static constexpr s32 kFmTypeNull = 1;   // Null FM synthesizer.
  static constexpr s32 kFmTypeYm2413 = 2; // Ym2413 FM synthesizer.
  static constexpr s32 kFmTypeYm2612 = 3; // Ym2612 FM synthesizer.
  static constexpr s32 kFmTypeYm3438 = 4; // Ym3438 FM synthesizer.

public:
  AudioRenderer();

  /**
   * Initialize the audio renderer.
   * 
   * It creates and initializes FM synthesizer, PSG and equalizers.
   */
  void Init();

  /**
   * Destroy all resources created by the audio renderer (FM synthesizer, PSG and equalizers).
   */
  void Destroy();

  /**
   * Reset FM synthesizer and PSG chips.  
   */
  void ResetChips();

  /**
   * Reset low-pass filter.
   */
  void ResetLowPassFilter();

  /**
   * Apply equalization settings
   */
  void ApplyEqualizationSettings();

  /**
   * Generate samples to the specified buffer.
   *  
   * @param  output_buffer  The pointer of the buffer to generate samples to.
   * 
   * @return  The number of generated samples.
   */
  s32 Update(s16* output_buffer);

  s32 LoadContext(u8* state);
  s32 SaveContext(u8* state);

private:
  /**
   * Build a new instance of a FM synthesizer.
   * 
   * @param fm_type The type of a FM synthesizer to build (kFmTypeNull, kFmTypeYm2413, kFmTypeYm2612 or kFmTypeYm3438). 
   */
  void RebuildFmSynthesizer(s32 fm_type);

  /**
   * Create and initialize a new instance of a YM2413 FM synthesizer.
   *  
   * @return  A new instance of a YM2413 FM synthesizer.
   */
  gpgx::ic::ym2413::Ym2413* CreateYm2413FmSynthesizer();

  /**
   * Create and initialize a new instance of a YM2612 (MAME OPN2) FM synthesizer.
   *  
   * @return  A new instance of a YM2612 FM synthesizer.
   */
  gpgx::ic::ym2612::Ym2612* CreateYm2612FmSynthesizer();

  /**
   * Create and initialize a new instance of a YM3438 (Nuked OPN2) FM synthesizer.
   *
   * @return  A new instance of a YM3438 FM synthesizer.
   */
  gpgx::ic::ym3438::Ym3438* CreateYm3438FmSynthesizer();

private:
  // The type of the current FM synthesizer.
  // (kFmTypeNone, kFmTypeNull, kFmTypeYm2413, kFmTypeYm2612 or kFmTypeYm3438)
  s32 m_fm_type;

  // The output buffer used by the FM synthesizer.
  // (large enough to hold a whole frame at original chips rate)
  int m_fm_buffer[1080 * 2 * 24];

  s16 llp; // Last sample for the left channel (when using low-pass filtering).
  s16 rrp; // Last sample for the right channel (when using low-pass filtering).

  // Equalizer for the left channel (index 0) and for the right channel (index 1).
  gpgx::audio::effect::Equalizer3band* m_eq[2]; 
};

} // namespace gpgx::audio

#endif // #ifndef __GPGX_AUDIO_AUDIO_RENDERER_H__