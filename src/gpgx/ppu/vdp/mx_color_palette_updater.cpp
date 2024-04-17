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

#include "gpgx/ppu/vdp/mx_color_palette_updater.h"

#include "xee/mem/memory.h"

#include "core/system_model.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// MXColorPaletteUpdater

//------------------------------------------------------------------------------

MXColorPaletteUpdater::MXColorPaletteUpdater(
  u8* reg, 
  PIXEL_OUT_T* pixel, 
  u8* system_hw) :
  m_reg(reg),
  m_pixel(pixel),
  m_system_hw(system_hw)
{
  xee::mem::Memset(&m_pixel_lut, 0, sizeof(m_pixel_lut));
}

//------------------------------------------------------------------------------

void MXColorPaletteUpdater::Initialize()
{
  // Each R,G,B color channel is 4-bit with a total of 15 different intensity levels.
  // 
  // Color intensity depends on the mode:
  // - normal   : xxx0     (0-14)
  // - mode4    : xxxx(*)  (0-15)
  // - GG mode  : xxxx     (0-15)
  // 
  // with:
  // - x = original CRAM value (2, 3 or 4-bit)
  // - (*) = 2-bit CRAM value is expanded to 4-bit

  s32 r = 0;
  s32 g = 0;
  s32 b = 0;

  // Initialize Mode 4 pixel color look-up table.
  for (s32 i = 0; i < 0x40; i++) {
    // CRAM 6-bit value (000BBGGRR).
    r = (i >> 0) & 3;
    g = (i >> 2) & 3;
    b = (i >> 4) & 3;

    // Expand to full range & convert to output pixel format.
    m_pixel_lut[i] = MAKE_PIXEL((r << 2) | r, (g << 2) | g, (b << 2) | b);
  }
}

//------------------------------------------------------------------------------

void MXColorPaletteUpdater::UpdateColor(s32 index, u32 data)
{
  switch (*m_system_hw) {
    case SYSTEM_GG:
    {
      // CRAM value (BBBBGGGGRRRR).
      s32 r = (data >> 0) & 0x0F;
      s32 g = (data >> 4) & 0x0F;
      s32 b = (data >> 8) & 0x0F;

      // Convert to output pixel.
      data = MAKE_PIXEL(r, g, b);
      break;
    }

    case SYSTEM_SG:
    case SYSTEM_SGII:
    case SYSTEM_SGII_RAM_EXT:
    {
      // Fixed TMS99xx palette.
      if (index & 0x0F) {
        // Colors 1-15.
        data = kTmsPalette[index & 0x0F];
      } else {
        // Backdrop color.
        data = kTmsPalette[m_reg[7] & 0x0F];
      }
      break;
    }

    default:
    {
      // Test M4 bit.
      if (!(m_reg[0] & 0x04)) {
        if (*m_system_hw & SYSTEM_MD) {
          // Invalid Mode (black screen).
          data = 0x00;
        } else if (*m_system_hw != SYSTEM_GGMS) {
          // Fixed CRAM palette.
          if (index & 0x0F) {
            // Colors 1-15.
            data = kTmsCRom[index & 0x0F];
          } else {
            // Backdrop color.
            data = kTmsCRom[m_reg[7] & 0x0F];
          }
        }
      }

      // Mode 4 palette.
      data = m_pixel_lut[data & 0x3F];
      break;
    }
  }

  // Input pixel: x0xiiiii (normal) or 01000000 (backdrop).
  if (m_reg[0] & 0x04) {
    // Mode 4.
    m_pixel[0x00 | index] = data;
    m_pixel[0x20 | index] = data;
    m_pixel[0x80 | index] = data;
    m_pixel[0xA0 | index] = data;
  } else {
    // TMS99xx modes (palette bit forced to 1 because Game Gear uses CRAM palette #1).
    if ((index == 0x40) || (index == (0x10 | (m_reg[7] & 0x0F)))) {
      // Update backdrop color.
      m_pixel[0x40] = data;

      // Update transparent color.
      m_pixel[0x10] = data;
      m_pixel[0x30] = data;
      m_pixel[0x90] = data;
      m_pixel[0xB0] = data;
    }

    if (index & 0x0F) {
      // update non-transparent colors.
      m_pixel[0x00 | index] = data;
      m_pixel[0x20 | index] = data;
      m_pixel[0x80 | index] = data;
      m_pixel[0xA0 | index] = data;
    }
  }
}

} // namespace gpgx::ppu::vdp

