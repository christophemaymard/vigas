/***************************************************************************************
 *  Genesis Plus GX
 *  Video Display Processor (pixel output rendering)
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  Copyright (C) 2007-2016  Eke-Eke (Genesis Plus GX)
 *  Copyright (C) 2022  AlexKiri (enhanced vscroll mode rendering function)
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

#include "gpgx/ppu/vdp/m5_bg_pattern_cache_updater.h"

#include "core/macros.h" // For LSB_FIRST.

namespace gpgx::ppu::vdp {

//==============================================================================
// M5BackgroundPatternCacheUpdater

//------------------------------------------------------------------------------
M5BackgroundPatternCacheUpdater::M5BackgroundPatternCacheUpdater(
  u8* pattern_cache, 
  u16* name_list, 
  u8* name_dirty, 
  u8* ram) :
  m_pattern_cache(pattern_cache),
  m_name_list(name_list),
  m_name_dirty(name_dirty),
  m_ram(ram)
{
}

//------------------------------------------------------------------------------

void M5BackgroundPatternCacheUpdater::UpdateBackgroundPatternCache(s32 index)
{
  u8 x = 0;
  u8 y = 0;
  u8 c = 0;
  u8* dst = nullptr;
  u16 name = 0;
  u32 bp = 0;

  for (s32 i = 0; i < index; i++) {
    // Get modified pattern name index.
    name = m_name_list[i];
    
    // Pattern cache base address.
    dst = &m_pattern_cache[name << 6];

    // Check modified lines.
    for (y = 0; y < 8; y++) {
      if (m_name_dirty[name] & (1 << y)) {
        // Byteplane data (one pattern = 4 bytes).
        // LIT_ENDIAN: byte0 (lsb) p2p3 p0p1 p6p7 p4p5 (msb) byte3
        // BIG_ENDIAN: byte0 (msb) p0p1 p2p3 p4p5 p6p7 (lsb) byte3
        bp = *(u32*)&m_ram[(name << 5) | (y << 2)];

        // Update cached line (8 pixels = 8 bytes).
        for (x = 0; x < 8; x++) {
          // Extract pixel data.
          c = bp & 0x0F;

          // Pattern cache data (one pattern = 8 bytes).
          // byte0 <-> p0 p1 p2 p3 p4 p5 p6 p7 <-> byte7 (hflip = 0)
          // byte0 <-> p7 p6 p5 p4 p3 p2 p1 p0 <-> byte7 (hflip = 1)
#ifdef LSB_FIRST
          // Byteplane data = (msb) p4p5 p6p7 p0p1 p2p3 (lsb)
          dst[0x00000 | (y << 3) | (x ^ 3)] = (c);        // vflip=0, hflip=0
          dst[0x20000 | (y << 3) | (x ^ 4)] = (c);        // vflip=0, hflip=1
          dst[0x40000 | ((y ^ 7) << 3) | (x ^ 3)] = (c);  // vflip=1, hflip=0
          dst[0x60000 | ((y ^ 7) << 3) | (x ^ 4)] = (c);  // vflip=1, hflip=1
#else
          // Byteplane data = (msb) p0p1 p2p3 p4p5 p6p7 (lsb)
          dst[0x00000 | (y << 3) | (x ^ 7)] = (c);        // vflip=0, hflip=0
          dst[0x20000 | (y << 3) | (x)] = (c);            // vflip=0, hflip=1
          dst[0x40000 | ((y ^ 7) << 3) | (x ^ 7)] = (c);  // vflip=1, hflip=0
          dst[0x60000 | ((y ^ 7) << 3) | (x)] = (c);      // vflip=1, hflip=1
#endif
          // Next pixel.
          bp = bp >> 4;
        }
      }
    }
    
    // Clear modified pattern flag.
    m_name_dirty[name] = 0;
  }
}

} // namespace gpgx::ppu::vdp

