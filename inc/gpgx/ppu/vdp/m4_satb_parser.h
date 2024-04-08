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

#ifndef __GPGX_PPU_VDP_M4_SATB_PARSER_H__
#define __GPGX_PPU_VDP_M4_SATB_PARSER_H__

#include "xee/fnd/data_type.h"

#include "core/vdp/object_info_t.h"
#include "core/viewport_t.h"

#include "gpgx/ppu/vdp/satb_parser.h"

namespace gpgx::ppu::vdp {

//==============================================================================

//------------------------------------------------------------------------------

/// Parser of sprite attribute table in mode 4.
class M4SpriteAttributeTableParser : public ISpriteAttributeTableParser
{
public:
  M4SpriteAttributeTableParser(
    viewport_t* viewport,
    u8* vram,
    object_info_t(&obj_info)[2][20],
    u8* object_count,
    u8* reg,
    u8* system_hw,
    u8* spr_ovr
  );

  // Implementation of ISpriteAttributeTableParser.

  s32 GetMaxSpritesPerLine() const;
  void ParseSpriteAttributeTable(s32 line);

private:
  viewport_t* m_viewport; /// 
  u8* m_vram; /// Video RAM (64K x 8-bit).
  object_info_t (&m_obj_info)[2][20]; /// 
  u8* m_object_count; /// Sprite counter.

  u8* m_reg; /// Internal VDP registers (23 x 8-bit).
  u8* m_system_hw;
  u8* m_spr_ovr; /// Sprite limit flag.
};

} // namespace gpgx::ppu::vdp

#endif // #ifndef __GPGX_PPU_VDP_M4_SATB_PARSER_H__

