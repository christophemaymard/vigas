/**
 * Sample buffer that resamples from input clock rate to output sample rate.
 * 
 * Modified for VIGAS project by Christophe Maymard:
 * - removed mono buffer support and #BLIP_MONO, 
 * - removed fixed_t and use u64 instead, 
 * - removed buf_t and use s32 instead, 
 * - removed blip_buffer_t, 
 * - removed check_assumptions(), 
 * - remove enum blip_max_frame (only used in check_assumptions())
 * 
 * Modified for Genesis Plus GX by EkeEke
 *  - disabled assertions checks (define #BLIP_ASSERT to re-enable)
 *  - fixed multiple time-frames support & removed m->avail
 *  - added blip_mix_samples function (see blip_buf.h)
 *  - added stereo buffer support (define #BLIP_MONO to disable)
 *  - added inverted stereo output (define #BLIP_INVERT to enable)
 * 
 * Library Copyright (C) 2003-2009 Shay Green. This library is free software;
 * you can redistribute it and/or modify it under the terms of the GNU Lesser
 * General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version. This
 * library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details. You should have received a copy of the GNU Lesser General Public
 * License along with this module; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __GPGX_AUDIO_BLIP_BUFFER_H__
#define __GPGX_AUDIO_BLIP_BUFFER_H__

#include "xee/fnd/data_type.h"

namespace gpgx::audio {

//==============================================================================

//------------------------------------------------------------------------------

class BlipBuffer
{
private:

  // Maximum clock_rate/sample_rate ratio. 
  // For a given sample_rate, clock_rate must not be greater than 
  // sample_rate * kMaxRatio.
  static constexpr s32 kMaxRatio = 1 << 20;

  static constexpr s32 kPreShift = 32;

  static constexpr s32 kTimeBits = kPreShift + 20;
  static constexpr u64 kTimeUnit = (u64)1 << kTimeBits;

  static constexpr s32 kBassShift = 9; // affects high-pass filter breakpoint frequency.
  static constexpr s32 kEndFrameExtra = 2; // allows deltas slightly after frame length.

  static constexpr s32 kHalfWidth  = 8;
  static constexpr s32 kBufExtra  = kHalfWidth * 2 + kEndFrameExtra;

  static constexpr s32 kPhaseBits  = 5;
  static constexpr s32 kPhaseCount = 1 << kPhaseBits;

  static constexpr s32 kDeltaBits  = 15;
  static constexpr s32 kDeltaUnit  = 1 << kDeltaBits;

  static constexpr s32 kFracBits = kTimeBits - kPreShift;
  static constexpr s32 kPhaseShift = kFracBits - kPhaseBits;

  static constexpr s32 kMaxSample = 32767;
  static constexpr s32 kMinSample = -32768;

  static const short m_bl_step[kPhaseCount + 1][kHalfWidth];

public:

  // Creates new buffer that can hold at most sample_count samples.
  // Sets rates so that there are kMaxRatio clocks per sample.
  // Returns pointer to new buffer, or NULL if insufficient memory.
  static BlipBuffer* blip_new(int sample_count);

  BlipBuffer();

  // Frees buffer.
  void blip_delete();

  // Sets approximate input clock rate and output sample rate.
  // For everyclock_rate input clocks, approximately sample_rate samples are generated.
  void blip_set_rates(f64 clock_rate, f64 sample_rate);

  // Clears entire buffer.
  // Afterwards, blip_samples_avail() == 0.
  void blip_clear();

  // Length of time frame, in clocks, needed to make sample_count additional.
  // samples available.
  // [Hardware MCD]
  int blip_clocks_needed(int sample_count);

  // Adds positive/negative deltas into stereo buffers at specified clock time.
  void blip_add_delta(unsigned int time, int delta_l, int delta_r);

  // Same as blip_add_delta(), but uses faster, lower-quality synthesis.
  void blip_add_delta_fast(unsigned int clock_time, int delta_l, int delta_r);

  // Reads and removes at most 'count' samples and writes them to to every other 
  // element of 'out', allowing easy interleaving of two buffers into a stereo sample 
  // stream. Outputs 16-bit signed samples. Returns number of samples actually read.
  // [Hardware SG SGII SGII_RAM_EXT MARKIII SMS SMS2 GG GGMS MD PBC PICO]
  int blip_read_samples(short out[], int count);

  // Same as above function except sample is mixed from three blip buffers source.
  // [Hardware MCD]
  int blip_mix_samples(BlipBuffer* m2, BlipBuffer* m3, short out[], int count);

  // Makes input clocks before clock_duration available for reading as output 
  // samples. Also begins new time frame at clock_duration, so that clock time 0 in 
  // the new time frame specifies the same clock as clock_duration in the old time 
  // frame specified. Deltas can have been added slightly past clock_duration (up to 
  // however many clocks there are in two output samples).
  void blip_end_frame(unsigned int clock_duration);

  // Number of buffered samples available for reading.
  int blip_samples_avail();

private:
  void remove_samples(int count);

private:
  u64 m_factor;
  u64 m_offset;
  int m_size;
  int m_integrator[2];
  s32* m_buffer[2];
};

} // namespace gpgx::audio

#endif // #ifndef __GPGX_AUDIO_BLIP_BUFFER_H__

