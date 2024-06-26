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

#ifndef __GPGX_PPU_VDP_M4_SPRITE_TILE_DRAWER_H__
#define __GPGX_PPU_VDP_M4_SPRITE_TILE_DRAWER_H__

#include "xee/fnd/data_type.h"

namespace gpgx::ppu::vdp {

//==============================================================================

//------------------------------------------------------------------------------

/// Drawer of normal sprite tile in mode 4.
class M4SpriteTileDrawer
{
public:
  M4SpriteTileDrawer(u16* status, u16* v_counter, u16* spr_col, u8* lut);
  
  void DrawSpriteTile(s32 width, u8* src, u8* line_buffer, s32 x_pos);

private:
  u16* m_status; /// VDP status flags.
  u16* m_v_counter; /// Vertical counter.
  u16* m_spr_col; /// Sprite collision info.
  u8* m_lut; /// Layer priority pixel look-up table.
};

} // namespace gpgx::ppu::vdp

#endif // #ifndef __GPGX_PPU_VDP_M4_SPRITE_TILE_DRAWER_H__

