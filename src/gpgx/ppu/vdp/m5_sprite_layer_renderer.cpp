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

#include "gpgx/ppu/vdp/m5_sprite_layer_renderer.h"

#include "core/vdp/object_info_t.h"

#include "gpgx/ppu/vdp/m5_sprite_tile_drawer.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// M5SpriteLayerRenderer

//------------------------------------------------------------------------------

M5SpriteLayerRenderer::M5SpriteLayerRenderer(
  object_info_t (&obj_info)[2][20],
  u8* object_count,
  u16* status,
  u8* spr_ovr,
  u8* pattern_cache,
  u8* line_buffer,
  u8* lut,
  u8* name_lut,
  u16* m_max_sprite_pixels,
  viewport_t* viewport) :
  m_obj_info(obj_info), 
  m_object_count(object_count), 
  m_spr_ovr(spr_ovr), 
  m_pattern_cache(pattern_cache), 
  m_line_buffer(line_buffer), 
  m_name_lut(name_lut), 
  m_max_sprite_pixels(m_max_sprite_pixels), 
  m_viewport(viewport)
{
  m_sprite_tile_drawer = new gpgx::ppu::vdp::M5SpriteTileDrawer(status, lut);
}

//------------------------------------------------------------------------------

void M5SpriteLayerRenderer::RenderSprites(s32 line)
{
  s32 i = 0;
  s32 column = 0;
  s32 xpos = 0;
  s32 width = 0;
  s32 pixelcount = 0;
  s32 masked = 0;
  s32 max_pixels = *m_max_sprite_pixels;

  u8* src = nullptr;
  u8* s = nullptr;
  u8* lb = nullptr;
  u32 temp = 0;
  u32 v_line = 0;
  u32 attr = 0;
  u32 name = 0;
  u32 atex = 0;

  // Sprite list for current line.
  object_info_t* object_info = m_obj_info[line];
  s32 count = m_object_count[line];

  // Draw sprites in front-to-back order.
  while (count--) {
    // Sprite X position.
    xpos = object_info->xpos;

    // Sprite masking.
    if (xpos) {
      // Requires at least one sprite with xpos > 0.
      *m_spr_ovr = 1;
    } else if (*m_spr_ovr) {
      // Remaining sprites are not drawn.
      masked = 1;
    }

    // Display area offset.
    xpos = xpos - 0x80;

    // Sprite size.
    temp = object_info->size;

    // Sprite width.
    width = 8 + ((temp & 0x0C) << 1);

    // Update pixel count (off-screen sprites are included).
    pixelcount += width;

    // Is sprite across visible area ?
    if (((xpos + width) > 0) && (xpos < m_viewport->w) && !masked) {
      // Sprite attributes.
      attr = object_info->attr;

      // Sprite vertical offset.
      v_line = object_info->ypos;

      // Sprite priority + palette bits.
      atex = (attr >> 9) & 0x70;

      // Pattern name base.
      name = attr & 0x07FF;

      // Mask vflip/hflip.
      attr &= 0x1800;

      // Pointer into pattern name offset look-up table.
      s = &m_name_lut[((attr >> 3) & 0x300) | (temp << 4) | ((v_line & 0x18) >> 1)];

      // Pointer into line buffer.
      lb = &m_line_buffer[0x20 + xpos];

      // Adjust width for sprite limit.
      if (pixelcount > max_pixels) {
        width -= (pixelcount - max_pixels);
      }

      // Number of tiles to draw.
      width = width >> 3;

      // Pattern row index.
      v_line = (v_line & 7) << 3;

      // Draw sprite patterns.
      for (column = 0; column < width; column++, lb += 8) {
        temp = attr | ((name + s[column]) & 0x07FF);
        src = &m_pattern_cache[(temp << 6) | (v_line)];
        m_sprite_tile_drawer->DrawSpriteTile(8, atex, src, lb);
      }
    }

    // Sprite limit.
    if (pixelcount >= max_pixels) {
      // Sprite masking is effective on next line if max pixel width is reached.
      *m_spr_ovr = (pixelcount >= m_viewport->w);

      // Stop sprite rendering.
      return;
    }

    // Next sprite entry.
    object_info++;
  }

  // Clear sprite masking for next line.
  *m_spr_ovr = 0;
}

} // namespace gpgx::ppu::vdp

