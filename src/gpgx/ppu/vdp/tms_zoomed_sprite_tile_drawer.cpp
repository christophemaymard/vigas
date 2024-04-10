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

#include "gpgx/ppu/vdp/tms_zoomed_sprite_tile_drawer.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// TmsZoomedSpriteTileDrawer

//------------------------------------------------------------------------------

TmsZoomedSpriteTileDrawer::TmsZoomedSpriteTileDrawer(u16* status, u8* lut) : 
  m_status(status),
  m_lut(lut)
{
}

//------------------------------------------------------------------------------

void TmsZoomedSpriteTileDrawer::DrawSpriteTile(s32 start, s32 width, u8* src, u8* line_buffer, u8 color)
{
  u16 temp = 0;

  // Zoomed sprites are rendered at half speed.
  for (s32 x = start; x < width; x += 2) {
    temp = src[(x >> 4) & 1];
    temp = (temp >> (7 - ((x >> 1) & 7))) & 0x01;
    temp = temp * color;
    temp |= (line_buffer[x] << 8);
    line_buffer[x] = m_lut[temp];
    *m_status |= ((temp & 0x8000) >> 10);

    temp &= 0x00FF;
    temp |= (line_buffer[x + 1] << 8);
    line_buffer[x + 1] = m_lut[temp];
    *m_status |= ((temp & 0x8000) >> 10);
  }
}

} // namespace gpgx::ppu::vdp

