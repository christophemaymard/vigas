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

#ifndef __GPGX_PPU_VDP_M5_IM2_SPRITE_LAYER_RENDERER_H__
#define __GPGX_PPU_VDP_M5_IM2_SPRITE_LAYER_RENDERER_H__

#include "xee/fnd/data_type.h"

#include "core/vdp/object_info_t.h"
#include "core/viewport_t.h"

#include "gpgx/ppu/vdp/m5_sprite_tile_drawer.h"
#include "gpgx/ppu/vdp/sprite_layer_renderer.h"

namespace gpgx::ppu::vdp {

//==============================================================================

//------------------------------------------------------------------------------

/// Renderer of sprite layer in mode 5 with interlace double resoultion (IM2) 
/// enabled.
class M5Im2SpriteLayerRenderer : public ISpriteLayerRenderer
{
public:
  M5Im2SpriteLayerRenderer(
    object_info_t (&obj_info)[2][20],
    u8* object_count, 
    u16* status, 
    u8* odd_frame, 
    u8* spr_ovr, 
    u8* pattern_cache, 
    u8* line_buffer, 
    u8* lut, 
    u8* name_lut, 
    u16* max_sprite_pixels, 
    viewport_t* viewport
  );

  // Implementation of ISpriteLayerRenderer.

  void RenderSprites(s32 line);

private:
  object_info_t (&m_obj_info)[2][20];
  u8* m_object_count; /// Sprite counter.

  u8* m_odd_frame; /// 0 = even field, 1 = odd field.
  u8* m_spr_ovr; /// Sprite limit flag.

  u8* m_pattern_cache; /// Cached and flipped patterns.
  u8* m_line_buffer; /// Line buffer.

  u8* m_name_lut; /// Sprite pattern name offset look-up table (Mode 5).
  u16* m_max_sprite_pixels; /// Max. sprites pixels per line (parsing & rendering).

  viewport_t* m_viewport;

  M5SpriteTileDrawer* m_sprite_tile_drawer;
};

} // namespace gpgx::ppu::vdp

#endif // #ifndef __GPGX_PPU_VDP_M5_IM2_SPRITE_LAYER_RENDERER_H__

