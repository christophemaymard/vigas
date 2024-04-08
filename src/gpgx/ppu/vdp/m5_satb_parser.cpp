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

#include "gpgx/ppu/vdp/m5_satb_parser.h"

#include "core/vdp/object_info_t.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// M5SpriteAttributeTableParser

//------------------------------------------------------------------------------

M5SpriteAttributeTableParser::M5SpriteAttributeTableParser(
  viewport_t* viewport,
  u8* vram,
  object_info_t(&obj_info)[2][20],
  u8* object_count,
  u8* sat,
  u16* satb,
  u8* im2_flag,
  u16* max_sprite_pixels,
  u16* status) : 
  m_viewport(viewport),
  m_vram(vram),
  m_obj_info(obj_info),
  m_object_count(object_count),
  m_sat(sat),
  m_satb(satb),
  m_im2_flag(im2_flag),
  m_max_sprite_pixels(max_sprite_pixels),
  m_status(status)
{
}

//------------------------------------------------------------------------------

s32 M5SpriteAttributeTableParser::GetMaxSpritesPerLine() const
{
  return m_viewport->w >> 4;
}

//------------------------------------------------------------------------------

void M5SpriteAttributeTableParser::ParseSpriteAttributeTable(s32 line)
{
  // Y position.
  int ypos;

  // Sprite height (8,16,24,32 pixels).
  int height;

  // Sprite size data.
  int size;

  // Sprite link data.
  int link = 0;

  // Sprite counter.
  int count = 0;

  // max. number of rendered sprites (16 or 20 sprites per line by default).
  int max = GetMaxSpritesPerLine();

  // max. number of parsed sprites (64 or 80 sprites per line by default).
  int total = *m_max_sprite_pixels >> 2;

  // Pointer to sprite attribute table.
  u16* p = (u16*)&m_vram[*m_satb];

  // Pointer to internal RAM.
  u16* q = (u16*)&m_sat[0];

  // Sprite list for next line.
  object_info_t* object_info = m_obj_info[(line + 1) & 1];

  // Adjust line offset.
  line += 0x81;

  do {
    // Read Y position from internal SAT cache.
    ypos = (q[link] >> *m_im2_flag) & 0x1FF;

    // Check if sprite Y position has been reached.
    if (line >= ypos) {
      // Read sprite size from internal SAT cache.
      size = q[link + 1] >> 8;

      // Sprite height.
      height = 8 + ((size & 3) << 3);

      // Y range.
      ypos = line - ypos;

      // Check if sprite is visible on current line.
      if (ypos < height) {
        // Sprite overflow.
        if (count == max) {
          *m_status |= 0x40;

          break;
        }

        // Update sprite list (only name, attribute & xpos are parsed from VRAM).
        object_info->attr = p[link + 2];
        object_info->xpos = p[link + 3] & 0x1ff;
        object_info->ypos = ypos;
        object_info->size = size & 0x0f;

        // Increment Sprite count.
        ++count;

        // Next sprite entry.
        object_info++;
      }
    }

    // Read link data from internal SAT cache.
    link = (q[link + 1] & 0x7F) << 2;

    // Stop parsing if link data points to first entry (#0) or after the last entry (#64 in H32 mode, #80 in H40 mode).
    if ((link == 0) || (link >= m_viewport->w)) {
      break;
    }
  } while (--total);

  // Update sprite count for next line (line value already incremented).
  m_object_count[line & 1] = count;
}

} // namespace gpgx::ppu::vdp

