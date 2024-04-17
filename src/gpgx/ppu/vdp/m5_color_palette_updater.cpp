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

#include "gpgx/ppu/vdp/m5_color_palette_updater.h"

#include "xee/mem/memory.h" // For Memset().

namespace gpgx::ppu::vdp {

//==============================================================================
// M5ColorPaletteUpdater

//------------------------------------------------------------------------------

M5ColorPaletteUpdater::M5ColorPaletteUpdater(u8* reg, PIXEL_OUT_T* pixel) : 
  m_reg(reg),
  m_pixel(pixel)
{
  xee::mem::Memset(&m_pixel_lut, 0, sizeof(m_pixel_lut));
}

//------------------------------------------------------------------------------

void M5ColorPaletteUpdater::Initialize()
{
  // Each R,G,B color channel is 4-bit with a total of 15 different intensity levels.
  // 
  // Color intensity depends on the mode:
  // - normal   : xxx0     (0-14)
  // - shadow   : 0xxx     (0-7)
  // - highlight: 1xxx - 1 (7-14)
  // 
  // with:
  // - x = original CRAM value (2, 3 or 4-bit)

  s32 r = 0;
  s32 g = 0;
  s32 b = 0;

  // Initialize Mode 5 pixel color look-up tables.
  for (s32 i = 0; i < 0x200; i++) {
    // CRAM 9-bit value (BBBGGGRRR).
    r = (i >> 0) & 7;
    g = (i >> 3) & 7;
    b = (i >> 6) & 7;

    // Convert to output pixel format.
    m_pixel_lut[0][i] = MAKE_PIXEL(r, g, b);
    m_pixel_lut[1][i] = MAKE_PIXEL(r << 1, g << 1, b << 1);
    m_pixel_lut[2][i] = MAKE_PIXEL(r + 7, g + 7, b + 7);
  }
}

//------------------------------------------------------------------------------

void M5ColorPaletteUpdater::UpdateColor(s32 index, u32 data)
{
  // Palette Mode.
  if (!(m_reg[0] & 0x04)) {
    // Color value is limited to 00X00X00X.
    data &= 0x49;
  }

  if (m_reg[12] & 0x08) {
    // Mode 5 (Shadow/Normal/Highlight).
    m_pixel[0x00 | index] = m_pixel_lut[0][data];
    m_pixel[0x40 | index] = m_pixel_lut[1][data];
    m_pixel[0x80 | index] = m_pixel_lut[2][data];
  } else {
    // Mode 5 (Normal).
    data = m_pixel_lut[1][data];

    // Input pixel: xxiiiiii.
    m_pixel[0x00 | index] = data;
    m_pixel[0x40 | index] = data;
    m_pixel[0x80 | index] = data;
  }
}

} // namespace gpgx::ppu::vdp

