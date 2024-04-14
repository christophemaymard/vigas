/***************************************************************************************
 *  Genesis Plus GX
 *  Video Display Processor (background layer rendering)
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

#ifndef __GPGX_PPU_VDP_M5_IM2_BG_COLUMN_DRAWER_H__
#define __GPGX_PPU_VDP_M5_IM2_BG_COLUMN_DRAWER_H__

#include "xee/fnd/data_type.h"

namespace gpgx::ppu::vdp {

//==============================================================================

//------------------------------------------------------------------------------


/// Column drawer in background layer rendering mode 5 with interlace double 
/// resolution (IM2) enabled.
/// 
/// One column = 2 tiles
/// Two pattern attributes are written in VRAM as two consecutives 16-bit words:
/// 
/// P = priority bit
/// C = color palette (2 bits)
/// V = Vertical Flip bit
/// H = Horizontal Flip bit
/// N = Pattern Number (11 bits)
/// 
/// (MSB) PCCVHNNN NNNNNNNN (LSB) (MSB) PCCVHNNN NNNNNNNN (LSB)
///           PATTERN1                      PATTERN2
/// 
/// Both pattern attributes are read from VRAM as one 32-bit word:
/// 
/// LIT_ENDIAN: (MSB) PCCVHNNN NNNNNNNN PCCVHNNN NNNNNNNN (LSB)
///                       PATTERN2          PATTERN1
/// 
/// BIG_ENDIAN: (MSB) PCCVHNNN NNNNNNNN PCCVHNNN NNNNNNNN (LSB)
///                       PATTERN1          PATTERN2
/// 
/// 
/// In line buffers, one pixel = one byte: (msb) 0Pppcccc (lsb)
/// with:
///   P = priority bit  (from pattern attribute)
///   p = color palette (from pattern attribute)
///   c = color data (from pattern cache)
/// 
/// One pattern = 8 pixels = 8 bytes = two 32-bit writes per pattern
class M5Im2BackgroundColumnDrawer
{
public:
  M5Im2BackgroundColumnDrawer(const u32* atex_table, u8* pattern_cache);

  /// Draw 2-cell column (16 pixels high).
  /// 
  /// @param  dest
  /// @param  attr
  /// @param  line
  void DrawColumn(u32** dest, u32 attr, u32 line);

private:
  void DrawLSBTile(u32** dest, u32 attr, u32 line);

  void DrawMSBTile(u32** dest, u32 attr, u32 line);

private:
  const u32* m_atex_table; /// Pattern attribute (priority + palette bits) expansion table.

  /// Cached and flipped patterns.
  /// 
  /// Pattern cache base address: VHN NNNNNNNN NYYYYxxx
  /// with :
  ///   x = Pattern Pixel (0-7)
  ///   Y = Pattern Row (0-15)
  ///   N = Pattern Number (0-1023)
  ///   H = Horizontal Flip bit
  ///   V = Vertical Flip bit
  u8* m_pattern_cache;
};

} // namespace gpgx::ppu::vdp

#endif // #ifndef __GPGX_PPU_VDP_M5_IM2_BG_COLUMN_DRAWER_H__

