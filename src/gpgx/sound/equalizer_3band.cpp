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

#include "gpgx/sound/equalizer_3band.h"

#include <cmath>

namespace gpgx::sound {

//==============================================================================
// Equalizer3band

//------------------------------------------------------------------------------

Equalizer3band::Equalizer3band()
{
  // Clear Filter #1 (Low band).
  m_lf = 0.0;
  m_f1p0 = 0.0;
  m_f1p1 = 0.0;
  m_f1p2 = 0.0;
  m_f1p3 = 0.0;

  // Clear Filter #2 (High band).
  m_hf = 0.0;
  m_f2p0 = 0.0;
  m_f2p1 = 0.0;
  m_f2p2 = 0.0;
  m_f2p3 = 0.0;

  // Clear sample history buffers.
  m_sdm1 = 0;
  m_sdm2 = 0;
  m_sdm3 = 0;

  // Clear gain controls.
  m_lg = 0.0;
  m_mg = 0.0;
  m_hg = 0.0;
}

//------------------------------------------------------------------------------

void Equalizer3band::init_3band_state(int lowfreq, int highfreq, int mixfreq)
{
  // Clear Filter #1 (Low band) poles.
  m_f1p0 = 0.0;
  m_f1p1 = 0.0;
  m_f1p2 = 0.0;
  m_f1p3 = 0.0;

  // Clear Filter #2 (High band) poles.
  m_f2p0 = 0.0;
  m_f2p1 = 0.0;
  m_f2p2 = 0.0;
  m_f2p3 = 0.0;

  // Clear sample history buffers.
  m_sdm1 = 0;
  m_sdm2 = 0;
  m_sdm3 = 0;

  // Set Low/Mid/High gains to unity.
  m_lg = 1.0;
  m_mg = 1.0;
  m_hg = 1.0;

  // Calculate filter cutoff frequencies.
  m_lf = 2 * ::sin(kPi * ((double)lowfreq / (double)mixfreq));
  m_hf = 2 * ::sin(kPi * ((double)highfreq / (double)mixfreq));
}

//------------------------------------------------------------------------------

double Equalizer3band::do_3band(int sample)
{
  // Filter #1 (lowpass).

  m_f1p0 += (m_lf * ((double)sample - m_f1p0)) + kVsa;
  m_f1p1 += (m_lf * (m_f1p0 - m_f1p1));
  m_f1p2 += (m_lf * (m_f1p1 - m_f1p2));
  m_f1p3 += (m_lf * (m_f1p2 - m_f1p3));

  // Low sample value.
  double l = m_f1p3;

  // Filter #2 (highpass).

  m_f2p0 += (m_hf * ((double)sample - m_f2p0)) + kVsa;
  m_f2p1 += (m_hf * (m_f2p0 - m_f2p1));
  m_f2p2 += (m_hf * (m_f2p1 - m_f2p2));
  m_f2p3 += (m_hf * (m_f2p2 - m_f2p3));

  // High sample value.
  double h = m_sdm3 - m_f2p3;

  // Calculate midrange (signal - (low + high)) value.

  // m = es->sdm3 - (h + l);.
  // fix from http://www.musicdsp.org/showArchiveComment.php?ArchiveID=236 ?
  double m = sample - (h + l);

  // Scale, Combine and store.

  l *= m_lg;
  m *= m_mg;
  h *= m_hg;

  // Update history buffer.

  m_sdm3 = m_sdm2;
  m_sdm2 = m_sdm1;
  m_sdm1 = sample;

  return (l + m + h);
}

} // namespace gpgx::sound

