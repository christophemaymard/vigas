/**
 * Sample buffer that resamples from input clock rate to output sample rate.
 *
 * Modified for VIGAS project by Christophe Maymard:
 * - removed mono buffer support and #BLIP_MONO,
 * - removed fixed_t and use u64 instead,
 * - removed buf_t and use s32 instead,
 * - removed blip_buffer_t,
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

#include "gpgx/sound/blip_buffer.h"

#ifdef BLIP_ASSERT
#include <cassert>
#endif

#include <cstdlib>
#include <new> // For std::nothrow.
#include <string>

#include "xee/fnd/data_type.h"

namespace gpgx::sound {

//==============================================================================
// BlipBuffer

//------------------------------------------------------------------------------

// Arithmetic (sign-preserving) right shift.
#define ARITH_SHIFT(n, shift) ((n) >> (shift))

#define CLAMP( n ) \
	{\
		if ( n > kMaxSample ) n = kMaxSample;\
    else if ( n < kMinSample) n = kMinSample;\
	}

//------------------------------------------------------------------------------

// Sinc_Generator( 0.9, 0.55, 4.5 ).
const short BlipBuffer::m_bl_step[kPhaseCount + 1][kHalfWidth] =
{
  {   43, -115,  350, -488, 1136, -914, 5861,21022},
  {   44, -118,  348, -473, 1076, -799, 5274,21001},
  {   45, -121,  344, -454, 1011, -677, 4706,20936},
  {   46, -122,  336, -431,  942, -549, 4156,20829},
  {   47, -123,  327, -404,  868, -418, 3629,20679},
  {   47, -122,  316, -375,  792, -285, 3124,20488},
  {   47, -120,  303, -344,  714, -151, 2644,20256},
  {   46, -117,  289, -310,  634,  -17, 2188,19985},
  {   46, -114,  273, -275,  553,  117, 1758,19675},
  {   44, -108,  255, -237,  471,  247, 1356,19327},
  {   43, -103,  237, -199,  390,  373,  981,18944},
  {   42,  -98,  218, -160,  310,  495,  633,18527},
  {   40,  -91,  198, -121,  231,  611,  314,18078},
  {   38,  -84,  178,  -81,  153,  722,   22,17599},
  {   36,  -76,  157,  -43,   80,  824, -241,17092},
  {   34,  -68,  135,   -3,    8,  919, -476,16558},
  {   32,  -61,  115,   34,  -60, 1006, -683,16001},
  {   29,  -52,   94,   70, -123, 1083, -862,15422},
  {   27,  -44,   73,  106, -184, 1152,-1015,14824},
  {   25,  -36,   53,  139, -239, 1211,-1142,14210},
  {   22,  -27,   34,  170, -290, 1261,-1244,13582},
  {   20,  -20,   16,  199, -335, 1301,-1322,12942},
  {   18,  -12,   -3,  226, -375, 1331,-1376,12293},
  {   15,   -4,  -19,  250, -410, 1351,-1408,11638},
  {   13,    3,  -35,  272, -439, 1361,-1419,10979},
  {   11,    9,  -49,  292, -464, 1362,-1410,10319},
  {    9,   16,  -63,  309, -483, 1354,-1383, 9660},
  {    7,   22,  -75,  322, -496, 1337,-1339, 9005},
  {    6,   26,  -85,  333, -504, 1312,-1280, 8355},
  {    4,   31,  -94,  341, -507, 1278,-1205, 7713},
  {    3,   35, -102,  347, -506, 1238,-1119, 7082},
  {    1,   40, -110,  350, -499, 1190,-1021, 6464},
  {    0,   43, -115,  350, -488, 1136, -914, 5861}
};

//------------------------------------------------------------------------------

BlipBuffer::BlipBuffer()
{
  m_factor = 0;
  m_offset = 0;
  m_size = 0;
  m_integrator[0] = 0;
  m_integrator[1] = 0;
  m_buffer[0] = nullptr;
  m_buffer[1] = nullptr;
}

//------------------------------------------------------------------------------

BlipBuffer* BlipBuffer::blip_new(int size)
{
#ifdef BLIP_ASSERT
  assert(size >= 0);
#endif
  BlipBuffer* m = new (std::nothrow) BlipBuffer();

  if (!m) {
    return nullptr;
  }

  m->m_buffer[0] = new (std::nothrow) s32[size + kBufExtra];
  m->m_buffer[1] = new (std::nothrow) s32[size + kBufExtra];

  if (!m->m_buffer[0] || !m->m_buffer[1]) {
    m->blip_delete();
    delete m;

    return nullptr;
  }

  m->m_factor = kTimeUnit / kMaxRatio;
  m->m_size = size;
  m->blip_clear();
  
  return m;
}

//------------------------------------------------------------------------------

void BlipBuffer::blip_delete()
{
  if (!m_buffer[0]) {
    delete[] m_buffer[0];
    m_buffer[0] = nullptr;
  }

  if (!m_buffer[1]) {
    delete[] m_buffer[1];
    m_buffer[1] = nullptr;
  }

  // Clear fields in case user tries to use after freeing.
  m_factor = 0;
  m_offset = 0;
  m_size = 0;
  m_integrator[0] = 0;
  m_integrator[1] = 0;
}

//------------------------------------------------------------------------------

void BlipBuffer::blip_set_rates(double clock_rate, double sample_rate)
{
  double factor = kTimeUnit * sample_rate / clock_rate;
  m_factor = (u64)factor;

#ifdef BLIP_ASSERT
  // Fails if clock_rate exceeds maximum, relative to sample_rate.
  assert(0 <= factor - m_factor && factor - m_factor < 1);
#endif

  // Avoid requiring math.h.
  // Equivalent to m->factor = (int) ceil( factor )
  if (m_factor < factor) {
    m_factor++;
  }

  // At this point, factor is most likely rounded up, but could still
  // have been rounded down in the floating-point calculation.
}

//------------------------------------------------------------------------------

void BlipBuffer::blip_clear()
{
  // We could set offset to 0, factor/2, or factor-1. 0 is suitable if
  // factor is rounded up. factor-1 is suitable if factor is rounded down.
  // Since we don't know rounding direction, factor/2 accommodates either,
  // with the slight loss of showing an error in half the time. Since for
  // a 64-bit factor this is years, the halving isn't a problem.
  m_offset = m_factor / 2;

  m_integrator[0] = 0;
  m_integrator[1] = 0;
  ::memset(m_buffer[0], 0, (m_size + kBufExtra) * sizeof(s32));
  ::memset(m_buffer[1], 0, (m_size + kBufExtra) * sizeof(s32));
}

//------------------------------------------------------------------------------

// [Hardware MCD]
int BlipBuffer::blip_clocks_needed(int sample_count)
{
#ifdef BLIP_ASSERT
  // Fails if buffer can't hold that many more samples.
  assert((sample_count >= 0) && (((m_offset >> kTimeBits) + sample_count) <= m_size));
#endif

  u64 needed = (u64)sample_count * kTimeUnit;

  if (needed < m_offset) {
    return 0;
  }

  return (int)((needed - m_offset + m_factor - 1) / m_factor);
}

//------------------------------------------------------------------------------

// Shifting by kPreShift allows calculation using unsigned int rather than
// possibly-wider u64. On 32-bit platforms, this is likely more efficient.
// And by having kPreShift 32, a 32-bit platform can easily do the shift by
// simply ignoring the low half.
void BlipBuffer::blip_add_delta(unsigned int time, int delta_l, int delta_r)
{
  if (delta_l | delta_r) {
    unsigned fixed = (unsigned)((time * m_factor + m_offset) >> kPreShift);
    int phase = fixed >> kPhaseShift & (kPhaseCount - 1);
    short const* in = m_bl_step[phase];
    short const* rev = m_bl_step[kPhaseCount - phase];
    int interp = fixed >> (kPhaseShift - kDeltaBits) & (kDeltaUnit - 1);
    int pos = fixed >> kFracBits;

#ifdef BLIP_INVERT
    s32* out_l = m_buffer[1] + pos;
    s32* out_r = m_buffer[0] + pos;
#else
    s32* out_l = m_buffer[0] + pos;
    s32* out_r = m_buffer[1] + pos;
#endif

    int delta;

#ifdef BLIP_ASSERT
    // Fails if buffer size was exceeded.
    assert(pos <= m_size + kEndFrameExtra);
#endif

    if (delta_l == delta_r) {
      s32 out;
      delta = (delta_l * interp) >> kDeltaBits;
      delta_l -= delta;
      out = in[0] * delta_l + in[kHalfWidth + 0] * delta;
      out_l[0] += out;
      out_r[0] += out;
      out = in[1] * delta_l + in[kHalfWidth + 1] * delta;
      out_l[1] += out;
      out_r[1] += out;
      out = in[2] * delta_l + in[kHalfWidth + 2] * delta;
      out_l[2] += out;
      out_r[2] += out;
      out = in[3] * delta_l + in[kHalfWidth + 3] * delta;
      out_l[3] += out;
      out_r[3] += out;
      out = in[4] * delta_l + in[kHalfWidth + 4] * delta;
      out_l[4] += out;
      out_r[4] += out;
      out = in[5] * delta_l + in[kHalfWidth + 5] * delta;
      out_l[5] += out;
      out_r[5] += out;
      out = in[6] * delta_l + in[kHalfWidth + 6] * delta;
      out_l[6] += out;
      out_r[6] += out;
      out = in[7] * delta_l + in[kHalfWidth + 7] * delta;
      out_l[7] += out;
      out_r[7] += out;
      out = rev[7] * delta_l + rev[7 - kHalfWidth] * delta;
      out_l[8] += out;
      out_r[8] += out;
      out = rev[6] * delta_l + rev[6 - kHalfWidth] * delta;
      out_l[9] += out;
      out_r[9] += out;
      out = rev[5] * delta_l + rev[5 - kHalfWidth] * delta;
      out_l[10] += out;
      out_r[10] += out;
      out = rev[4] * delta_l + rev[4 - kHalfWidth] * delta;
      out_l[11] += out;
      out_r[11] += out;
      out = rev[3] * delta_l + rev[3 - kHalfWidth] * delta;
      out_l[12] += out;
      out_r[12] += out;
      out = rev[2] * delta_l + rev[2 - kHalfWidth] * delta;
      out_l[13] += out;
      out_r[13] += out;
      out = rev[1] * delta_l + rev[1 - kHalfWidth] * delta;
      out_l[14] += out;
      out_r[14] += out;
      out = rev[0] * delta_l + rev[0 - kHalfWidth] * delta;
      out_l[15] += out;
      out_r[15] += out;
    } else {
      delta = (delta_l * interp) >> kDeltaBits;
      delta_l -= delta;
      out_l[0] += in[0] * delta_l + in[kHalfWidth + 0] * delta;
      out_l[1] += in[1] * delta_l + in[kHalfWidth + 1] * delta;
      out_l[2] += in[2] * delta_l + in[kHalfWidth + 2] * delta;
      out_l[3] += in[3] * delta_l + in[kHalfWidth + 3] * delta;
      out_l[4] += in[4] * delta_l + in[kHalfWidth + 4] * delta;
      out_l[5] += in[5] * delta_l + in[kHalfWidth + 5] * delta;
      out_l[6] += in[6] * delta_l + in[kHalfWidth + 6] * delta;
      out_l[7] += in[7] * delta_l + in[kHalfWidth + 7] * delta;
      out_l[8] += rev[7] * delta_l + rev[7 - kHalfWidth] * delta;
      out_l[9] += rev[6] * delta_l + rev[6 - kHalfWidth] * delta;
      out_l[10] += rev[5] * delta_l + rev[5 - kHalfWidth] * delta;
      out_l[11] += rev[4] * delta_l + rev[4 - kHalfWidth] * delta;
      out_l[12] += rev[3] * delta_l + rev[3 - kHalfWidth] * delta;
      out_l[13] += rev[2] * delta_l + rev[2 - kHalfWidth] * delta;
      out_l[14] += rev[1] * delta_l + rev[1 - kHalfWidth] * delta;
      out_l[15] += rev[0] * delta_l + rev[0 - kHalfWidth] * delta;

      delta = (delta_r * interp) >> kDeltaBits;
      delta_r -= delta;
      out_r[0] += in[0] * delta_r + in[kHalfWidth + 0] * delta;
      out_r[1] += in[1] * delta_r + in[kHalfWidth + 1] * delta;
      out_r[2] += in[2] * delta_r + in[kHalfWidth + 2] * delta;
      out_r[3] += in[3] * delta_r + in[kHalfWidth + 3] * delta;
      out_r[4] += in[4] * delta_r + in[kHalfWidth + 4] * delta;
      out_r[5] += in[5] * delta_r + in[kHalfWidth + 5] * delta;
      out_r[6] += in[6] * delta_r + in[kHalfWidth + 6] * delta;
      out_r[7] += in[7] * delta_r + in[kHalfWidth + 7] * delta;
      out_r[8] += rev[7] * delta_r + rev[7 - kHalfWidth] * delta;
      out_r[9] += rev[6] * delta_r + rev[6 - kHalfWidth] * delta;
      out_r[10] += rev[5] * delta_r + rev[5 - kHalfWidth] * delta;
      out_r[11] += rev[4] * delta_r + rev[4 - kHalfWidth] * delta;
      out_r[12] += rev[3] * delta_r + rev[3 - kHalfWidth] * delta;
      out_r[13] += rev[2] * delta_r + rev[2 - kHalfWidth] * delta;
      out_r[14] += rev[1] * delta_r + rev[1 - kHalfWidth] * delta;
      out_r[15] += rev[0] * delta_r + rev[0 - kHalfWidth] * delta;
    }
  }
}

//------------------------------------------------------------------------------

void BlipBuffer::blip_add_delta_fast(unsigned int time, int delta_l, int delta_r)
{
  if (delta_l | delta_r) {
    unsigned fixed = (unsigned)((time * m_factor + m_offset) >> kPreShift);
    int interp = fixed >> (kFracBits - kDeltaBits) & (kDeltaUnit - 1);
    int pos = fixed >> kFracBits;

#ifdef STEREO_INVERT
    s32* out_l = m_buffer[1] + pos;
    s32* out_r = m_buffer[0] + pos;
#else
    s32* out_l = m_buffer[0] + pos;
    s32* out_r = m_buffer[1] + pos;
#endif

    int delta = delta_l * interp;

#ifdef BLIP_ASSERT
    // Fails if buffer size was exceeded.
    assert(pos <= m_size + kEndFrameExtra);
#endif

    if (delta_l == delta_r) {
      delta_l = delta_l * kDeltaUnit - delta;
      out_l[7] += delta_l;
      out_l[8] += delta;
      out_r[7] += delta_l;
      out_r[8] += delta;
    } else {
      out_l[7] += delta_l * kDeltaUnit - delta;
      out_l[8] += delta;
      delta = delta_r * interp;
      out_r[7] += delta_r * kDeltaUnit - delta;
      out_r[8] += delta;
    }
  }
}

//------------------------------------------------------------------------------

// [Hardware SG SGII SGII_RAM_EXT MARKIII SMS SMS2 GG GGMS MD PBC PICO]
int BlipBuffer::blip_read_samples(short out[], int count)
{
#ifdef BLIP_ASSERT
  assert(count >= 0);

  if (count > (m_offset >> kTimeBits))
    count = m_offset >> kTimeBits;

  if (count)
#endif
  {
    s32 const* in = m_buffer[0];
    s32 const* in2 = m_buffer[1];

    int sum = m_integrator[0];
    int sum2 = m_integrator[1];

    s32 const* end = in + count;

    do {
      // Eliminate fraction.
      int s = ARITH_SHIFT(sum, kDeltaBits);

      sum += *in++;

      CLAMP(s);

      *out++ = (short)s;

      // High-pass filter.
      sum -= s << (kDeltaBits - kBassShift);

      // Eliminate fraction.
      s = ARITH_SHIFT(sum2, kDeltaBits);

      sum2 += *in2++;

      CLAMP(s);

      *out++ = (short)s;

      // High-pass filter.
      sum2 -= s << (kDeltaBits - kBassShift);
    } while (in != end);

    m_integrator[0] = sum;
    m_integrator[1] = sum2;

    remove_samples(count);
  }

  return count;
}

//------------------------------------------------------------------------------

// [Hardware SCD]
int BlipBuffer::blip_mix_samples(BlipBuffer* m2, BlipBuffer* m3, short out[], int count)
{
#ifdef BLIP_ASSERT
  assert(count >= 0);

  if (count > (m_offset >> kTimeBits))
    count = m_offset >> kTimeBits;
  if (count > (m2->m_offset >> kTimeBits))
    count = m2->m_offset >> kTimeBits;
  if (count > (m3->m_offset >> kTimeBits))
    count = m3->m_offset >> kTimeBits;

  if (count)
#endif
  {
    s32 const* in[3] = { m_buffer[0], m2->m_buffer[0], m3->m_buffer[0] };
    s32 const* in2[3] = { m_buffer[1], m2->m_buffer[1], m3->m_buffer[1] };

    int sum = m_integrator[0];
    int sum2 = m_integrator[1];

    s32 const* end = in[0] + count;

    do {
      // Eliminate fraction.
      int s = ARITH_SHIFT(sum, kDeltaBits);

      sum += *in[0]++;
      sum += *in[1]++;
      sum += *in[2]++;

      CLAMP(s);

      *out++ = (short)s;

      // High-pass filter.
      sum -= s << (kDeltaBits - kBassShift);

      // Eliminate fraction.
      s = ARITH_SHIFT(sum2, kDeltaBits);

      sum2 += *in2[0]++;
      sum2 += *in2[1]++;
      sum2 += *in2[2]++;

      CLAMP(s);

      *out++ = (short)s;

      // High-pass filter.
      sum2 -= s << (kDeltaBits - kBassShift);
    } while (in[0] != end);

    m_integrator[0] = sum;
    m_integrator[1] = sum2;

    remove_samples(count);
    m2->remove_samples(count);
    m3->remove_samples(count);
  }

  return count;
}

//------------------------------------------------------------------------------

void BlipBuffer::remove_samples(int count)
{
  int remain = (m_offset >> kTimeBits) + kBufExtra - count;
  m_offset -= count * kTimeUnit;

  s32* buf = m_buffer[0];
  ::memmove(&buf[0], &buf[count], remain * sizeof(s32));
  ::memset(&buf[remain], 0, count * sizeof(s32));

  buf = m_buffer[1];
  ::memmove(&buf[0], &buf[count], remain * sizeof(s32));
  ::memset(&buf[remain], 0, count * sizeof(s32));
}

//------------------------------------------------------------------------------

void BlipBuffer::blip_end_frame(unsigned int clock_duration)
{
#ifdef BLIP_ASSERT
  // Fails if buffer size was exceeded.
  assert((m_offset >> kTimeBits) <= m_size);
#endif
  m_offset += clock_duration * m_factor;
}

//------------------------------------------------------------------------------

int BlipBuffer::blip_samples_avail()
{
  return (m_offset >> kTimeBits);
}

} // namespace gpgx::sound

