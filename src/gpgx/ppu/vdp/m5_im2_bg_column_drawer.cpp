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

#include "gpgx/ppu/vdp/m5_im2_bg_column_drawer.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// M5Im2BackgroundColumnDrawer

//------------------------------------------------------------------------------

M5Im2BackgroundColumnDrawer::M5Im2BackgroundColumnDrawer(
  const u32* atex_table,
  u8* pattern_cache) :
  m_atex_table(atex_table),
  m_pattern_cache(pattern_cache)
{
}

//------------------------------------------------------------------------------

void M5Im2BackgroundColumnDrawer::DrawColumn(u32** dest, u32 attr, u32 line)
{
#ifdef LSB_FIRST
  DrawLSBTile(dest, attr, line);
  DrawMSBTile(dest, attr, line);
#else
  DrawMSBTile(dest, attr, line);
  DrawLSBTile(dest, attr, line);
#endif // #ifdef LSB_FIRST
}

//------------------------------------------------------------------------------

void M5Im2BackgroundColumnDrawer::DrawLSBTile(u32** dest, u32 attr, u32 line)
{
  u32 atex = m_atex_table[(attr >> 13) & 7];;
  u32* src = (u32*)&m_pattern_cache[((attr & 0x000003FF) << 7 | (attr & 0x00001800) << 6 | line) ^ ((attr & 0x00001000) >> 6)];

  **dest = (src[0] | atex);
  (*dest)++;
  **dest = (src[1] | atex);
  (*dest)++;
}

//------------------------------------------------------------------------------

void M5Im2BackgroundColumnDrawer::DrawMSBTile(u32** dest, u32 attr, u32 line)
{
  u32 atex = m_atex_table[(attr >> 29) & 7];
  u32* src = (u32*)&m_pattern_cache[((attr & 0x03FF0000) >> 9 | (attr & 0x18000000) >> 10 | line) ^ ((attr & 0x10000000) >> 22)];

  **dest = (src[0] | atex);
  (*dest)++;
  **dest = (src[1] | atex);
  (*dest)++;
}

} // namespace gpgx::ppu::vdp

