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

#ifndef __GPGX_PPU_VDP_M5_IM2_VS_BG_LAYER_RENDERER_H__
#define __GPGX_PPU_VDP_M5_IM2_VS_BG_LAYER_RENDERER_H__

#include "xee/fnd/data_type.h"

#include "core/vdp/clip_t.h"
#include "core/viewport_t.h"

#include "gpgx/ppu/vdp/bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_bg_column_drawer.h"

namespace gpgx::ppu::vdp {

//==============================================================================

//------------------------------------------------------------------------------

/// Renderer of background layer in mode 5 with interlace double resolution 
/// (IM2) enabled and 16 pixel column vertical scrolling.
class M5Im2VsBackgroundLayerRenderer : public IBackgroundLayerRenderer
{
public:
  M5Im2VsBackgroundLayerRenderer(
    u8* reg,
    u8* vram,
    u8* vsram,

    u8* odd_frame,

    u8* playfield_shift,
    u8* playfield_col_mask,
    u16* playfield_row_mask,

    u16* hscb,
    u8* hscroll_mask,

    u16* ntab,
    u16* ntbb,
    u16* ntwb,

    u8* a_line_buffer,
    u8* b_line_buffer,

    u8* bg_lut,
    u8* bg_ste_lut,

    clip_t* a_clip,
    clip_t* w_clip,

    viewport_t* viewport,
    M5Im2BackgroundColumnDrawer* bg_column_drawer
  );

  // Implementation of IBackgroundLayerRenderer.

  void RenderBackground(s32 line);

private:
  void Merge(u8* srca, u8* srcb, u8* dst, u8* table, s32 width);

private:
  u8* m_reg; /// Internal VDP registers (23 x 8-bit).
  u8* m_vram; /// Video RAM (64K x 8-bit).
  u8* m_vsram; /// On-chip vertical scroll RAM (40 x 11-bit).

  u8* m_odd_frame; /// 0 = even field, 1 = odd field.

  u8* m_playfield_shift; /// Width of planes A, B (in bits).
  u8* m_playfield_col_mask; /// Playfield column mask.
  u16* m_playfield_row_mask; /// Playfield row mask.

  u16* m_hscb; /// Horizontal scroll table base address.
  u8* m_hscroll_mask; /// Horizontal Scrolling line mask.

  u16* m_ntab; /// Name table A base address.
  u16* m_ntbb; /// Name table B base address.
  u16* m_ntwb; /// Name table W base address.

  u8* m_a_line_buffer; /// Plane A line buffer.
  u8* m_b_line_buffer; /// Plane B line buffer.

  u8* m_bg_lut;
  u8* m_bg_ste_lut;

  clip_t* m_a_clip; /// Plane A clip.
  clip_t* m_w_clip; /// Window clip.

  viewport_t* m_viewport;
  M5Im2BackgroundColumnDrawer* m_bg_column_drawer;
};

} // namespace gpgx::ppu::vdp

#endif // #ifndef __GPGX_PPU_VDP_M5_IM2_VS_BG_LAYER_RENDERER_H__

