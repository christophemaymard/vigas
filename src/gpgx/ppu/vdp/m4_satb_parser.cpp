/***************************************************************************************
 *  Genesis Plus GX
 *  Video Display Processor (sprite attribute table parsing)
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

#include "gpgx/ppu/vdp/m4_satb_parser.h"

#include "core/vdp/object_info_t.h"
#include "core/system_model.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// M4SpriteAttributeTableParser

//------------------------------------------------------------------------------

M4SpriteAttributeTableParser::M4SpriteAttributeTableParser(
  viewport_t* viewport,
  u8* vram,
  object_info_t(&obj_info)[2][20],
  u8* object_count,
  u8* reg,
  u8* system_hw,
  u8* spr_ovr) :
  m_viewport(viewport),
  m_vram(vram),
  m_obj_info(obj_info),
  m_object_count(object_count),
  m_reg(reg), 
  m_system_hw(system_hw),
  m_spr_ovr(spr_ovr)
{
}

//------------------------------------------------------------------------------

s32 M4SpriteAttributeTableParser::GetMaxSpritesPerLine() const
{
  return 8;
}

//------------------------------------------------------------------------------

void M4SpriteAttributeTableParser::ParseSpriteAttributeTable(s32 line)
{
  int i = 0;
  u8* st;

  // Sprite counter (8 max. per line).
  int count = 0;

  // Y position.
  int ypos;

  // Sprite list for next line.
  object_info_t* object_info = m_obj_info[(line + 1) & 1];

  // Sprite height (8x8 or 8x16).
  int height = 8 + ((m_reg[1] & 0x02) << 2);

  // Sprite attribute table address mask.
  u16 st_mask = ~0x3F80 ^ (m_reg[5] << 7);

  // Unused bits used as a mask on 315-5124 VDP only.
  if (*m_system_hw > SYSTEM_SMS) {
    st_mask |= 0x80;
  }

  // Pointer to sprite attribute table.
  st = &m_vram[st_mask & 0x3F00];

  // Parse Sprite Table (64 entries).
  do {
    // Sprite Y position.
    ypos = st[i];

    // Check end of sprite list marker (no effect in extended modes).
    if ((ypos == 208) && (m_viewport->h == 192)) {
      break;
    }

    // Wrap Y coordinate.
    // (NB: this is likely not 100% accurate and needs to be verified on real hardware)
    if (ypos > (m_viewport->h + 16)) {
      ypos -= 256;
    }

    // Y range.
    ypos = line - ypos;

    // Adjust Y range for zoomed sprites (not working on Mega Drive VDP).
    if (*m_system_hw < SYSTEM_MD) {
      ypos >>= (m_reg[1] & 0x01);
    }

    // Check if sprite is visible on this line.
    if ((ypos >= 0) && (ypos < height)) {
      // Sprite overflow.
      if (count == GetMaxSpritesPerLine()) {
        // Flag is set only during active area.
        if ((line >= 0) && (line < m_viewport->h)) {
          *m_spr_ovr = 0x40;
        }

        break;
      }

      // Store sprite attributes for later processing.
      object_info->ypos = ypos;
      object_info->xpos = st[(0x80 + (i << 1)) & st_mask];
      object_info->attr = st[(0x81 + (i << 1)) & st_mask];

      // Increment Sprite count.
      ++count;

      // Next sprite entry.
      object_info++;
    }
  } while (++i < 64);

  // Update sprite count for next line.
  m_object_count[(line + 1) & 1] = count;
}

} // namespace gpgx::ppu::vdp

