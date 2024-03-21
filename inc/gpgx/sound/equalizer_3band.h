/**
 * 3 band equalizer.
 * 
 * (c) Neil C / Etanza Systems / 2K6
 * Shouts / Loves / Moans = etanza at lycos dot co dot uk 
 * 
 * This work is hereby placed in the public domain for all purposes, including
 * use in commercial applications.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __GPGX_SOUND_EQUALIZER_3BAND_H__
#define __GPGX_SOUND_EQUALIZER_3BAND_H__

#include "xee/fnd/data_type.h"

namespace gpgx::sound {

//==============================================================================

//------------------------------------------------------------------------------

class Equalizer3band
{
private:
  static constexpr f64 kVsa = (1.0 / 4294967295.0); // Very small amount (Denormal Fix).
  static constexpr f32 kPi = 3.14159265358979323846264338327f;
public:
  Equalizer3band();

  // Initialise equalizer.
  // Recommended frequencies are:
  // - lowfreq  = 880  Hz
  // - highfreq = 5000 Hz
  // Set mixfreq to whatever rate your system is using (eg 48Khz).
  void init_3band_state(int lowfreq, int highfreq, int mixfreq);

  // Equalize one sample.
  // Sample can be any range you like.
  // Note that the output will depend on the gain settings for each band 
  // (especially the bass) so may require clipping before output, 
  // but you knew that anyway.
  f64 do_3band(int sample);

  // Set the gain control of the low band.
  void SetLowGainControl(f64 gain) { m_lg = gain; }

  // Set the gain control of the middle band.
  void SetMiddleGainControl(f64 gain) { m_mg = gain; }

  // Set the gain control of the high band.
  void SetHighGainControl(f64 gain) { m_hg = gain; }

private:
  f64 m_lf;      // Filter #1 (Low band): Frequency.
  f64 m_f1p0;    // Filter #1 (Low band): Pole 0.
  f64 m_f1p1;    // Filter #1 (Low band): Pole 1.
  f64 m_f1p2;    // Filter #1 (Low band): Pole 2.
  f64 m_f1p3;    // Filter #1 (Low band): Pole 3.

  f64 m_hf;      // Filter #2 (High band): Frequency.
  f64 m_f2p0;    // Filter #2 (High band): Pole 0.
  f64 m_f2p1;    // Filter #2 (High band): Pole 1.
  f64 m_f2p2;    // Filter #2 (High band): Pole 2.
  f64 m_f2p3;    // Filter #2 (High band): Pole 3.

  f64 m_sdm1;      // Sample history buffer: Sample data minus 1.
  f64 m_sdm2;      // Sample history buffer: Sample data minus 2.
  f64 m_sdm3;      // Sample history buffer: Sample data minus 3.

  f64 m_lg;      // Gain Control: low  gain.
  f64 m_mg;      // Gain Control: mid  gain.
  f64 m_hg;      // Gain Control: high gain.
};

} // namespace gpgx::sound

#endif // #ifndef __GPGX_SOUND_EQUALIZER_3BAND_H__