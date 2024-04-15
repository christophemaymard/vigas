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

#include "gpgx/ppu/vdp/m4_sprite_layer_renderer.h"

#include "xee/mem/memory.h"

#include "core/vdp/object_info_t.h"
#include "core/system_model.h"

#include "gpgx/ppu/vdp/m4_sprite_tile_drawer.h"
#include "gpgx/ppu/vdp/m4_zoomed_sprite_tile_drawer.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// M4SpriteLayerRenderer

//------------------------------------------------------------------------------

M4SpriteLayerRenderer::M4SpriteLayerRenderer(
  object_info_t (&obj_info)[2][20],
  u8* object_count,
  u16* status,
  u8* reg,
  u16* spr_col,
  u8* spr_ovr,
  u16* v_counter,
  u8* pattern_cache,
  u8* lut,
  u8* line_buffer,
  u8* system_hw,
  core_config_t* config,
  viewport_t* viewport) : 
  m_obj_info(obj_info),
  m_object_count(object_count),
  m_status(status),
  m_reg(reg),
  m_spr_col(spr_col),
  m_spr_ovr(spr_ovr),
  m_v_counter(v_counter),
  m_pattern_cache(pattern_cache),
  m_line_buffer(line_buffer),
  m_system_hw(system_hw),
  m_config(config),
  m_viewport(viewport)
{
  m_sprite_tile_drawer = new M4SpriteTileDrawer(
    m_status, 
    m_v_counter, 
    m_spr_col, 
    lut
  );
  m_zoomed_sprite_tile_drawer = new M4ZoomedSpriteTileDrawer(
    m_status,
    m_v_counter,
    m_spr_col,
    lut
  );
}

//------------------------------------------------------------------------------

void M4SpriteLayerRenderer::RenderSprites(s32 line)
{
  s32 xpos = 0;
  s32 end = 0;
  u8* src = nullptr;
  u8* lb = nullptr;
  u16 temp = 0;

  // Sprite list for current line.
  object_info_t* object_info = m_obj_info[line];
  s32 count = m_object_count[line];

  // Default sprite width.
  s32 width = 8;

  // Sprite Generator address mask (LSB is masked for 8x16 sprites).
  u16 sg_mask = (~0x1C0 ^ (m_reg[6] << 6)) & (~((m_reg[1] & 0x02) >> 1));

  // Zoomed sprites (not working on Genesis VDP).
  if (*m_system_hw < SYSTEM_MD) {
    width <<= (m_reg[1] & 0x01);
  }

  // Unused bits used as a mask on 315-5124 VDP only.
  if (*m_system_hw > SYSTEM_SMS) {
    sg_mask |= 0xC0;
  }

  // Latch SOVR flag from previous line to VDP status.
  *m_status |= *m_spr_ovr;

  // Clear SOVR flag for current line.
  *m_spr_ovr = 0;

  // Draw sprites in front-to-back order.
  while (count--) {
    // Sprite pattern index.
    temp = (object_info->attr | 0x100) & sg_mask;

    // Pointer to pattern cache line.
    src = (u8*)&m_pattern_cache[(temp << 6) | (object_info->ypos << 3)];

    // Sprite X position.
    xpos = object_info->xpos;

    // X position shift.
    xpos -= (m_reg[0] & 0x08);

    if (xpos < 0) {
      // Clip sprites on left edge.
      src = src - xpos;
      end = xpos + width;
      xpos = 0;
    } else if ((xpos + width) > 256) {
      // Clip sprites on right edge.
      end = 256 - xpos;
    } else {
      // Sprite maximal width.
      end = width;
    }

    // Pointer to line buffer.
    lb = &m_line_buffer[0x20 + xpos];

    if (width > 8) {
      // Draw sprite pattern (zoomed sprites are rendered at half speed).
      m_zoomed_sprite_tile_drawer->DrawSpriteTile(end, src, lb, xpos);

      // 315-5124 VDP specific.
      if (*m_system_hw < SYSTEM_SMS2) {
        // only 4 first sprites can be zoomed.
        if (count == (m_object_count[line] - 4)) {
          // Set default width for remaining sprites.
          width = 8;
        }
      }
    } else {
      // Draw sprite pattern.
      m_sprite_tile_drawer->DrawSpriteTile(end, src, lb, xpos);
    }

    // Next sprite entry.
    object_info++;
  }

  // handle Game Gear reduced screen (160x144).
  if ((*m_system_hw == SYSTEM_GG) && !m_config->gg_extra && (*m_v_counter < m_viewport->h)) {
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

