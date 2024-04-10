/***************************************************************************************
 *  Genesis Plus GX
 *  Video Display Processor (sprite layer rendering)
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

#include "gpgx/ppu/vdp/tms_sprite_layer_renderer.h"

#include "xee/mem/memory.h"

#include "core/vdp/object_info_t.h"
#include "core/system_model.h" // For SYSTEM_GG.

namespace gpgx::ppu::vdp {

//==============================================================================
// TmsSpriteLayerRenderer

//------------------------------------------------------------------------------

TmsSpriteLayerRenderer::TmsSpriteLayerRenderer(
  object_info_t (&obj_info)[2][20],
  u8* object_count,
  u8* spr_ovr,
  u16* status,
  u8* reg,
  u8* lut,
  u8* line_buffer,
  u8* vram,
  u8* system_hw,
  core_config_t* core_config,
  u16* v_counter,
  viewport_t* viewport) :
  m_obj_info(obj_info),
  m_object_count(object_count),
  m_spr_ovr(spr_ovr),
  m_status(status),
  m_reg(reg),
  m_lut(lut),
  m_line_buffer(line_buffer),
  m_vram(vram),
  m_system_hw(system_hw),
  m_core_config(core_config),
  m_v_counter(v_counter),
  m_viewport(viewport)
{
}

//------------------------------------------------------------------------------

void TmsSpriteLayerRenderer::RenderSprites(s32 line)
{
  s32 x = 0;
  s32 start = 0;
  s32 end = 0;
  u8* lb = nullptr;
  u8* sg = nullptr;
  u8 color = 0;
  u8 pattern[2] = { 0, 0 };
  u16 temp = 0;

  // Sprite list for current line.
  object_info_t* object_info = m_obj_info[line];
  s32 count = m_object_count[line];

  // Default sprite width (8 pixels).
  s32 width = 8;

  // Adjust width for 16x16 sprites.
  width <<= ((m_reg[1] & 0x02) >> 1);

  // Adjust width for zoomed sprites.
  width <<= (m_reg[1] & 0x01);

  // Latch SOVR flag from previous line to VDP status.
  *m_status |= *m_spr_ovr;

  // Clear SOVR flag for current line.
  *m_spr_ovr = 0;

  // Draw sprites in front-to-back order.
  while (count--) {
    // Sprite X position.
    start = object_info->xpos;

    // Sprite Color + Early Clock bit.
    color = object_info->size;

    // X position shift (32 pixels).
    start -= ((color & 0x80) >> 2);

    // Pointer to line buffer.
    lb = &m_line_buffer[0x20 + start];

    if ((start + width) > 256) {
      // Clip sprites on right edge.
      end = 256 - start;
      start = 0;
    } else {
      end = width;

      if (start < 0) {
        // Clip sprites on left edge.
        start = 0 - start;
      } else {
        start = 0;
      }
    }

    // Sprite Color (0-15).
    color &= 0x0F;

    // Sprite Pattern Name.
    temp = object_info->attr;

    // Mask two LSB for 16x16 sprites.
    temp &= ~((m_reg[1] & 0x02) >> 0);
    temp &= ~((m_reg[1] & 0x02) >> 1);

    // Pointer to sprite generator table.
    sg = (u8*)&m_vram[((m_reg[6] << 11) & 0x3800) | (temp << 3) | object_info->ypos];

    // Sprite Pattern data (2 x 8 pixels).
    pattern[0] = sg[0x00];
    pattern[1] = sg[0x10];

    if (m_reg[1] & 0x01) {
      // Zoomed sprites are rendered at half speed.
      for (x = start; x < end; x += 2) {
        temp = pattern[(x >> 4) & 1];
        temp = (temp >> (7 - ((x >> 1) & 7))) & 0x01;
        temp = temp * color;
        temp |= (lb[x] << 8);
        lb[x] = m_lut[temp];
        *m_status |= ((temp & 0x8000) >> 10);
        temp &= 0x00FF;
        temp |= (lb[x + 1] << 8);
        lb[x + 1] = m_lut[temp];
        *m_status |= ((temp & 0x8000) >> 10);
      }
    } else {
      // Normal sprites.
      for (x = start; x < end; x++) {
        temp = pattern[(x >> 3) & 1];
        temp = (temp >> (7 - (x & 7))) & 0x01;
        temp = temp * color;
        temp |= (lb[x] << 8);
        lb[x] = m_lut[temp];
        *m_status |= ((temp & 0x8000) >> 10);
      }
    }

    // Next sprite entry.
    object_info++;
  }

  // handle Game Gear reduced screen (160x144).
  if ((*m_system_hw == SYSTEM_GG) && !m_core_config->gg_extra && (*m_v_counter < m_viewport->h)) {
    s32 line = *m_v_counter - (m_viewport->h - 144) / 2;

    if ((line < 0) || (line >= 144)) {
      xee::mem::Memset(&m_line_buffer[0x20], 0x40, 256);
    } else {
      if (m_viewport->x > 0) {
        xee::mem::Memset(&m_line_buffer[0x20], 0x40, 48);
        xee::mem::Memset(&m_line_buffer[0x20 + 48 + 160], 0x40, 48);
      }
    }
  }
}

} // namespace gpgx::ppu::vdp

