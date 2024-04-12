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

#include "gpgx/ppu/vdp/m1x_bg_layer_renderer.h"

#include "xee/mem/memory.h" // For Memset().

#include "core/system_model.h" // For SYSTEM_SMS.

namespace gpgx::ppu::vdp {

//==============================================================================
// M1XBackgroundLayerRenderer

//------------------------------------------------------------------------------

M1XBackgroundLayerRenderer::M1XBackgroundLayerRenderer(
  u8* reg,
  u8* line_buffer,
  u8* vram,
  u8* system_hw) :
  m_reg(reg),
  m_line_buffer(line_buffer),
  m_vram(vram),
  m_system_hw(system_hw)
{
}

//------------------------------------------------------------------------------

void M1XBackgroundLayerRenderer::RenderBackground(s32 line)
{
  u8 color = m_reg[7];

  u8* lb = &m_line_buffer[0x20];
  u8* nt = &m_vram[((m_reg[2] << 10) & 0x3C00) + ((line >> 3) * 40)];

  u16 pg_mask = ~0x3800 ^ (m_reg[4] << 11);

  // 40 x 6 pixels.
  s32 width = 40;

  // Unused bits used as a mask on TMS99xx & 315-5124 VDP only.
  if (*m_system_hw > SYSTEM_SMS) {
    pg_mask |= 0x1800;
  }

  u8* pg = &m_vram[((0x2000 + ((line & 0xC0) << 5)) & pg_mask) + (line & 7)];

  // Left border (8 pixels).
  xee::mem::Memset(lb, 0x40, 8);
  lb += 8;

  u8 pattern = 0;

  do {
    pattern = pg[*nt++ << 3];

    *lb++ = 0x10 | ((color >> (((pattern >> 7) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 6) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 5) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 4) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 3) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 2) & 1) << 2)) & 0x0F);
  } while (--width);

  // Right borders (8 pixels).
  xee::mem::Memset(lb, 0x40, 8);
}

} // namespace gpgx::ppu::vdp

