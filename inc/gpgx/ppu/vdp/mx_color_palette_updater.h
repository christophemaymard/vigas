/***************************************************************************************
 *  Genesis Plus GX
 *  Video Display Processor (color palette)
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

#ifndef __GPGX_PPU_VDP_MX_COLOR_PALETTE_UPDATER_H__
#define __GPGX_PPU_VDP_MX_COLOR_PALETTE_UPDATER_H__

#include "xee/fnd/data_type.h"

#include "core/vdp/pixel.h"

namespace gpgx::ppu::vdp {

//==============================================================================

//------------------------------------------------------------------------------

/// Updater of color palette in mode 0, 1, 2, 3 and 4.
class MXColorPaletteUpdater
{
private:
  /// Original SG-1000 palette.
  static constexpr PIXEL_OUT_T kTmsPalette[16] =
  {
#if defined(USE_8BPP_RENDERING)
  0x00, 0x00, 0x39, 0x79,
  0x4B, 0x6F, 0xC9, 0x5B,
  0xE9, 0xED, 0xD5, 0xD9,
  0x35, 0xCE, 0xDA, 0xFF
#elif defined(USE_15BPP_RENDERING)
  0x8000, 0x8000, 0x9308, 0xAF6F,
  0xA95D, 0xBDDF, 0xE949, 0xA3BE,
  0xFD4A, 0xFDEF, 0xEB0A, 0xF330,
  0x92A7, 0xE177, 0xE739, 0xFFFF
#elif defined(USE_16BPP_RENDERING)
  0x0000, 0x0000, 0x2648, 0x5ECF,
  0x52BD, 0x7BBE, 0xD289, 0x475E,
  0xF2AA, 0xFBCF, 0xD60A, 0xE670,
  0x2567, 0xC2F7, 0xCE59, 0xFFFF
#elif defined(USE_32BPP_RENDERING)
  0xFF000000, 0xFF000000, 0xFF21C842, 0xFF5EDC78,
  0xFF5455ED, 0xFF7D76FC, 0xFFD4524D, 0xFF42EBF5,
  0xFFFC5554, 0xFFFF7978, 0xFFD4C154, 0xFFE6CE80,
  0xFF21B03B, 0xFFC95BB4, 0xFFCCCCCC, 0xFFFFFFFF
#endif
  };

  /// Fixed Master System palette for modes 0, 1, 2 and 3.
  static constexpr u8 kTmsCRom[16] =
  {
    0x00, 0x00, 0x08, 0x0C,
    0x10, 0x30, 0x01, 0x3C,
    0x02, 0x03, 0x05, 0x0F,
    0x04, 0x33, 0x15, 0x3F
  };

public:
  MXColorPaletteUpdater(u8* reg, PIXEL_OUT_T* pixel, u8* system_hw);
  
  void Initialize();

  void UpdateColor(s32 index, u32 data);

private:
  u8* m_reg; /// Internal VDP registers (23 x 8-bit).

  PIXEL_OUT_T* m_pixel; /// Output pixel data look-up table.

  u8* m_system_hw;

  PIXEL_OUT_T m_pixel_lut[0x40];
};

} // namespace gpgx::ppu::vdp

#endif // #ifndef __GPGX_PPU_VDP_MX_COLOR_PALETTE_UPDATER_H__

