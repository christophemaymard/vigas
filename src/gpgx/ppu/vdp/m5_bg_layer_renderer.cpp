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

#include "gpgx/ppu/vdp/m5_bg_layer_renderer.h"

namespace gpgx::ppu::vdp {

//==============================================================================
// M5BackgroundLayerRenderer

//------------------------------------------------------------------------------

M5BackgroundLayerRenderer::M5BackgroundLayerRenderer(
  u8* reg,
  u8* vram,
  u8* vsram,

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
  M5BackgroundColumnDrawer* bg_column_drawer) : 
  m_reg(reg),
  m_vram(vram),
  m_vsram(vsram),

  m_playfield_shift(playfield_shift),
  m_playfield_col_mask(playfield_col_mask),
  m_playfield_row_mask(playfield_row_mask),

  m_hscb(hscb),
  m_hscroll_mask(hscroll_mask),

  m_ntab(ntab),
  m_ntbb(ntbb),
  m_ntwb(ntwb),

  m_a_line_buffer(a_line_buffer),
  m_b_line_buffer(b_line_buffer),

  m_bg_lut(bg_lut),
  m_bg_ste_lut(bg_ste_lut),

  m_a_clip(a_clip),
  m_w_clip(w_clip),

  m_viewport(viewport),
  m_bg_column_drawer(bg_column_drawer)
{
}

//------------------------------------------------------------------------------

void M5BackgroundLayerRenderer::RenderBackground(s32 line)
{
  // Common data.
  u32 xscroll = *(u32*)&m_vram[*m_hscb + ((line & *m_hscroll_mask) << 2)];
  u32 yscroll = *(u32*)&m_vsram[0];
  u32 pf_col_mask = *m_playfield_col_mask;
  u32 pf_row_mask = *m_playfield_row_mask;
  u32 pf_shift = *m_playfield_shift;

  // Window & Plane A.
  s32 a = (m_reg[18] & 0x1F) << 3;
  s32 w = (m_reg[18] >> 7) & 1;

  // Plane B width.
  s32 start = 0;
  s32 end = m_viewport->w >> 4;

  // Plane B scroll.
#ifdef LSB_FIRST
  u32 shift = (xscroll >> 16) & 0x0F;
  u32 index = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
  u32 v_line = (line + (yscroll >> 16)) & pf_row_mask;
#else
  u32 shift = (xscroll & 0x0F);
  u32 index = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
  u32 v_line = (line + yscroll) & pf_row_mask;
#endif

  // Plane B name table.
  u32* nt = (u32*)&m_vram[*m_ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

  // Pattern row index.
  v_line = (v_line & 7) << 3;

  s32 column = 0;
  u32 atbuf = 0;
  u32* dst = nullptr;

  if (shift) {
    // Plane B line buffer.
    dst = (u32*)&m_b_line_buffer[0x10 + shift];

    atbuf = nt[(index - 1) & pf_col_mask];
    m_bg_column_drawer->DrawColumn(&dst, atbuf, v_line);
  } else {
    // Plane B line buffer.
    dst = (u32*)&m_b_line_buffer[0x20];
  }

  for (column = 0; column < end; column++, index++) {
    atbuf = nt[index & pf_col_mask];
    m_bg_column_drawer->DrawColumn(&dst, atbuf, v_line);
  }

  if (w == (line >= a)) {
    // Window takes up entire line.
    a = 0;
    w = 1;
  } else {
    // Window and Plane A share the line.
    a = m_a_clip->enable;
    w = m_w_clip->enable;
  }

  // Plane A.
  if (a) {
    // Plane A width.
    start = m_a_clip->left;
    end = m_a_clip->right;

    // Plane A scroll.
#ifdef LSB_FIRST
    shift = (xscroll & 0x0F);
    index = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
    v_line = (line + yscroll) & pf_row_mask;
#else
    shift = (xscroll >> 16) & 0x0F;
    index = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
    v_line = (line + (yscroll >> 16)) & pf_row_mask;
#endif

    // Plane A name table.
    nt = (u32*)&m_vram[*m_ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    // Pattern row index.
    v_line = (v_line & 7) << 3;

    if (shift) {
      // Plane A line buffer.
      dst = (u32*)&m_a_line_buffer[0x10 + shift + (start << 4)];

      // Window bug.
      if (start) {
        atbuf = nt[index & pf_col_mask];
      } else {
        atbuf = nt[(index - 1) & pf_col_mask];
      }

      m_bg_column_drawer->DrawColumn(&dst, atbuf, v_line);
    } else {
      // Plane A line buffer.
      dst = (u32*)&m_a_line_buffer[0x20 + (start << 4)];
    }

    for (column = start; column < end; column++, index++) {
      atbuf = nt[index & pf_col_mask];
      m_bg_column_drawer->DrawColumn(&dst, atbuf, v_line);
    }

    // Window width.
    start = m_w_clip->left;
    end = m_w_clip->right;
  }

  // Window.
  if (w) {
    // Window name table.
    nt = (u32*)&m_vram[*m_ntwb | ((line >> 3) << (6 + (m_reg[12] & 1)))];

    // Pattern row index.
    v_line = (line & 7) << 3;

    // Plane A line buffer.
    dst = (u32*)&m_a_line_buffer[0x20 + (start << 4)];

    for (column = start; column < end; column++) {
      atbuf = nt[column];
      m_bg_column_drawer->DrawColumn(&dst, atbuf, v_line);
    }
  }

  // Merge background layers.
  Merge(
    &m_a_line_buffer[0x20],
    &m_b_line_buffer[0x20], 
    &m_b_line_buffer[0x20], 
    (m_reg[12] & 0x08) ? m_bg_ste_lut : m_bg_lut,
    m_viewport->w
  );
}

//------------------------------------------------------------------------------

void M5BackgroundLayerRenderer::Merge(u8* srca, u8* srcb, u8* dst, u8* table, s32 width)
{
  do {
    *dst++ = table[(*srcb++ << 8) | (*srca++)];
  } while (--width);
}

} // namespace gpgx::ppu::vdp

