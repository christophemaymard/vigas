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

#include "gpgx/ppu/vdp/tms_satb_parser.h"

#include "core/vdp/object_info_t.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// TmsSpriteAttributeTableParser

//------------------------------------------------------------------------------

TmsSpriteAttributeTableParser::TmsSpriteAttributeTableParser(
  viewport_t* viewport,
  u8* vram,
  object_info_t(&obj_info)[2][20],
  u8* object_count,
  u8* reg,
  u8* spr_ovr,
  u16* status) : 
  m_viewport(viewport),
  m_vram(vram),
  m_obj_info(obj_info),
  m_object_count(object_count),
  m_reg(reg),
  m_spr_ovr(spr_ovr),
  m_status(status)
{
}

//------------------------------------------------------------------------------

s32 TmsSpriteAttributeTableParser::GetMaxSpritesPerLine() const
{
  return 4;
}

//------------------------------------------------------------------------------

void TmsSpriteAttributeTableParser::ParseSpriteAttributeTable(s32 line)
{
  int i = 0;

  // Sprite counter (4 max. per line).
  int count = 0;

  // no sprites in Text modes.
  if (!(m_reg[1] & 0x10)) {
    // Y position.
    int ypos;

    // Sprite list for next line.
    object_info_t* object_info = m_obj_info[(line + 1) & 1];

    // Pointer to sprite attribute table.
    u8* st = &m_vram[(m_reg[5] << 7) & 0x3F80];

    // Sprite height (8 pixels by default).
    int height = 8;

    // Adjust height for 16x16 sprites.
    height <<= ((m_reg[1] & 0x02) >> 1);

    // Adjust height for zoomed sprites.
    height <<= (m_reg[1] & 0x01);

    // Parse Sprite Table (32 entries).
    do {
      // Sprite Y position.
      ypos = st[i << 2];

      // Check end of sprite list marker.
      if (ypos == 0xD0) {
        break;
      }

      // Wrap Y coordinate for sprites > 256-32.
      if (ypos >= 224) {
        ypos -= 256;
      }

      // Y range.
      ypos = line - ypos;

      // Sprite is visible on this line ?
      if ((ypos >= 0) && (ypos < height)) {
        // Sprite overflow.
        if (count == GetMaxSpritesPerLine()) {
          // Flag is set only during active area.
          if (line < m_viewport->h) {
            *m_spr_ovr = 0x40;
          }

          break;
        }

        // Adjust Y range back for zoomed sprites.
        ypos >>= (m_reg[1] & 0x01);

        // Store sprite attributes for later processing.
        object_info->ypos = ypos;
        object_info->xpos = st[(i << 2) + 1];
        object_info->attr = st[(i << 2) + 2];
        object_info->size = st[(i << 2) + 3];

        // Increment Sprite count.
        ++count;

        // Next sprite entry.
        object_info++;
      }
    } while (++i < 32);
  }

  // Update sprite count for next line.
  m_object_count[(line + 1) & 1] = count;

  // Insert number of last sprite entry processed.
  *m_status = (*m_status & 0xE0) | (i & 0x1F);
}

} // namespace gpgx::ppu::vdp

