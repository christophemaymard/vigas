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

#include "gpgx/ppu/vdp/m4_bg_layer_renderer.h"

#include "xee/mem/memory.h" // For Memset().

#include "core/system_model.h" // For SYSTEM_SMS.

namespace gpgx::ppu::vdp {

//==============================================================================
// M4BackgroundLayerRenderer

//------------------------------------------------------------------------------

M4BackgroundLayerRenderer::M4BackgroundLayerRenderer(
  u8* reg,
  u16* vscroll,
  u8* pattern_cache,
  u8* line_buffer,
  const u32* atex_table,
  u8* vram,
  u8* system_hw,
  viewport_t* viewport) : 
  m_reg(reg),
  m_vscroll(vscroll),
  m_pattern_cache(pattern_cache),
  m_line_buffer(line_buffer),
  m_atex_table(atex_table),
  m_vram(vram),
  m_system_hw(system_hw),
  m_viewport(viewport)
{
}

//------------------------------------------------------------------------------

void M4BackgroundLayerRenderer::RenderBackground(s32 line)
{
  // 32 x 8 pixels.
  s32 width = 32;

  // Horizontal scrolling.
  s32 index = ((m_reg[0] & 0x40) && (line < 0x10)) ? 0x100 : m_reg[0x08];
  s32 shift = index & 7;

  // Background line buffer.
  u32* dst = (u32*)&m_line_buffer[0x20 + shift];

  // Vertical scrolling.
  s32 v_line = line + *m_vscroll;

  // Pattern name table mask.
  u16 nt_mask = ~0x3C00 ^ (m_reg[2] << 10);

  // Unused bits used as a mask on TMS99xx & 315-5124 VDP only.
  if (*m_system_hw > SYSTEM_SMS) {
    nt_mask |= 0x400;
  }

  u16* nt = nullptr;

  // Test for extended modes (Master System II & Game gear VDP only).
  if (m_viewport->h > 192) {
    // Vertical scroll mask.
    v_line = v_line % 256;

    // Pattern name Table.
    nt = (u16*)&m_vram[(0x3700 & nt_mask) + ((v_line >> 3) << 6)];
  } else {
    // Vertical scroll mask.
    v_line = v_line % 224;

    // Pattern name Table.
    nt = (u16*)&m_vram[(0x3800 + ((v_line >> 3) << 6)) & nt_mask];
  }

  // Pattern row index.
  v_line = (v_line & 7) << 3;

  // Tile column index.
  index = (0x100 - index) >> 3;

  // Clip left-most column if required.
  if (shift) {
    xee::mem::Memset(&m_line_buffer[0x20], 0, shift);
    index++;
  }

  u32* src = nullptr;
  u32 attr = 0;
  u32 atex = 0;

  // Draw tiles.
  for (s32 column = 0; column < width; column++, index++) {
    // Stop vertical scrolling for rightmost eight tiles.
    if ((column == 24) && (m_reg[0] & 0x80)) {
      // Clear Pattern name table start address.
      if (m_viewport->h > 192) {
        nt = (u16*)&m_vram[(0x3700 & nt_mask) + ((line >> 3) << 6)];
      } else {
        nt = (u16*)&m_vram[(0x3800 + ((line >> 3) << 6)) & nt_mask];
      }

      // Clear Pattern row index.
      v_line = (line & 7) << 3;
    }

    // Read name table attribute word.
    attr = nt[index % width];
#ifndef LSB_FIRST
    attr = (((attr & 0xFF) << 8) | ((attr & 0xFF00) >> 8));
#endif

    // Expand priority and palette bits.
    atex = m_atex_table[(attr >> 11) & 3];

    // Cached pattern data line (4 bytes = 4 pixels at once).
    src = (u32*)&m_pattern_cache[((attr & 0x7FF) << 6) | (v_line)];

    // Copy left & right half, adding the attribute bits in.
    *dst++ = (src[0] | atex);
    *dst++ = (src[1] | atex);
  }
}

} // namespace gpgx::ppu::vdp

