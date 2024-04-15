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

#ifndef _RENDER_H_
#define _RENDER_H_

#include "xee/fnd/data_type.h"

#include "gpgx/ppu/vdp/bg_layer_renderer.h"
#include "gpgx/ppu/vdp/bg_pattern_cache_updater.h"
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
#include "gpgx/ppu/vdp/m5_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_bg_pattern_cache_updater.h"
#include "gpgx/ppu/vdp/m5_im2_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_ste_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_vs_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_satb_parser.h"
#include "gpgx/ppu/vdp/m5_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_ste_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_vs_bg_layer_renderer.h"
#include "gpgx/ppu/vdp/satb_parser.h"
#include "gpgx/ppu/vdp/sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/tms_satb_parser.h"
#include "gpgx/ppu/vdp/tms_sprite_layer_renderer.h"

/* 3:3:2 RGB */
#if defined(USE_8BPP_RENDERING)
#define PIXEL(r,g,b) (((r) << 5) | ((g) << 2) | (b))
#define GET_R(pixel) (((pixel) & 0xe0) >> 5)
#define GET_G(pixel) (((pixel) & 0x1c) >> 2)
#define GET_B(pixel) (((pixel) & 0x03) >> 0)

/* 5:5:5 RGB */
#elif defined(USE_15BPP_RENDERING)
#if defined(USE_ABGR)
#define PIXEL(r,g,b) ((1 << 15) | ((b) << 10) | ((g) << 5) | (r))
#define GET_B(pixel) (((pixel) & 0x7c00) >> 10)
#define GET_G(pixel) (((pixel) & 0x03e0) >> 5)
#define GET_R(pixel) (((pixel) & 0x001f) >> 0)
#else
#define PIXEL(r,g,b) ((1 << 15) | ((r) << 10) | ((g) << 5) | (b))
#define GET_R(pixel) (((pixel) & 0x7c00) >> 10)
#define GET_G(pixel) (((pixel) & 0x03e0) >> 5)
#define GET_B(pixel) (((pixel) & 0x001f) >> 0)
#endif

/* 5:6:5 RGB */
#elif defined(USE_16BPP_RENDERING)
#define PIXEL(r,g,b) (((r) << 11) | ((g) << 5) | (b))
#define GET_R(pixel) (((pixel) & 0xf800) >> 11)
#define GET_G(pixel) (((pixel) & 0x07e0) >> 5)
#define GET_B(pixel) (((pixel) & 0x001f) >> 0)

/* 8:8:8 RGB */
#elif defined(USE_32BPP_RENDERING)
#define PIXEL(r,g,b) ((0xff << 24) | ((r) << 16) | ((g) << 8) | (b))
#define GET_R(pixel) (((pixel) & 0xff0000) >> 16)
#define GET_G(pixel) (((pixel) & 0x00ff00) >> 8)
#define GET_B(pixel) (((pixel) & 0x0000ff) >> 0)
#endif

/* LCD image persistence (ghosting) filter */
/* Simulates (roughly) the slow decay response time of passive-matrix LCD */
/* Rate value is formatted as 0.8 fixed-point integer (between 0.0 and 0.99609375), a higher value meaning a slower decay */
/* Required for proper display of some effects in a few Game Gear games (James Pond 3, Power Drift, Super Monaco GP II,...) */
#define RENDER_PIXEL_LCD(in,out,table,rate) \
{ \
  PIXEL_OUT_T pixel_out = table[*in++]; \
  PIXEL_OUT_T pixel_old  = *out; \
  u8 r = GET_R(pixel_out); \
  u8 g = GET_G(pixel_out); \
  u8 b = GET_B(pixel_out); \
  int r_decay = GET_R(pixel_old) - r; \
  int g_decay = GET_G(pixel_old) - g; \
  int b_decay = GET_B(pixel_old) - b; \
  if (r_decay > 0) r += (rate * r_decay) >> 8; \
  if (g_decay > 0) g += (rate * g_decay) >> 8; \
  if (b_decay > 0) b += (rate * b_decay) >> 8; \
  *out++ = PIXEL(r,g,b); \
}

/* Global variables */
extern u16 spr_col;

/* Function prototypes */
extern void render_init(void);
extern void render_reset(void);
extern void render_line(int line);
extern void blank_line(int line, int offset, int width);
extern void remap_line(int line);
extern void window_clip(unsigned int data, unsigned int sw);
extern void color_update_m4(int index, unsigned int data);
extern void color_update_m5(int index, unsigned int data);

//------------------------------------------------------------------------------
// Background layer rendering.

/// Renderer of background layer.
extern gpgx::ppu::vdp::IBackgroundLayerRenderer* g_bg_layer_renderer;

/// Renderer of background layer in invalid mode (1+3 or 1+2+3).
extern gpgx::ppu::vdp::InvalidBackgroundLayerRenderer* g_bg_layer_renderer_inv;

/// Renderer of background layer in mode 0 (Graphics I).
extern gpgx::ppu::vdp::M0BackgroundLayerRenderer* g_bg_layer_renderer_m0;

/// Renderer of background layer in mode 1 (Text).
extern gpgx::ppu::vdp::M1BackgroundLayerRenderer* g_bg_layer_renderer_m1;

/// Renderer of background layer in mode 1 (Text) with Extended PG.
extern gpgx::ppu::vdp::M1XBackgroundLayerRenderer* g_bg_layer_renderer_m1x;

/// Renderer of background layer in mode 2 (Graphics II).
extern gpgx::ppu::vdp::M2BackgroundLayerRenderer* g_bg_layer_renderer_m2;

/// Renderer of background layer in mode 3 (Multicolor).
extern gpgx::ppu::vdp::M3BackgroundLayerRenderer* g_bg_layer_renderer_m3;

/// Renderer of background layer in mode 3 (Multicolor) with Extended PG.
extern gpgx::ppu::vdp::M3XBackgroundLayerRenderer* g_bg_layer_renderer_m3x;

/// Renderer of background layer in mode 4.
extern gpgx::ppu::vdp::M4BackgroundLayerRenderer* g_bg_layer_renderer_m4;

/// Renderer of background layer in mode 5.
extern gpgx::ppu::vdp::M5BackgroundLayerRenderer* g_bg_layer_renderer_m5;

/// Renderer of background layer in mode 5 with interlace double resolution 
/// (IM2) enabled.
extern gpgx::ppu::vdp::M5Im2BackgroundLayerRenderer* g_bg_layer_renderer_m5_im2;

/// Renderer of background layer in mode 5 with interlace double resolution 
/// (IM2) enabled and 16 pixel column vertical scrolling.
extern gpgx::ppu::vdp::M5Im2VsBackgroundLayerRenderer* g_bg_layer_renderer_m5_im2_vs;

/// Renderer of background layer in mode 5 with 16 pixel column vertical scrolling.
extern gpgx::ppu::vdp::M5VsBackgroundLayerRenderer* g_bg_layer_renderer_m5_vs;

/// Renderers of background layer.
/// Index = M1 M3 M4 M2
extern gpgx::ppu::vdp::IBackgroundLayerRenderer* g_bg_layer_renderer_modes[16];

//------------------------------------------------------------------------------
// Sprite layer rendering.

/// Renderer of sprite layer.
extern gpgx::ppu::vdp::ISpriteLayerRenderer* g_sprite_layer_renderer;

/// Renderer of sprite layer in mode TMS.
extern gpgx::ppu::vdp::TmsSpriteLayerRenderer* g_sprite_layer_renderer_tms;

/// Renderer of sprite layer in mode 4.
extern gpgx::ppu::vdp::M4SpriteLayerRenderer* g_sprite_layer_renderer_m4;

/// Renderer of sprite layer in mode 5.
extern gpgx::ppu::vdp::M5SpriteLayerRenderer* g_sprite_layer_renderer_m5;

/// Renderer of sprite layer in mode 5 (STE).
extern gpgx::ppu::vdp::M5SteSpriteLayerRenderer* g_sprite_layer_renderer_m5_ste;

/// Renderer of sprite layer in mode 5 (IM2).
extern gpgx::ppu::vdp::M5Im2SpriteLayerRenderer* g_sprite_layer_renderer_m5_im2;

/// Renderer of sprite layer in mode 5 (IM2/STE).
extern gpgx::ppu::vdp::M5Im2SteSpriteLayerRenderer* g_sprite_layer_renderer_m5_im2_ste;

//------------------------------------------------------------------------------
// Sprite attribute table parsing.

/// Parser of sprite attribute table.
extern gpgx::ppu::vdp::ISpriteAttributeTableParser* g_satb_parser;

/// Parser of sprite attribute table in mode TMS.
extern gpgx::ppu::vdp::TmsSpriteAttributeTableParser* g_satb_parser_tms;

/// Parser of sprite attribute table in mode 4.
extern gpgx::ppu::vdp::M4SpriteAttributeTableParser* g_satb_parser_m4;

/// Parser of sprite attribute table in mode 5.
extern gpgx::ppu::vdp::M5SpriteAttributeTableParser* g_satb_parser_m5;

//------------------------------------------------------------------------------
// Background pattern cache updating.

/// Updater of background pattern cache.
extern gpgx::ppu::vdp::IBackgroundPatternCacheUpdater* g_bg_pattern_cache_updater;

/// Updater of background pattern cache in mode 4.
extern gpgx::ppu::vdp::M4BackgroundPatternCacheUpdater* g_bg_pattern_cache_updater_m4;

/// Updater of background pattern cache in mode 5.
extern gpgx::ppu::vdp::M5BackgroundPatternCacheUpdater* g_bg_pattern_cache_updater_m5;

#endif /* _RENDER_H_ */
