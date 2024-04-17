/***************************************************************************************
 *  Genesis Plus
 *  Video Display Processor (pixel output rendering)
 *
 *  Support for all TMS99xx modes, Mode 4 & Mode 5 rendering
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

#include "core/vdp_render.h"

#include <math.h>

#include "xee/fnd/compiler.h"
#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "core/core_config.h"
#include "core/framebuffer.h"
#include "core/macros.h"
#include "core/system_hw.h"
#include "core/system_model.h"
#include "core/vdp_ctrl.h"
#include "core/viewport.h"
#include "core/vram.h"
#include "core/vdp/clip_t.h"
#include "core/vdp/object_info_t.h"
#include "core/vdp/pixel.h"

#include "gpgx/ppu/vdp/inv_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m0_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m1_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m1x_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m2_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m3_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m3x_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m4_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m4_bg_pattern_cache_updater.h"
#include "gpgx/ppu/vdp/m4_satb_parser.h"
#include "gpgx/ppu/vdp/m4_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m4_sprite_tile_drawer.h"
#include "gpgx/ppu/vdp/m4_zoomed_sprite_tile_drawer.h"
#include "gpgx/ppu/vdp/m5_bg_column_drawer.h"
#include "gpgx/ppu/vdp/m5_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_bg_pattern_cache_updater.h"
#include "gpgx/ppu/vdp/m5_im2_bg_column_drawer.h"
#include "gpgx/ppu/vdp/m5_im2_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_ste_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_vs_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_satb_parser.h"
#include "gpgx/ppu/vdp/m5_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_sprite_tile_drawer.h"
#include "gpgx/ppu/vdp/m5_ste_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_vs_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/tms_satb_parser.h"
#include "gpgx/ppu/vdp/tms_sprite_layer_renderer.h"

#ifndef HAVE_NO_SPRITE_LIMIT
#define MAX_SPRITES_PER_LINE 20
#define MODE5_MAX_SPRITE_PIXELS max_sprite_pixels
#endif

/* Pixel priority look-up tables information */
#define LUT_MAX     (6)
#define LUT_SIZE    (0x10000)

/// Clipping:
/// - clip[0] = Plane A clipping,
/// - clip[1] = Window clipping
static clip_t clip[2];

/* Pattern attribute (priority + palette bits) expansion table */
static const u32 atex_table[] =
{
  0x00000000,
  0x10101010,
  0x20202020,
  0x30303030,
  0x40404040,
  0x50505050,
  0x60606060,
  0x70707070
};

/* fixed Master System palette for Modes 0,1,2,3 */
static const u8 tms_crom[16] =
{
  0x00, 0x00, 0x08, 0x0C,
  0x10, 0x30, 0x01, 0x3C,
  0x02, 0x03, 0x05, 0x0F,
  0x04, 0x33, 0x15, 0x3F
};

/* original SG-1000 palette */
#if defined(USE_8BPP_RENDERING)
static const PIXEL_OUT_T tms_palette[16] =
{
  0x00, 0x00, 0x39, 0x79,
  0x4B, 0x6F, 0xC9, 0x5B,
  0xE9, 0xED, 0xD5, 0xD9,
  0x35, 0xCE, 0xDA, 0xFF
};

#elif defined(USE_15BPP_RENDERING)
static const PIXEL_OUT_T tms_palette[16] =
{
  0x8000, 0x8000, 0x9308, 0xAF6F,
  0xA95D, 0xBDDF, 0xE949, 0xA3BE,
  0xFD4A, 0xFDEF, 0xEB0A, 0xF330,
  0x92A7, 0xE177, 0xE739, 0xFFFF
};

#elif defined(USE_16BPP_RENDERING)
static const PIXEL_OUT_T tms_palette[16] =
{
  0x0000, 0x0000, 0x2648, 0x5ECF,
  0x52BD, 0x7BBE, 0xD289, 0x475E,
  0xF2AA, 0xFBCF, 0xD60A, 0xE670,
  0x2567, 0xC2F7, 0xCE59, 0xFFFF
};

#elif defined(USE_32BPP_RENDERING)
static const PIXEL_OUT_T tms_palette[16] =
{
  0xFF000000, 0xFF000000, 0xFF21C842, 0xFF5EDC78,
  0xFF5455ED, 0xFF7D76FC, 0xFFD4524D, 0xFF42EBF5,
  0xFFFC5554, 0xFFFF7978, 0xFFD4C154, 0xFFE6CE80,
  0xFF21B03B, 0xFFC95BB4, 0xFFCCCCCC, 0xFFFFFFFF
};
#endif

/* Cached and flipped patterns */
static u8 ALIGNED_(4) bg_pattern_cache[0x80000];

/* Sprite pattern name offset look-up table (Mode 5) */
static u8 name_lut[0x400];

/* Bitplane to packed pixel look-up table (Mode 4) */
static u32 bp_lut[0x10000];

/// Layer priority pixel look-up tables:
/// - lut[0] = bg,
/// - lut[1] = bgobj,
/// - lut[2] = bg_ste,
/// - lut[3] = obj,
/// - lut[4] = bgobj_ste,
/// - lut[5] = bgobj_m4
static u8 lut[LUT_MAX][LUT_SIZE];

/* Output pixel data look-up tables*/
static PIXEL_OUT_T pixel[0x100];
static PIXEL_OUT_T pixel_lut[3][0x200];
static PIXEL_OUT_T pixel_lut_m4[0x40];

/* Background & Sprite line buffers */
static u8 linebuf[2][0x200];

/* Sprite limit flag */
static u8 spr_ovr;

static object_info_t obj_info[2][MAX_SPRITES_PER_LINE];

/* Sprite Counter */
static u8 object_count[2];

/* Sprite Collision Info */
u16 spr_col;

static gpgx::ppu::vdp::M5BackgroundColumnDrawer* g_bg_column_drawer_m5 = nullptr;
static gpgx::ppu::vdp::M5Im2BackgroundColumnDrawer* g_bg_column_drawer_m5_im2 = nullptr;

gpgx::ppu::vdp::IBackgroundLayerRenderer* g_bg_layer_renderer = nullptr;
gpgx::ppu::vdp::InvalidBackgroundLayerRenderer* g_bg_layer_renderer_inv = nullptr;
gpgx::ppu::vdp::M0BackgroundLayerRenderer* g_bg_layer_renderer_m0 = nullptr;
gpgx::ppu::vdp::M1BackgroundLayerRenderer* g_bg_layer_renderer_m1 = nullptr;
gpgx::ppu::vdp::M1XBackgroundLayerRenderer* g_bg_layer_renderer_m1x = nullptr;
gpgx::ppu::vdp::M2BackgroundLayerRenderer* g_bg_layer_renderer_m2 = nullptr;
gpgx::ppu::vdp::M3BackgroundLayerRenderer* g_bg_layer_renderer_m3 = nullptr;
gpgx::ppu::vdp::M3XBackgroundLayerRenderer* g_bg_layer_renderer_m3x = nullptr;
gpgx::ppu::vdp::M4BackgroundLayerRenderer* g_bg_layer_renderer_m4 = nullptr;
gpgx::ppu::vdp::M5BackgroundLayerRenderer* g_bg_layer_renderer_m5 = nullptr;
gpgx::ppu::vdp::M5Im2BackgroundLayerRenderer* g_bg_layer_renderer_m5_im2 = nullptr;
gpgx::ppu::vdp::M5Im2VsBackgroundLayerRenderer* g_bg_layer_renderer_m5_im2_vs = nullptr;
gpgx::ppu::vdp::M5VsBackgroundLayerRenderer* g_bg_layer_renderer_m5_vs = nullptr;


/// Renderers of background layer.
/// Index = M1 M3 M4 M2
gpgx::ppu::vdp::IBackgroundLayerRenderer* g_bg_layer_renderer_modes[16] = { 0 };

gpgx::ppu::vdp::ISpriteLayerRenderer* g_sprite_layer_renderer = nullptr;
gpgx::ppu::vdp::TmsSpriteLayerRenderer* g_sprite_layer_renderer_tms = nullptr;
gpgx::ppu::vdp::M4SpriteLayerRenderer* g_sprite_layer_renderer_m4 = nullptr;
gpgx::ppu::vdp::M5SpriteLayerRenderer* g_sprite_layer_renderer_m5 = nullptr;
gpgx::ppu::vdp::M5SteSpriteLayerRenderer* g_sprite_layer_renderer_m5_ste = nullptr;
gpgx::ppu::vdp::M5Im2SpriteLayerRenderer* g_sprite_layer_renderer_m5_im2 = nullptr;
gpgx::ppu::vdp::M5Im2SteSpriteLayerRenderer* g_sprite_layer_renderer_m5_im2_ste = nullptr;

gpgx::ppu::vdp::ISpriteAttributeTableParser* g_satb_parser = nullptr;
gpgx::ppu::vdp::TmsSpriteAttributeTableParser* g_satb_parser_tms = nullptr;
gpgx::ppu::vdp::M4SpriteAttributeTableParser* g_satb_parser_m4 = nullptr;
gpgx::ppu::vdp::M5SpriteAttributeTableParser* g_satb_parser_m5 = nullptr;

gpgx::ppu::vdp::IBackgroundPatternCacheUpdater* g_bg_pattern_cache_updater = nullptr;
gpgx::ppu::vdp::M4BackgroundPatternCacheUpdater* g_bg_pattern_cache_updater_m4 = nullptr;
gpgx::ppu::vdp::M5BackgroundPatternCacheUpdater* g_bg_pattern_cache_updater_m5 = nullptr;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*--------------------------------------------------------------------------*/

/// Initialize background layer rendering.
static void background_layer_rendering_init()
{
  // Initialize renderer of background layer in invalid mode (1+3 or 1+2+3).
  if (!g_bg_layer_renderer_inv) {
    g_bg_layer_renderer_inv = new gpgx::ppu::vdp::InvalidBackgroundLayerRenderer(
      reg,
      linebuf[0]
    );
  }

  // Initialize renderer of background layer in mode 0 (Graphics I).
  if (!g_bg_layer_renderer_m0) {
    g_bg_layer_renderer_m0 = new gpgx::ppu::vdp::M0BackgroundLayerRenderer(
      reg,
      linebuf[0],
      vram
    );
  }

  // Initialize renderer of background layer in mode 1 (Text).
  if (!g_bg_layer_renderer_m1) {
    g_bg_layer_renderer_m1 = new gpgx::ppu::vdp::M1BackgroundLayerRenderer(
      reg,
      linebuf[0],
      vram
    );
  }

  // Initialize renderer of background layer in mode 1x (Text + Extended PG).
  if (!g_bg_layer_renderer_m1x) {
    g_bg_layer_renderer_m1x = new gpgx::ppu::vdp::M1XBackgroundLayerRenderer(
      reg,
      linebuf[0],
      vram,
      &system_hw
    );
  }

  // Initialize renderer of background layer in mode 2 (Graphics II).
  if (!g_bg_layer_renderer_m2) {
    g_bg_layer_renderer_m2 = new gpgx::ppu::vdp::M2BackgroundLayerRenderer(
      reg,
      linebuf[0],
      vram,
      &system_hw
    );
  }

  // Initialize renderer of background layer in mode 3 (Multicolor).
  if (!g_bg_layer_renderer_m3) {
    g_bg_layer_renderer_m3 = new gpgx::ppu::vdp::M3BackgroundLayerRenderer(
      reg,
      linebuf[0],
      vram
    );
  }

  // Initialize renderer of background layer in mode 3X (Multicolor + Extended PG).
  if (!g_bg_layer_renderer_m3x) {
    g_bg_layer_renderer_m3x = new gpgx::ppu::vdp::M3XBackgroundLayerRenderer(
      reg,
      linebuf[0],
      vram,
      &system_hw
    );
  }

  // Initialize renderer of background layer in mode 4.
  if (!g_bg_layer_renderer_m4) {
    g_bg_layer_renderer_m4 = new gpgx::ppu::vdp::M4BackgroundLayerRenderer(
      reg,
      &vscroll,
      bg_pattern_cache,
      linebuf[0],
      atex_table,
      vram,
      &system_hw,
      &viewport
    );
  }

  // Initialize column drawer in background layer rendering mode 5.
  if (!g_bg_column_drawer_m5) {
    g_bg_column_drawer_m5 = new gpgx::ppu::vdp::M5BackgroundColumnDrawer(
      atex_table,
      bg_pattern_cache
    );
  }

  // Initialize renderer of background layer in mode 5.
  if (!g_bg_layer_renderer_m5) {
    g_bg_layer_renderer_m5 = new gpgx::ppu::vdp::M5BackgroundLayerRenderer(
      reg,
      vram,
      vsram,

      &playfield_shift,
      &playfield_col_mask,
      &playfield_row_mask,

      &hscb,
      &hscroll_mask,

      &ntab,
      &ntbb,
      &ntwb,

      linebuf[1],
      linebuf[0],

      lut[0],
      lut[2],

      &clip[0],
      &clip[1],

      &viewport,
      g_bg_column_drawer_m5
    );
  }

  // Initialize renderer of background layer in mode 5 with 16 pixel column vertical scrolling.
  if (!g_bg_layer_renderer_m5_vs) {
    g_bg_layer_renderer_m5_vs = new gpgx::ppu::vdp::M5VsBackgroundLayerRenderer(
      reg,
      vram,
      vsram,

      &playfield_shift,
      &playfield_col_mask,
      &playfield_row_mask,

      &hscb,
      &hscroll_mask,

      &ntab,
      &ntbb,
      &ntwb,

      linebuf[1],
      linebuf[0],

      lut[0],
      lut[2],

      &clip[0],
      &clip[1],

      &viewport,
      g_bg_column_drawer_m5
    );
  }

  // Initialize column drawer in background layer rendering mode 5.
  if (!g_bg_column_drawer_m5_im2) {
    g_bg_column_drawer_m5_im2 = new gpgx::ppu::vdp::M5Im2BackgroundColumnDrawer(
      atex_table,
      bg_pattern_cache
    );
  }

  // Initialize renderer of background layer in mode 5 with interlace 
  // double resolution (IM2) enabled.
  if (!g_bg_layer_renderer_m5_im2) {
    g_bg_layer_renderer_m5_im2 = new gpgx::ppu::vdp::M5Im2BackgroundLayerRenderer(
      reg,
      vram,
      vsram,

      &odd_frame,

      &playfield_shift,
      &playfield_col_mask,
      &playfield_row_mask,

      &hscb,
      &hscroll_mask,

      &ntab,
      &ntbb,
      &ntwb,

      linebuf[1],
      linebuf[0],

      lut[0],
      lut[2],

      &clip[0],
      &clip[1],

      &viewport,
      g_bg_column_drawer_m5_im2
    );
  }

  // Initialize renderer of background layer in mode 5 with interlace 
  // double resolution (IM2) enabled and 16 pixel column vertical scrolling.
  if (!g_bg_layer_renderer_m5_im2_vs) {
    g_bg_layer_renderer_m5_im2_vs = new gpgx::ppu::vdp::M5Im2VsBackgroundLayerRenderer(
      reg,
      vram,
      vsram,

      &odd_frame,

      &playfield_shift,
      &playfield_col_mask,
      &playfield_row_mask,

      &hscb,
      &hscroll_mask,

      &ntab,
      &ntbb,
      &ntwb,

      linebuf[1],
      linebuf[0],

      lut[0],
      lut[2],

      &clip[0],
      &clip[1],

      &viewport,
      g_bg_column_drawer_m5_im2
    );
  }

  // 
  if (!g_bg_layer_renderer_modes[0]) {
    g_bg_layer_renderer_modes[0] = g_bg_layer_renderer_m0;
    g_bg_layer_renderer_modes[1] = g_bg_layer_renderer_m2;
    g_bg_layer_renderer_modes[2] = g_bg_layer_renderer_m4;
    g_bg_layer_renderer_modes[3] = g_bg_layer_renderer_m4;
    g_bg_layer_renderer_modes[4] = g_bg_layer_renderer_m3;
    g_bg_layer_renderer_modes[5] = g_bg_layer_renderer_m3x;
    g_bg_layer_renderer_modes[6] = g_bg_layer_renderer_m4;
    g_bg_layer_renderer_modes[7] = g_bg_layer_renderer_m4;
    g_bg_layer_renderer_modes[8] = g_bg_layer_renderer_m1;
    g_bg_layer_renderer_modes[9] = g_bg_layer_renderer_m1x;
    g_bg_layer_renderer_modes[10] = g_bg_layer_renderer_m4;
    g_bg_layer_renderer_modes[11] = g_bg_layer_renderer_m4;
    g_bg_layer_renderer_modes[12] = g_bg_layer_renderer_inv;
    g_bg_layer_renderer_modes[13] = g_bg_layer_renderer_inv;
    g_bg_layer_renderer_modes[14] = g_bg_layer_renderer_m4;
    g_bg_layer_renderer_modes[15] = g_bg_layer_renderer_m4;
  }
}

/// Initialize sprite layer rendering.
static void sprite_layer_rendering_init()
{
  // Initialize renderer of sprite layer in mode TMS.
  if (!g_sprite_layer_renderer_tms) {
    g_sprite_layer_renderer_tms = new gpgx::ppu::vdp::TmsSpriteLayerRenderer(
      obj_info,
      object_count,
      &spr_ovr,
      &status,
      reg,
      lut[5],
      linebuf[0],
      vram,
      &system_hw,
      &core_config,
      &v_counter,
      &viewport
    );
  }

  // Initialize renderer of sprite layer in mode 4.
  if (!g_sprite_layer_renderer_m4) {
    g_sprite_layer_renderer_m4 = new gpgx::ppu::vdp::M4SpriteLayerRenderer(
      obj_info,
      object_count,
      &status,
      reg,
      &spr_col,
      &spr_ovr,
      &v_counter,
      bg_pattern_cache,
      lut[5],
      linebuf[0],
      &system_hw,
      &core_config,
      &viewport
    );
  }

  // Initialize renderer of sprite layer in mode 5.
  if (!g_sprite_layer_renderer_m5) {
    g_sprite_layer_renderer_m5 = new gpgx::ppu::vdp::M5SpriteLayerRenderer(
      obj_info, 
      object_count, 
      &status, 
      &spr_ovr, 
      bg_pattern_cache, 
      linebuf[0], 
      lut[1], 
      name_lut, 
      &max_sprite_pixels, 
      &viewport
    );
  }

  // Initialize renderer of sprite layer in mode 5 (STE).
  if (!g_sprite_layer_renderer_m5_ste) {
    g_sprite_layer_renderer_m5_ste = new gpgx::ppu::vdp::M5SteSpriteLayerRenderer(
      obj_info,
      object_count,
      &status,
      &spr_ovr,
      bg_pattern_cache,
      linebuf[1],
      lut[3],
      linebuf[0],
      lut[4],
      name_lut,
      &max_sprite_pixels,
      &viewport
    );
  }

  // Initialize renderer of sprite layer in mode 5 (IM2).
  if (!g_sprite_layer_renderer_m5_im2) {
    g_sprite_layer_renderer_m5_im2 = new gpgx::ppu::vdp::M5Im2SpriteLayerRenderer(
      obj_info, 
      object_count, 
      &status, 
      &odd_frame, 
      &spr_ovr, 
      bg_pattern_cache, 
      linebuf[0], 
      lut[1], 
      name_lut, 
      &max_sprite_pixels, 
      &viewport
    );
  }

  // Initialize renderer of sprite layer in mode 5 (IM2/STE).
  if (!g_sprite_layer_renderer_m5_im2_ste) {
    g_sprite_layer_renderer_m5_im2_ste = new gpgx::ppu::vdp::M5Im2SteSpriteLayerRenderer(
      obj_info,
      object_count,
      &status,
      &odd_frame,
      &spr_ovr,
      bg_pattern_cache,
      linebuf[1],
      lut[3],
      linebuf[0],
      lut[4],
      name_lut,
      &max_sprite_pixels,
      &viewport
    );
  }
}

/// Initialize sprite attribute table parsing.
static void sprite_attribute_table_parsing_init()
{
  // Initialize parser of sprite attribute table (Mode TMS).
  if (!g_satb_parser_tms) {
    g_satb_parser_tms = new gpgx::ppu::vdp::TmsSpriteAttributeTableParser(
      &viewport,
      vram,
      obj_info,
      object_count,
      reg,
      &spr_ovr,
      &status
    );
  }

  // Initialize parser of sprite attribute table (Mode 4).
  if (!g_satb_parser_m4) {
    g_satb_parser_m4 = new gpgx::ppu::vdp::M4SpriteAttributeTableParser(
      &viewport,
      vram,
      obj_info,
      object_count,
      reg,
      &system_hw,
      &spr_ovr
    );
  }

  // Initialize parser of sprite attribute table (Mode 5).
  if (!g_satb_parser_m5) {
    g_satb_parser_m5 = new gpgx::ppu::vdp::M5SpriteAttributeTableParser(
      &viewport,
      vram,
      obj_info,
      object_count,
      sat,
      &satb,
      &im2_flag,
      &max_sprite_pixels,
      &status
    );
  }
}

/// Initialize background pattern cache updating.
static void background_pattern_cache_updating_init()
{
  // Initialize updater of background pattern cache (Mode 4).
  if (!g_bg_pattern_cache_updater_m4) {
    g_bg_pattern_cache_updater_m4 = new gpgx::ppu::vdp::M4BackgroundPatternCacheUpdater(
      bg_pattern_cache,
      bg_name_list,
      bg_name_dirty,
      vram,
      bp_lut
    );
  }

  // Initialize updater of background pattern cache (Mode 5).
  if (!g_bg_pattern_cache_updater_m5) {
    g_bg_pattern_cache_updater_m5 = new gpgx::ppu::vdp::M5BackgroundPatternCacheUpdater(
      bg_pattern_cache,
      bg_name_list,
      bg_name_dirty,
      vram
    );
  }
}

/*--------------------------------------------------------------------------*/
/* Sprite pattern name offset look-up table function (Mode 5)               */
/*--------------------------------------------------------------------------*/

static void make_name_lut(void)
{
  int vcol, vrow;
  int width, height;
  int flipx, flipy;
  int i;

  for (i = 0; i < 0x400; i += 1)
  {
    /* Sprite settings */
    vcol = i & 3;
    vrow = (i >> 2) & 3;
    height = (i >> 4) & 3;
    width  = (i >> 6) & 3;
    flipx  = (i >> 8) & 1;
    flipy  = (i >> 9) & 1;

    if ((vrow > height) || vcol > width)
    {
      /* Invalid settings (unused) */
      name_lut[i] = -1;
    }
    else
    {
      /* Adjust column & row index if sprite is flipped */
      if(flipx) vcol = (width - vcol);
      if(flipy) vrow = (height - vrow);

      /* Pattern offset (pattern order is up->down->left->right) */
      name_lut[i] = vrow + (vcol * (height + 1));
    }
  }
}


/*--------------------------------------------------------------------------*/
/* Bitplane to packed pixel look-up table function (Mode 4)                 */
/*--------------------------------------------------------------------------*/

static void make_bp_lut(void)
{
  int x,i,j;
  u32 out;

  /* ---------------------- */
  /* Pattern color encoding */
  /* -------------------------------------------------------------------------*/
  /* 4 byteplanes are required to define one pattern line (8 pixels)          */
  /* A single pixel color is coded with 4 bits (c3 c2 c1 c0)                  */
  /* Each bit is coming from byteplane bits, as explained below:              */
  /* pixel 0: c3 = bp3 bit 7, c2 = bp2 bit 7, c1 = bp1 bit 7, c0 = bp0 bit 7  */
  /* pixel 1: c3 = bp3 bit 6, c2 = bp2 bit 6, c1 = bp1 bit 6, c0 = bp0 bit 6  */
  /* ...                                                                      */
  /* pixel 7: c3 = bp3 bit 0, c2 = bp2 bit 0, c1 = bp1 bit 0, c0 = bp0 bit 0  */
  /* -------------------------------------------------------------------------*/

  for(i = 0; i < 0x100; i++)
  for(j = 0; j < 0x100; j++)
  {
    out = 0;
    for(x = 0; x < 8; x++)
    {
      /* pixel line data = hh00gg00ff00ee00dd00cc00bb00aa00 (32-bit) */
      /* aa-hh = upper or lower 2-bit values of pixels 0-7 (shifted) */
      out |= (j & (0x80 >> x)) ? (u32)(8 << (x << 2)) : 0;
      out |= (i & (0x80 >> x)) ? (u32)(4 << (x << 2)) : 0;
    }

    /* i = low byte in VRAM  (bp0 or bp2) */
    /* j = high byte in VRAM (bp1 or bp3) */
 #ifdef LSB_FIRST
    bp_lut[(j << 8) | (i)] = out;
 #else
    bp_lut[(i << 8) | (j)] = out;
 #endif
   }
}


/*--------------------------------------------------------------------------*/
/* Layers priority pixel look-up tables functions                           */
/*--------------------------------------------------------------------------*/

/* Input (bx):  d5-d0=color, d6=priority, d7=unused */
/* Input (ax):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=priority, d7=zero */
static u32 make_lut_bg(u32 bx, u32 ax)
{
  int bf = (bx & 0x7F);
  int bp = (bx & 0x40);
  int b  = (bx & 0x0F);

  int af = (ax & 0x7F);
  int ap = (ax & 0x40);
  int a  = (ax & 0x0F);

  int c = (ap ? (a ? af : bf) : (bp ? (b ? bf : af) : (a ? af : bf)));

  /* Strip palette & priority bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0x80;

  return (c);
}

/* Input (bx):  d5-d0=color, d6=priority, d7=unused */
/* Input (sx):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=priority, d7=intensity select (0=half/1=normal) */
static u32 make_lut_bg_ste(u32 bx, u32 ax)
{
  int bf = (bx & 0x7F);
  int bp = (bx & 0x40);
  int b  = (bx & 0x0F);

  int af = (ax & 0x7F);
  int ap = (ax & 0x40);
  int a  = (ax & 0x0F);

  int c = (ap ? (a ? af : bf) : (bp ? (b ? bf : af) : (a ? af : bf)));

  /* Half intensity when both pixels are low priority */
  c |= ((ap | bp) << 1);

  /* Strip palette & priority bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0x80;

  return (c);
}

/* Input (bx):  d5-d0=color, d6=priority/1, d7=sprite pixel marker */
/* Input (sx):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=priority, d7=sprite pixel marker */
static u32 make_lut_obj(u32 bx, u32 sx)
{
  int c;

  int bf = (bx & 0x7F);
  int bs = (bx & 0x80);
  int sf = (sx & 0x7F);

  if((sx & 0x0F) == 0) return bx;

  c = (bs ? bf : sf);

  /* Strip palette bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0xC0;

  return (c | 0x80);
}


/* Input (bx):  d5-d0=color, d6=priority, d7=opaque sprite pixel marker */
/* Input (sx):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=zero/priority, d7=opaque sprite pixel marker */
static u32 make_lut_bgobj(u32 bx, u32 sx)
{
  int c;

  int bf = (bx & 0x3F);
  int bs = (bx & 0x80);
  int bp = (bx & 0x40);
  int b  = (bx & 0x0F);

  int sf = (sx & 0x3F);
  int sp = (sx & 0x40);
  int s  = (sx & 0x0F);

  if(s == 0) return bx;

  /* Previous sprite has higher priority */
  if(bs) return bx;

  c = (sp ? sf : (bp ? (b ? bf : sf) : sf));

  /* Strip palette & priority bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0x80;

  return (c | 0x80);
}

/* Input (bx):  d5-d0=color, d6=priority, d7=intensity (half/normal) */
/* Input (sx):  d5-d0=color, d6=priority, d7=sprite marker */
/* Output:    d5-d0=color, d6=intensity (half/normal), d7=(double/invalid) */
static u32 make_lut_bgobj_ste(u32 bx, u32 sx)
{
  int c;

  int bf = (bx & 0x3F);
  int bp = (bx & 0x40);
  int b  = (bx & 0x0F);
  int bi = (bx & 0x80) >> 1;

  int sf = (sx & 0x3F);
  int sp = (sx & 0x40);
  int s  = (sx & 0x0F);
  int si = sp | bi;

  if(sp)
  {
    if(s)
    {
      if((sf & 0x3E) == 0x3E)
      {
        if(sf & 1)
        {
          c = (bf | 0x00);
        }
        else
        {
          c = (bx & 0x80) ? (bf | 0x80) : (bf | 0x40);
        }
      }
      else
      {
        if(sf == 0x0E || sf == 0x1E || sf == 0x2E)
        {
          c = (sf | 0x40);
        }
        else
        {
          c = (sf | si);
        }
      }
    }
    else
    {
      c = (bf | bi);
    }
  }
  else
  {
    if(bp)
    {
      if(b)
      {
        c = (bf | bi);
      }
      else
      {
        if(s)
        {
          if((sf & 0x3E) == 0x3E)
          {
            if(sf & 1)
            {
              c = (bf | 0x00);
            }
            else
            {
              c = (bx & 0x80) ? (bf | 0x80) : (bf | 0x40);
            }
          }
          else
          {
            if(sf == 0x0E || sf == 0x1E || sf == 0x2E)
            {
              c = (sf | 0x40);
            }
            else
            {
              c = (sf | si);
            }
          }
        }
        else
        {
          c = (bf | bi);
        }
      }
    }
    else
    {
      if(s)
      {
        if((sf & 0x3E) == 0x3E)
        {
          if(sf & 1)
          {
            c = (bf | 0x00);
          }
          else
          {
            c = (bx & 0x80) ? (bf | 0x80) : (bf | 0x40);
          }
        }
        else
        {
          if(sf == 0x0E || sf == 0x1E || sf == 0x2E)
          {
            c = (sf | 0x40);
          }
          else
          {
            c = (sf | si);
          }
        }
      }
      else
      {
        c = (bf | bi);
      }
    }
  }

  if((c & 0x0f) == 0x00) c &= 0xC0;

  return (c);
}

/* Input (bx):  d3-d0=color, d4=palette, d5=priority, d6=zero, d7=sprite pixel marker */
/* Input (sx):  d3-d0=color, d7-d4=zero */
/* Output:      d3-d0=color, d4=palette, d5=zero/priority, d6=zero, d7=sprite pixel marker */
static u32 make_lut_bgobj_m4(u32 bx, u32 sx)
{
  int c;

  int bf = (bx & 0x3F);
  int bs = (bx & 0x80);
  int bp = (bx & 0x20);
  int b  = (bx & 0x0F);

  int s  = (sx & 0x0F);
  int sf = (s | 0x10); /* force palette bit */

  /* Transparent sprite pixel */
  if(s == 0) return bx;

  /* Previous sprite has higher priority */
  if(bs) return bx;

  /* note: priority bit is always 0 for Modes 0,1,2,3 */
  c = (bp ? (b ? bf : sf) : sf);

  return (c | 0x80);
}


/*--------------------------------------------------------------------------*/
/* Pixel color lookup tables initialization                                 */
/*--------------------------------------------------------------------------*/

static void palette_init(void)
{
  int r, g, b, i;

  /************************************************/
  /* Each R,G,B color channel is 4-bit with a     */
  /* total of 15 different intensity levels.      */
  /*                                              */
  /* Color intensity depends on the mode:         */
  /*                                              */
  /*    normal   : xxx0     (0-14)                */
  /*    shadow   : 0xxx     (0-7)                 */
  /*    highlight: 1xxx - 1 (7-14)                */
  /*    mode4    : xxxx(*)  (0-15)                */
  /*    GG mode  : xxxx     (0-15)                */
  /*                                              */
  /* with x = original CRAM value (2, 3 or 4-bit) */
  /*  (*) 2-bit CRAM value is expanded to 4-bit   */
  /************************************************/

  /* Initialize Mode 5 pixel color look-up tables */
  for (i = 0; i < 0x200; i++)
  {
    /* CRAM 9-bit value (BBBGGGRRR) */
    r = (i >> 0) & 7;
    g = (i >> 3) & 7;
    b = (i >> 6) & 7;

    /* Convert to output pixel format */
    pixel_lut[0][i] = MAKE_PIXEL(r,g,b);
    pixel_lut[1][i] = MAKE_PIXEL(r<<1,g<<1,b<<1);
    pixel_lut[2][i] = MAKE_PIXEL(r+7,g+7,b+7);
  }

  /* Initialize Mode 4 pixel color look-up table */
  for (i = 0; i < 0x40; i++)
  {
    /* CRAM 6-bit value (000BBGGRR) */
    r = (i >> 0) & 3;
    g = (i >> 2) & 3;
    b = (i >> 4) & 3;

    /* Expand to full range & convert to output pixel format */
    pixel_lut_m4[i] = MAKE_PIXEL((r << 2) | r, (g << 2) | g, (b << 2) | b);
  }
}


/*--------------------------------------------------------------------------*/
/* Color palette update functions                                           */
/*--------------------------------------------------------------------------*/

void color_update_m4(int index, unsigned int data)
{
  switch (system_hw)
  {
    case SYSTEM_GG:
    {
      /* CRAM value (BBBBGGGGRRRR) */
      int r = (data >> 0) & 0x0F;
      int g = (data >> 4) & 0x0F;
      int b = (data >> 8) & 0x0F;

      /* Convert to output pixel */
      data = MAKE_PIXEL(r,g,b);
      break;
    }

    case SYSTEM_SG:
    case SYSTEM_SGII:
    case SYSTEM_SGII_RAM_EXT:
    {
      /* Fixed TMS99xx palette */
      if (index & 0x0F)
      {
        /* Colors 1-15 */
        data = tms_palette[index & 0x0F];
      }
      else
      {
        /* Backdrop color */
        data = tms_palette[reg[7] & 0x0F];
      }
      break;
    }

    default:
    {
      /* Test M4 bit */
      if (!(reg[0] & 0x04))
      {
        if (system_hw & SYSTEM_MD)
        {
          /* Invalid Mode (black screen) */
          data = 0x00;
        }
        else if (system_hw != SYSTEM_GGMS)
        {
          /* Fixed CRAM palette */
          if (index & 0x0F)
          {
            /* Colors 1-15 */
            data = tms_crom[index & 0x0F];
          }
          else
          {
            /* Backdrop color */
            data = tms_crom[reg[7] & 0x0F];
          }
        }
      }

      /* Mode 4 palette */
      data = pixel_lut_m4[data & 0x3F];
      break;
    }
  }


  /* Input pixel: x0xiiiii (normal) or 01000000 (backdrop) */
  if (reg[0] & 0x04)
  {
    /* Mode 4 */
    pixel[0x00 | index] = data;
    pixel[0x20 | index] = data;
    pixel[0x80 | index] = data;
    pixel[0xA0 | index] = data;
  }
  else
  {
    /* TMS99xx modes (palette bit forced to 1 because Game Gear uses CRAM palette #1) */
    if ((index == 0x40) || (index == (0x10 | (reg[7] & 0x0F))))
    {
      /* Update backdrop color */
      pixel[0x40] = data;

      /* Update transparent color */
      pixel[0x10] = data;
      pixel[0x30] = data;
      pixel[0x90] = data;
      pixel[0xB0] = data;
    }

    if (index & 0x0F)
    {
      /* update non-transparent colors */
      pixel[0x00 | index] = data;
      pixel[0x20 | index] = data;
      pixel[0x80 | index] = data;
      pixel[0xA0 | index] = data;
    }
  }
}

void color_update_m5(int index, unsigned int data)
{
  /* Palette Mode */
  if (!(reg[0] & 0x04))
  {
    /* Color value is limited to 00X00X00X */
    data &= 0x49;
  }

  if(reg[12] & 0x08)
  {
    /* Mode 5 (Shadow/Normal/Highlight) */
    pixel[0x00 | index] = pixel_lut[0][data];
    pixel[0x40 | index] = pixel_lut[1][data];
    pixel[0x80 | index] = pixel_lut[2][data];
  }
  else
  {
    /* Mode 5 (Normal) */
    data = pixel_lut[1][data];

    /* Input pixel: xxiiiiii */
    pixel[0x00 | index] = data;
    pixel[0x40 | index] = data;
    pixel[0x80 | index] = data;
  }
}


/*--------------------------------------------------------------------------*/
/* Window & Plane A clipping update function (Mode 5)                       */
/*--------------------------------------------------------------------------*/

void window_clip(unsigned int data, unsigned int sw)
{
  /* Window size and invert flags */
  int hp = (data & 0x1f);
  int hf = (data >> 7) & 1;

  /* Perform horizontal clipping; the results are applied in reverse
     if the horizontal inversion flag is set
   */
  int a = hf;
  int w = hf ^ 1;

  /* Display width (16 or 20 columns) */
  sw = 16 + (sw << 2);

  if(hp)
  {
    if(hp > sw)
    {
      /* Plane W takes up entire line */
      clip[w].left = 0;
      clip[w].right = sw;
      clip[w].enable = 1;
      clip[a].enable = 0;
    }
    else
    {
      /* Plane W takes left side, Plane A takes right side */
      clip[w].left = 0;
      clip[a].right = sw;
      clip[a].left = clip[w].right = hp;
      clip[0].enable = clip[1].enable = 1;
    }
  }
  else
  {
    /* Plane A takes up entire line */
    clip[a].left = 0;
    clip[a].right = sw;
    clip[a].enable = 1;
    clip[w].enable = 0;
  }
}


/*--------------------------------------------------------------------------*/
/* Init, reset routines                                                     */
/*--------------------------------------------------------------------------*/

void render_init(void)
{
  int bx, ax;

  /* Initialize layers priority pixel look-up tables */
  u16 index;
  for (bx = 0; bx < 0x100; bx++)
  {
    for (ax = 0; ax < 0x100; ax++)
    {
      index = (bx << 8) | (ax);

      lut[0][index] = make_lut_bg(bx, ax);
      lut[1][index] = make_lut_bgobj(bx, ax);
      lut[2][index] = make_lut_bg_ste(bx, ax);
      lut[3][index] = make_lut_obj(bx, ax);
      lut[4][index] = make_lut_bgobj_ste(bx, ax);
      lut[5][index] = make_lut_bgobj_m4(bx,ax);
    }
  }

  /* Initialize pixel color look-up tables */
  palette_init();

  /* Make sprite pattern name index look-up table (Mode 5) */
  make_name_lut();

  /* Make bitplane to pixel look-up table (Mode 4) */
  make_bp_lut();

  // Initialize sprite attribute table parsing.
  sprite_attribute_table_parsing_init();

  // Initialize background pattern cache updating.
  background_pattern_cache_updating_init();

  // Initialize sprite layer rendering.
  sprite_layer_rendering_init();

  // Initialize background layer rendering.
  background_layer_rendering_init();
}

void render_reset(void)
{
  /* Clear display bitmap */
  xee::mem::Memset(framebuffer.data, 0, framebuffer.pitch * framebuffer.height);

  /* Clear line buffers */
  xee::mem::Memset(linebuf, 0, sizeof(linebuf));

  /* Clear color palettes */
  xee::mem::Memset(pixel, 0, sizeof(pixel));

  /* Clear pattern cache */
  xee::mem::Memset ((char *) bg_pattern_cache, 0, sizeof (bg_pattern_cache));

  /* Reset Sprite infos */
  spr_ovr = spr_col = object_count[0] = object_count[1] = 0;
}


/*--------------------------------------------------------------------------*/
/* Line rendering functions                                                 */
/*--------------------------------------------------------------------------*/

void render_line(int line)
{
  /* Check display status */
  if (reg[1] & 0x40)
  {
    /* Update pattern cache */
    if (bg_list_index)
    {
      g_bg_pattern_cache_updater->UpdateBackgroundPatternCache(bg_list_index);
      bg_list_index = 0;
    }

    /* Render BG layer(s) */
    g_bg_layer_renderer->RenderBackground(line);

    /* Render sprite layer */
    g_sprite_layer_renderer->RenderSprites(line & 1);

    /* Left-most column blanking */
    if (reg[0] & 0x20)
    {
      if (system_hw >= SYSTEM_MARKIII)
      {
        xee::mem::Memset(&linebuf[0][0x20], 0x40, 8);
      }
    }

    /* Parse sprites for next line */
    if (line < (viewport.h - 1))
    {
      g_satb_parser->ParseSpriteAttributeTable(line);
    }

    /* Horizontal borders */
    if (viewport.x > 0)
    {
      xee::mem::Memset(&linebuf[0][0x20 - viewport.x], 0x40, viewport.x);
      xee::mem::Memset(&linebuf[0][0x20 + viewport.w], 0x40, viewport.x);
    }
  }
  else
  {
    /* Master System & Game Gear VDP specific */
    if (system_hw < SYSTEM_MD)
    {
      /* Update SOVR flag */
      status |= spr_ovr;
      spr_ovr = 0;

      /* Sprites are still parsed when display is disabled */
      g_satb_parser->ParseSpriteAttributeTable(line);
    }

    /* Blanked line */
    xee::mem::Memset(&linebuf[0][0x20 - viewport.x], 0x40, viewport.w + 2*viewport.x);
  }

  /* Pixel color remapping */
  remap_line(line);
}

void blank_line(int line, int offset, int width)
{
  xee::mem::Memset(&linebuf[0][0x20 + offset], 0x40, width);
  remap_line(line);
}

void remap_line(int line)
{
  /* Line width */
  int width = viewport.w + 2*viewport.x;

  /* Pixel line buffer */
  u8 *src = &linebuf[0][0x20 - viewport.x];

  /* Adjust line offset in framebuffer */
  line = (line + viewport.y) % lines_per_frame;

  /* Take care of Game Gear reduced screen when overscan is disabled */
  if (line < 0) return;

  /* Convert VDP pixel data to output pixel format */
  PIXEL_OUT_T *dst = ((PIXEL_OUT_T *)&framebuffer.data[(line * framebuffer.pitch)]);

  do
  {
    *dst++ = pixel[*src++];
  }
  while (--width);
}
