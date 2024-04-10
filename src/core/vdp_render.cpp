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
#include "core/vdp/object_info_t.h"

#include "gpgx/ppu/vdp/m4_bg_pattern_cache_updater.h"
#include "gpgx/ppu/vdp/m4_satb_parser.h"
#include "gpgx/ppu/vdp/m4_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_bg_pattern_cache_updater.h"
#include "gpgx/ppu/vdp/m5_im2_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_im2_ste_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_satb_parser.h"
#include "gpgx/ppu/vdp/m5_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/m5_ste_sprite_layer_renderer.h"
#include "gpgx/ppu/vdp/tms_satb_parser.h"
#include "gpgx/ppu/vdp/tms_sprite_layer_renderer.h"

#ifndef HAVE_NO_SPRITE_LIMIT
#define MAX_SPRITES_PER_LINE 20
#define MODE5_MAX_SPRITE_PIXELS max_sprite_pixels
#endif

/* Output pixels type*/
#if defined(USE_8BPP_RENDERING)
#define PIXEL_OUT_T u8
#elif defined(USE_32BPP_RENDERING)
#define PIXEL_OUT_T u32
#else
#define PIXEL_OUT_T u16
#endif


/* Pixel priority look-up tables information */
#define LUT_MAX     (6)
#define LUT_SIZE    (0x10000)


#ifdef ALIGN_LONG
#undef READ_LONG
#undef WRITE_LONG

static XEE_INLINE u32 READ_LONG(void *address)
{
  if ((u32)address & 3)
  {
#ifdef LSB_FIRST  /* little endian version */
    return ( *((u8 *)address) +
        (*((u8 *)address+1) << 8)  +
        (*((u8 *)address+2) << 16) +
        (*((u8 *)address+3) << 24) );
#else       /* big endian version */
    return ( *((u8 *)address+3) +
        (*((u8 *)address+2) << 8)  +
        (*((u8 *)address+1) << 16) +
        (*((u8 *)address)   << 24) );
#endif  /* LSB_FIRST */
  }
  else return *(u32 *)address;
}

static XEE_INLINE void WRITE_LONG(void *address, u32 data)
{
  if ((u32)address & 3)
  {
#ifdef LSB_FIRST
      *((u8 *)address) =  data;
      *((u8 *)address+1) = (data >> 8);
      *((u8 *)address+2) = (data >> 16);
      *((u8 *)address+3) = (data >> 24);
#else
      *((u8 *)address+3) =  data;
      *((u8 *)address+2) = (data >> 8);
      *((u8 *)address+1) = (data >> 16);
      *((u8 *)address)   = (data >> 24);
#endif /* LSB_FIRST */
    return;
  }
  else *(u32 *)address = data;
}

#endif  /* ALIGN_LONG */


/* Draw 2-cell column (8-pixels high) */
/*
   Pattern cache base address: VHN NNNNNNNN NNYYYxxx
   with :
      x = Pattern Pixel (0-7)
      Y = Pattern Row (0-7)
      N = Pattern Number (0-2047) from pattern attribute
      H = Horizontal Flip bit from pattern attribute
      V = Vertical Flip bit from pattern attribute
*/
#define GET_LSB_TILE(ATTR, LINE) \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (u32 *)&bg_pattern_cache[(ATTR & 0x00001FFF) << 6 | (LINE)];
#define GET_MSB_TILE(ATTR, LINE) \
  atex = atex_table[(ATTR >> 29) & 7]; \
  src = (u32 *)&bg_pattern_cache[(ATTR & 0x1FFF0000) >> 10 | (LINE)];

/* Draw 2-cell column (16 pixels high) */
/*
   Pattern cache base address: VHN NNNNNNNN NYYYYxxx
   with :
      x = Pattern Pixel (0-7)
      Y = Pattern Row (0-15)
      N = Pattern Number (0-1023)
      H = Horizontal Flip bit
      V = Vertical Flip bit
*/
#define GET_LSB_TILE_IM2(ATTR, LINE) \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (u32 *)&bg_pattern_cache[((ATTR & 0x000003FF) << 7 | (ATTR & 0x00001800) << 6 | (LINE)) ^ ((ATTR & 0x00001000) >> 6)];
#define GET_MSB_TILE_IM2(ATTR, LINE) \
  atex = atex_table[(ATTR >> 29) & 7]; \
  src = (u32 *)&bg_pattern_cache[((ATTR & 0x03FF0000) >> 9 | (ATTR & 0x18000000) >> 10 | (LINE)) ^ ((ATTR & 0x10000000) >> 22)];

/*
   One column = 2 tiles
   Two pattern attributes are written in VRAM as two consecutives 16-bit words:

   P = priority bit
   C = color palette (2 bits)
   V = Vertical Flip bit
   H = Horizontal Flip bit
   N = Pattern Number (11 bits)

   (MSB) PCCVHNNN NNNNNNNN (LSB) (MSB) PCCVHNNN NNNNNNNN (LSB)
              PATTERN1                      PATTERN2

   Both pattern attributes are read from VRAM as one 32-bit word:

   LIT_ENDIAN: (MSB) PCCVHNNN NNNNNNNN PCCVHNNN NNNNNNNN (LSB)
                          PATTERN2          PATTERN1

   BIG_ENDIAN: (MSB) PCCVHNNN NNNNNNNN PCCVHNNN NNNNNNNN (LSB)
                          PATTERN1          PATTERN2


   In line buffers, one pixel = one byte: (msb) 0Pppcccc (lsb)
   with:
      P = priority bit  (from pattern attribute)
      p = color palette (from pattern attribute)
      c = color data (from pattern cache)

   One pattern = 8 pixels = 8 bytes = two 32-bit writes per pattern
*/

#ifdef ALIGN_LONG
#ifdef LSB_FIRST
#define DRAW_COLUMN(ATTR, LINE) \
  GET_LSB_TILE(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++; \
  GET_MSB_TILE(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++;
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++; \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++;
#else
#define DRAW_COLUMN(ATTR, LINE) \
  GET_MSB_TILE(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++; \
  GET_LSB_TILE(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++;
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++; \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  WRITE_LONG(dst, src[0] | atex); \
  dst++; \
  WRITE_LONG(dst, src[1] | atex); \
  dst++;
#endif
#else /* NOT ALIGNED */
#ifdef LSB_FIRST
#define DRAW_COLUMN(ATTR, LINE) \
  GET_LSB_TILE(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex); \
  GET_MSB_TILE(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex);
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex); \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex);
#else
#define DRAW_COLUMN(ATTR, LINE) \
  GET_MSB_TILE(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex); \
  GET_LSB_TILE(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex);
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex); \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  *dst++ = (src[0] | atex); \
  *dst++ = (src[1] | atex);
#endif
#endif /* ALIGN_LONG */

#ifdef ALT_RENDERER
/* Draw background tiles directly using priority look-up table */
/* SRC_A = layer A rendered pixel line (4 bytes = 4 pixels at once) */
/* SRC_B = layer B cached pixel line (4 bytes = 4 pixels at once) */
/* Note: cache address is always aligned so no need to use READ_LONG macro */
/* This might be faster or slower than original method, depending on  */
/* architecture (x86, PowerPC), cache size, memory access speed, etc...  */

#ifdef LSB_FIRST
#define DRAW_BG_TILE(SRC_A, SRC_B) \
  *lb++ = table[((SRC_B << 8) & 0xff00) | (SRC_A & 0xff)]; \
  *lb++ = table[(SRC_B & 0xff00) | ((SRC_A >> 8) & 0xff)]; \
  *lb++ = table[((SRC_B >> 8) & 0xff00) | ((SRC_A >> 16) & 0xff)]; \
  *lb++ = table[((SRC_B >> 16) & 0xff00) | ((SRC_A >> 24) & 0xff)];
#else
#define DRAW_BG_TILE(SRC_A, SRC_B) \
  *lb++ = table[((SRC_B >> 16) & 0xff00) | ((SRC_A >> 24) & 0xff)]; \
  *lb++ = table[((SRC_B >> 8) & 0xff00) | ((SRC_A >> 16) & 0xff)]; \
  *lb++ = table[(SRC_B & 0xff00) | ((SRC_A >> 8) & 0xff)]; \
  *lb++ = table[((SRC_B << 8) & 0xff00) | (SRC_A & 0xff)];
#endif

#ifdef ALIGN_LONG
#ifdef LSB_FIRST
#define DRAW_BG_COLUMN(ATTR, LINE, SRC_A, SRC_B) \
  GET_LSB_TILE(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_MSB_TILE(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#define DRAW_BG_COLUMN_IM2(ATTR, LINE, SRC_A, SRC_B) \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#else
#define DRAW_BG_COLUMN(ATTR, LINE, SRC_A, SRC_B) \
  GET_MSB_TILE(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_LSB_TILE(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#define DRAW_BG_COLUMN_IM2(ATTR, LINE, SRC_A, SRC_B) \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = READ_LONG((u32 *)lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#endif
#else /* NOT ALIGNED */
#ifdef LSB_FIRST
#define DRAW_BG_COLUMN(ATTR, LINE, SRC_A, SRC_B) \
  GET_LSB_TILE(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_MSB_TILE(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#define DRAW_BG_COLUMN_IM2(ATTR, LINE, SRC_A, SRC_B) \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#else
#define DRAW_BG_COLUMN(ATTR, LINE, SRC_A, SRC_B) \
  GET_MSB_TILE(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_LSB_TILE(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#define DRAW_BG_COLUMN_IM2(ATTR, LINE, SRC_A, SRC_B) \
  GET_MSB_TILE_IM2(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  GET_LSB_TILE_IM2(ATTR, LINE) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[0] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B) \
  SRC_A = *(u32 *)(lb); \
  SRC_B = (src[1] | atex); \
  DRAW_BG_TILE(SRC_A, SRC_B)
#endif
#endif /* ALIGN_LONG */
#endif /* ALT_RENDERER */

#define DRAW_SPRITE_TILE(WIDTH,ATTR,TABLE)  \
  for (i=0;i<WIDTH;i++) \
  { \
    temp = *src++; \
    if (temp & 0x0f) \
    { \
      temp |= (lb[i] << 8); \
      lb[i] = TABLE[temp | ATTR]; \
      status |= ((temp & 0x8000) >> 10); \
    } \
  }

#define DRAW_SPRITE_TILE_ACCURATE(WIDTH,ATTR,TABLE)  \
  for (i=0;i<WIDTH;i++) \
  { \
    temp = *src++; \
    if (temp & 0x0f) \
    { \
      temp |= (lb[i] << 8); \
      lb[i] = TABLE[temp | ATTR]; \
      if ((temp & 0x8000) && !(status & 0x20)) \
      { \
        spr_col = (v_counter << 8) | ((xpos + i + 13) >> 1); \
        status |= 0x20; \
      } \
    } \
  }

#define DRAW_SPRITE_TILE_ACCURATE_2X(WIDTH,ATTR,TABLE)  \
  for (i=0;i<WIDTH;i+=2) \
  { \
    temp = *src++; \
    if (temp & 0x0f) \
    { \
      temp |= (lb[i] << 8); \
      lb[i] = TABLE[temp | ATTR]; \
      if ((temp & 0x8000) && !(status & 0x20)) \
      { \
        spr_col = (v_counter << 8) | ((xpos + i + 13) >> 1); \
        status |= 0x20; \
      } \
      temp &= 0x00FF; \
      temp |= (lb[i+1] << 8); \
      lb[i+1] = TABLE[temp | ATTR]; \
      if ((temp & 0x8000) && !(status & 0x20)) \
      { \
        spr_col = (v_counter << 8) | ((xpos + i + 1 + 13) >> 1); \
        status |= 0x20; \
      } \
    } \
  }


/* Pixels conversion macro */
/* 4-bit color channels are either compressed to 2/3-bit or dithered to 5/6/8-bit equivalents */
/* 3:3:2 RGB */
#if defined(USE_8BPP_RENDERING)
#define MAKE_PIXEL(r,g,b)  (((r) >> 1) << 5 | ((g) >> 1) << 2 | (b) >> 2)

/* 5:5:5 RGB */
#elif defined(USE_15BPP_RENDERING)
#if defined(USE_ABGR)
#define MAKE_PIXEL(r,g,b) ((1 << 15) | (b) << 11 | ((b) >> 3) << 10 | (g) << 6 | ((g) >> 3) << 5 | (r) << 1 | (r) >> 3)
#else
#define MAKE_PIXEL(r,g,b) ((1 << 15) | (r) << 11 | ((r) >> 3) << 10 | (g) << 6 | ((g) >> 3) << 5 | (b) << 1 | (b) >> 3)
#endif
/* 5:6:5 RGB */
#elif defined(USE_16BPP_RENDERING)
#define MAKE_PIXEL(r,g,b) ((r) << 12 | ((r) >> 3) << 11 | (g) << 7 | ((g) >> 2) << 5 | (b) << 1 | (b) >> 3)

/* 8:8:8 RGB */
#elif defined(USE_32BPP_RENDERING)
#define MAKE_PIXEL(r,g,b) ((0xff << 24) | (r) << 20 | (r) << 16 | (g) << 12 | (g)  << 8 | (b) << 4 | (b))
#endif

/* Window & Plane A clipping */
static struct clip_t
{
  u8 left;
  u8 right;
  u8 enable;
} clip[2];

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
static const u8 tms_palette[16] =
{
  0x00, 0x00, 0x39, 0x79,
  0x4B, 0x6F, 0xC9, 0x5B,
  0xE9, 0xED, 0xD5, 0xD9,
  0x35, 0xCE, 0xDA, 0xFF
};

#elif defined(USE_15BPP_RENDERING)
static const u16 tms_palette[16] =
{
  0x8000, 0x8000, 0x9308, 0xAF6F,
  0xA95D, 0xBDDF, 0xE949, 0xA3BE,
  0xFD4A, 0xFDEF, 0xEB0A, 0xF330,
  0x92A7, 0xE177, 0xE739, 0xFFFF
};

#elif defined(USE_16BPP_RENDERING)
static const u16 tms_palette[16] =
{
  0x0000, 0x0000, 0x2648, 0x5ECF,
  0x52BD, 0x7BBE, 0xD289, 0x475E,
  0xF2AA, 0xFBCF, 0xD60A, 0xE670,
  0x2567, 0xC2F7, 0xCE59, 0xFFFF
};

#elif defined(USE_32BPP_RENDERING)
static const u32 tms_palette[16] =
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

/* Layer priority pixel look-up tables */
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

/* Function pointers */
void (*render_bg)(int line);

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
/* Pixel layer merging function                                             */
/*--------------------------------------------------------------------------*/

static XEE_INLINE void merge(u8 *srca, u8 *srcb, u8 *dst, u8 *table, int width)
{
  do
  {
    *dst++ = table[(*srcb++ << 8) | (*srca++)];
  }
  while (--width);
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
/* Background layers rendering functions                                    */
/*--------------------------------------------------------------------------*/

/* Graphics I */
void render_bg_m0(int line)
{
  u8 color, name, pattern;

  u8 *lb = &linebuf[0][0x20];
  u8 *nt = &vram[((reg[2] << 10) & 0x3C00) + ((line & 0xF8) << 2)];
  u8 *ct = &vram[((reg[3] <<  6) & 0x3FC0)];
  u8 *pg = &vram[((reg[4] << 11) & 0x3800) + (line & 7)];

  /* 32 x 8 pixels */
  int width = 32;

  do
  {
    name = *nt++;
    color = ct[name >> 3];
    pattern = pg[name << 3];

    *lb++ = 0x10 | ((color >> (((pattern >> 7) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 6) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 5) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 4) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 3) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 2) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 1) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 0) & 1) << 2)) & 0x0F);
  }
  while (--width);
}

/* Text */
void render_bg_m1(int line)
{
  u8 pattern;
  u8 color = reg[7];

  u8 *lb = &linebuf[0][0x20];
  u8 *nt = &vram[((reg[2] << 10) & 0x3C00) + ((line >> 3) * 40)];
  u8 *pg = &vram[((reg[4] << 11) & 0x3800) + (line & 7)];

  /* 40 x 6 pixels */
  int width = 40;

  /* Left border (8 pixels) */
  xee::mem::Memset (lb, 0x40, 8);
  lb += 8;

  do
  {
    pattern = pg[*nt++ << 3];

    *lb++ = 0x10 | ((color >> (((pattern >> 7) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 6) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 5) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 4) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 3) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 2) & 1) << 2)) & 0x0F);
  }
  while (--width);

  /* Right borders (8 pixels) */
  xee::mem::Memset(lb, 0x40, 8);
}

/* Text + extended PG */
void render_bg_m1x(int line)
{
  u8 pattern;
  u8 *pg;

  u8 color = reg[7];

  u8 *lb = &linebuf[0][0x20];
  u8 *nt = &vram[((reg[2] << 10) & 0x3C00) + ((line >> 3) * 40)];

  u16 pg_mask = ~0x3800 ^ (reg[4] << 11);

  /* 40 x 6 pixels */
  int width = 40;

  /* Unused bits used as a mask on TMS99xx & 315-5124 VDP only */
  if (system_hw > SYSTEM_SMS)
  {
    pg_mask |= 0x1800;
  }

  pg = &vram[((0x2000 + ((line & 0xC0) << 5)) & pg_mask) + (line & 7)];

  /* Left border (8 pixels) */
  xee::mem::Memset (lb, 0x40, 8);
  lb += 8;

  do
  {
    pattern = pg[*nt++ << 3];

    *lb++ = 0x10 | ((color >> (((pattern >> 7) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 6) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 5) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 4) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 3) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 2) & 1) << 2)) & 0x0F);
  }
  while (--width);

  /* Right borders (8 pixels) */
  xee::mem::Memset(lb, 0x40, 8);
}

/* Graphics II */
void render_bg_m2(int line)
{
  u8 color, pattern;
  u16 name;
  u8 *ct, *pg;

  u8 *lb = &linebuf[0][0x20];
  u8 *nt = &vram[((reg[2] << 10) & 0x3C00) + ((line & 0xF8) << 2)];

  u16 ct_mask = ~0x3FC0 ^ (reg[3] << 6);
  u16 pg_mask = ~0x3800 ^ (reg[4] << 11);

  /* 32 x 8 pixels */
  int width = 32;

  /* Unused bits used as a mask on TMS99xx & 315-5124 VDP only */
  if (system_hw > SYSTEM_SMS)
  {
    ct_mask |= 0x1FC0;
    pg_mask |= 0x1800;
  }

  ct = &vram[((0x2000 + ((line & 0xC0) << 5)) & ct_mask) + (line & 7)];
  pg = &vram[((0x2000 + ((line & 0xC0) << 5)) & pg_mask) + (line & 7)];

  do
  {
    name = *nt++ << 3 ;
    color = ct[name & ct_mask];
    pattern = pg[name];

    *lb++ = 0x10 | ((color >> (((pattern >> 7) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 6) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 5) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 4) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 3) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 2) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 1) & 1) << 2)) & 0x0F);
    *lb++ = 0x10 | ((color >> (((pattern >> 0) & 1) << 2)) & 0x0F);
  }
  while (--width);
}

/* Multicolor */
void render_bg_m3(int line)
{
  u8 color;
  u8 *lb = &linebuf[0][0x20];
  u8 *nt = &vram[((reg[2] << 10) & 0x3C00) + ((line & 0xF8) << 2)];
  u8 *pg = &vram[((reg[4] << 11) & 0x3800) + ((line >> 2) & 7)];

  /* 32 x 8 pixels */
  int width = 32;

  do
  {
    color = pg[*nt++ << 3];

    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
  }
  while (--width);
}

/* Multicolor + extended PG */
void render_bg_m3x(int line)
{
  u8 color;
  u8 *pg;

  u8 *lb = &linebuf[0][0x20];
  u8 *nt = &vram[((reg[2] << 10) & 0x3C00) + ((line & 0xF8) << 2)];

  u16 pg_mask = ~0x3800 ^ (reg[4] << 11);

  /* 32 x 8 pixels */
  int width = 32;

  /* Unused bits used as a mask on TMS99xx & 315-5124 VDP only */
  if (system_hw > SYSTEM_SMS)
  {
    pg_mask |= 0x1800;
  }

  pg = &vram[((0x2000 + ((line & 0xC0) << 5)) & pg_mask) + ((line >> 2) & 7)];

  do
  {
    color = pg[*nt++ << 3];

    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
  }
  while (--width);
}

/* Invalid (2+3/1+2+3) */
void render_bg_inv(int line)
{
  u8 color = reg[7];

  u8 *lb = &linebuf[0][0x20];

  /* 40 x 6 pixels */
  int width = 40;

  /* Left border (8 pixels) */
  xee::mem::Memset (lb, 0x40, 8);
  lb += 8;

  do
  {
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 4) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
    *lb++ = 0x10 | ((color >> 0) & 0x0F);
  }
  while (--width);

  /* Right borders (8 pixels) */
  xee::mem::Memset(lb, 0x40, 8);
}

/* Mode 4 */
void render_bg_m4(int line)
{
  int column;
  u16 *nt;
  u32 attr, atex, *src;

  /* 32 x 8 pixels */
  int width = 32;

  /* Horizontal scrolling */
  int index = ((reg[0] & 0x40) && (line < 0x10)) ? 0x100 : reg[0x08];
  int shift = index & 7;

  /* Background line buffer */
  u32 *dst = (u32 *)&linebuf[0][0x20 + shift];

  /* Vertical scrolling */
  int v_line = line + vscroll;

  /* Pattern name table mask */
  u16 nt_mask = ~0x3C00 ^ (reg[2] << 10);

  /* Unused bits used as a mask on TMS99xx & 315-5124 VDP only */
  if (system_hw > SYSTEM_SMS)
  {
    nt_mask |= 0x400;
  }

  /* Test for extended modes (Master System II & Game gear VDP only) */
  if (viewport.h > 192)
  {
    /* Vertical scroll mask */
    v_line = v_line % 256;

    /* Pattern name Table */
    nt = (u16 *)&vram[(0x3700 & nt_mask) + ((v_line >> 3) << 6)];
  }
  else
  {
    /* Vertical scroll mask */
    v_line = v_line % 224;

    /* Pattern name Table */
    nt = (u16 *)&vram[(0x3800 + ((v_line >> 3) << 6)) & nt_mask];
  }

  /* Pattern row index */
  v_line = (v_line & 7) << 3;

  /* Tile column index */
  index = (0x100 - index) >> 3;

  /* Clip left-most column if required */
  if (shift)
  {
    xee::mem::Memset(&linebuf[0][0x20], 0, shift);
    index++;
  }

  /* Draw tiles */
  for(column = 0; column < width; column++, index++)
  {
    /* Stop vertical scrolling for rightmost eight tiles */
    if((column == 24) && (reg[0] & 0x80))
    {
      /* Clear Pattern name table start address */
      if (viewport.h > 192)
      {
        nt = (u16 *)&vram[(0x3700 & nt_mask) + ((line >> 3) << 6)];
      }
      else
      {
        nt = (u16 *)&vram[(0x3800 + ((line >> 3) << 6)) & nt_mask];
      }

      /* Clear Pattern row index */
      v_line = (line & 7) << 3;
    }

    /* Read name table attribute word */
    attr = nt[index % width];
#ifndef LSB_FIRST
    attr = (((attr & 0xFF) << 8) | ((attr & 0xFF00) >> 8));
#endif

    /* Expand priority and palette bits */
    atex = atex_table[(attr >> 11) & 3];

    /* Cached pattern data line (4 bytes = 4 pixels at once) */
    src = (u32 *)&bg_pattern_cache[((attr & 0x7FF) << 6) | (v_line)];

    /* Copy left & right half, adding the attribute bits in */
#ifdef ALIGN_LONG
    WRITE_LONG(dst, src[0] | atex);
    dst++;
    WRITE_LONG(dst, src[1] | atex);
    dst++;
#else
    *dst++ = (src[0] | atex);
    *dst++ = (src[1] | atex);
#endif
  }
}

/* Mode 5 */
#ifndef ALT_RENDERER
void render_bg_m5(int line)
{
  int column;
  u32 atex, atbuf, *src, *dst;

  /* Common data */
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = *(u32 *)&vsram[0];
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;

  /* Window & Plane A */
  int a = (reg[18] & 0x1F) << 3;
  int w = (reg[18] >> 7) & 1;

  /* Plane B width */
  int start = 0;
  int end = viewport.w >> 4;

  /* Plane B scroll */
#ifdef LSB_FIRST
  u32 shift  = (xscroll >> 16) & 0x0F;
  u32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
  u32 v_line = (line + (yscroll >> 16)) & pf_row_mask;
#else
  u32 shift  = (xscroll & 0x0F);
  u32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
  u32 v_line = (line + yscroll) & pf_row_mask;
#endif

  /* Plane B name table */
  u32 *nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

  /* Pattern row index */
  v_line = (v_line & 7) << 3;

  if(shift)
  {
    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x10 + shift];

    atbuf = nt[(index - 1) & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }
  else
  {
    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x20];
  }

  for(column = 0; column < end; column++, index++)
  {
    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }

  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A scroll */
#ifdef LSB_FIRST
    shift   = (xscroll & 0x0F);
    index   = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
    v_line  = (line + yscroll) & pf_row_mask;
#else
    shift   = (xscroll >> 16) & 0x0F;
    index   = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
    v_line  = (line + (yscroll >> 16)) & pf_row_mask;
#endif

    /* Plane A name table */
    nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    if(shift)
    {
      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x10 + shift + (start << 4)];

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index - 1) & pf_col_mask];
      }

      DRAW_COLUMN(atbuf, v_line)
    }
    else
    {
      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x20 + (start << 4)];
    }

    for(column = start; column < end; column++, index++)
    {
      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = (line & 7) << 3;

    /* Plane A line buffer */
    dst = (u32 *)&linebuf[1][0x20 + (start << 4)];

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }

  /* Merge background layers */
  merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[(reg[12] & 0x08) >> 2], viewport.w);
}

void render_bg_m5_vs(int line)
{
  int column;
  u32 atex, atbuf, *src, *dst;
  u32 v_line, *nt;

  /* Common data */
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = 0;
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;
  u32 *vs          = (u32 *)&vsram[0];

  /* Window & Plane A */
  int a = (reg[18] & 0x1F) << 3;
  int w = (reg[18] >> 7) & 1;

  /* Plane B width */
  int start = 0;
  int end = viewport.w >> 4;

  /* Plane B horizontal scroll */
#ifdef LSB_FIRST
  u32 shift  = (xscroll >> 16) & 0x0F;
  u32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  u32 shift  = (xscroll & 0x0F);
  u32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* Left-most column vertical scrolling when partially shown horizontally (verified on PAL MD2)  */
  /* TODO: check on Genesis 3 models since it apparently behaves differently  */
  /* In H32 mode, vertical scrolling is disabled, in H40 mode, same value is used for both planes */
  /* See Formula One / Kawasaki Superbike Challenge (H32) & Gynoug / Cutie Suzuki no Ringside Angel (H40) */
  if (reg[12] & 1)
  {
    yscroll = vs[19] & (vs[19] >> 16);
  }

  if(shift)
  {
    /* Plane B vertical scroll */
    v_line = (line + yscroll) & pf_row_mask;

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x10 + shift];

    atbuf = nt[(index - 1) & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }
  else
  {
    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x20];
  }

  for(column = 0; column < end; column++, index++)
  {
    /* Plane B vertical scroll */
#ifdef LSB_FIRST
    v_line = (line + (vs[column] >> 16)) & pf_row_mask;
#else
    v_line = (line + vs[column]) & pf_row_mask;
#endif

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }

  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A horizontal scroll */
#ifdef LSB_FIRST
    shift = (xscroll & 0x0F);
    index = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift = (xscroll >> 16) & 0x0F;
    index = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    if(shift)
    {
      /* Plane A vertical scroll */
      v_line = (line + yscroll) & pf_row_mask;

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x10 + shift + (start << 4)];

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index - 1) & pf_col_mask];
      }

      DRAW_COLUMN(atbuf, v_line)
    }
    else
    {
      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x20 + (start << 4)];
    }

    for(column = start; column < end; column++, index++)
    {
      /* Plane A vertical scroll */
#ifdef LSB_FIRST
      v_line = (line + vs[column]) & pf_row_mask;
#else
      v_line = (line + (vs[column] >> 16)) & pf_row_mask;
#endif

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = (line & 7) << 3;

    /* Plane A line buffer */
    dst = (u32 *)&linebuf[1][0x20 + (start << 4)];

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }

  /* Merge background layers */
  merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[(reg[12] & 0x08) >> 2], viewport.w);
}

/* Enhanced function that allows each cell to be vscrolled individually, instead of being limited to 2-cell */
void render_bg_m5_vs_enhanced(int line)
{
  int column;
  u32 atex, atbuf, *src, *dst;
  u32 v_line, next_v_line, *nt;

  /* Vertical scroll offset */
  int v_offset = 0;

  /* Common data */
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = 0;
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;
  u32 *vs          = (u32 *)&vsram[0];

  /* Window & Plane A */
  int a = (reg[18] & 0x1F) << 3;
  int w = (reg[18] >> 7) & 1;

  /* Plane B width */
  int start = 0;
  int end = viewport.w >> 4;

  /* Plane B horizontal scroll */
#ifdef LSB_FIRST
  u32 shift  = (xscroll >> 16) & 0x0F;
  u32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  u32 shift  = (xscroll & 0x0F);
  u32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* Left-most column vertical scrolling when partially shown horizontally (verified on PAL MD2)  */
  /* TODO: check on Genesis 3 models since it apparently behaves differently  */
  /* In H32 mode, vertical scrolling is disabled, in H40 mode, same value is used for both planes */
  /* See Formula One / Kawasaki Superbike Challenge (H32) & Gynoug / Cutie Suzuki no Ringside Angel (H40) */
  if (reg[12] & 1)
  {
    yscroll = vs[19] & (vs[19] >> 16);
  }

  if(shift)
  {
    /* Plane B vertical scroll */
    v_line = (line + yscroll) & pf_row_mask;

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x10 + shift];

    atbuf = nt[(index - 1) & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }
  else
  {
    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x20];
  }

  for(column = 0; column < end; column++, index++)
  {
    /* Plane B vertical scroll */
#ifdef LSB_FIRST
    v_line = (line + (vs[column] >> 16)) & pf_row_mask;
    next_v_line = (line + (vs[column + 1] >> 16)) & pf_row_mask;
#else
    v_line = (line + vs[column]) & pf_row_mask;
    next_v_line = (line + vs[column + 1]) & pf_row_mask;
#endif

    if (column != end - 1)
    {
      /* The offset of the intermediary cell is an average of the offsets of the current 2-cell and the next 2-cell. */
      /* For the last column, the previously calculated offset is used */
      v_offset = ((int)next_v_line - (int)v_line) / 2;
      v_offset = (abs(v_offset) >= core_config.enhanced_vscroll_limit) ? 0 : v_offset;
    }

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    atbuf = nt[index & pf_col_mask];
#ifdef LSB_FIRST
    GET_LSB_TILE(atbuf, v_line)
#else
    GET_MSB_TILE(atbuf, v_line)
#endif

#ifdef ALIGN_LONG
    WRITE_LONG(dst, src[0] | atex);
    dst++;
    WRITE_LONG(dst, src[1] | atex);
    dst++;
#else
    *dst++ = (src[0] | atex);
    *dst++ = (src[1] | atex);
#endif

#ifdef LSB_FIRST
    v_line = (line + v_offset + (vs[column] >> 16)) & pf_row_mask;
#else
    v_line = (line + v_offset + vs[column]) & pf_row_mask;
#endif

    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
    v_line = (v_line & 7) << 3;
    atbuf = nt[index & pf_col_mask];

#ifdef LSB_FIRST
    GET_MSB_TILE(atbuf, v_line)
#else
    GET_LSB_TILE(atbuf, v_line)
#endif
#ifdef ALIGN_LONG
    WRITE_LONG(dst, src[0] | atex);
    dst++;
    WRITE_LONG(dst, src[1] | atex);
    dst++;
#else
    *dst++ = (src[0] | atex);
    *dst++ = (src[1] | atex);
#endif
  }

  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A horizontal scroll */
#ifdef LSB_FIRST
    shift = (xscroll & 0x0F);
    index = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift = (xscroll >> 16) & 0x0F;
    index = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    if(shift)
    {
      /* Plane A vertical scroll */
      v_line = (line + yscroll) & pf_row_mask;

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x10 + shift + (start << 4)];

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index - 1) & pf_col_mask];
      }

      DRAW_COLUMN(atbuf, v_line)
    }
    else
    {
      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x20 + (start << 4)];
    }

    for(column = start; column < end; column++, index++)
    {
      /* Plane A vertical scroll */
#ifdef LSB_FIRST
      v_line = (line + vs[column]) & pf_row_mask;
      next_v_line = (line + vs[column + 1]) & pf_row_mask;
#else
      v_line = (line + (vs[column] >> 16)) & pf_row_mask;
      next_v_line = (line + (vs[column + 1] >> 16)) & pf_row_mask;
#endif

      if (column != end - 1)
      {
        v_offset = ((int)next_v_line - (int)v_line) / 2;
        v_offset = (abs(v_offset) >= core_config.enhanced_vscroll_limit) ? 0 : v_offset;
      }

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      atbuf = nt[index & pf_col_mask];
#ifdef LSB_FIRST
      GET_LSB_TILE(atbuf, v_line)
#else
      GET_MSB_TILE(atbuf, v_line)
#endif
#ifdef ALIGN_LONG
      WRITE_LONG(dst, src[0] | atex);
      dst++;
      WRITE_LONG(dst, src[1] | atex);
      dst++;
#else
      *dst++ = (src[0] | atex);
      *dst++ = (src[1] | atex);
#endif

#ifdef LSB_FIRST
      v_line = (line + v_offset + vs[column]) & pf_row_mask;
#else
      v_line = (line + v_offset + (vs[column] >> 16)) & pf_row_mask;
#endif

      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
      v_line = (v_line & 7) << 3;
      atbuf = nt[index & pf_col_mask];

#ifdef LSB_FIRST
      GET_MSB_TILE(atbuf, v_line)
#else
      GET_LSB_TILE(atbuf, v_line)
#endif
#ifdef ALIGN_LONG
      WRITE_LONG(dst, src[0] | atex);
      dst++;
      WRITE_LONG(dst, src[1] | atex);
      dst++;
#else
      *dst++ = (src[0] | atex);
      *dst++ = (src[1] | atex);
#endif
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = (line & 7) << 3;

    /* Plane A line buffer */
    dst = (u32 *)&linebuf[1][0x20 + (start << 4)];

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }

  /* Merge background layers */
  merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[(reg[12] & 0x08) >> 2], viewport.w);
}

void render_bg_m5_im2(int line)
{
  int column;
  u32 atex, atbuf, *src, *dst;

  /* Common data */
  int odd = odd_frame;
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = *(u32 *)&vsram[0];
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;

  /* Window & Plane A */
  int a = (reg[18] & 0x1F) << 3;
  int w = (reg[18] >> 7) & 1;

  /* Plane B width */
  int start = 0;
  int end = viewport.w >> 4;

  /* Plane B scroll */
#ifdef LSB_FIRST
  u32 shift  = (xscroll >> 16) & 0x0F;
  u32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
  u32 v_line = (line + (yscroll >> 17)) & pf_row_mask;
#else
  u32 shift  = (xscroll & 0x0F);
  u32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
  u32 v_line = (line + (yscroll >> 1)) & pf_row_mask;
#endif

  /* Plane B name table */
  u32 *nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

  /* Pattern row index */
  v_line = (((v_line & 7) << 1) | odd) << 3;

  if(shift)
  {
    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x10 + shift];

    atbuf = nt[(index - 1) & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }
  else
  {
    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x20];
  }

  for(column = 0; column < end; column++, index++)
  {
    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }

  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A scroll */
#ifdef LSB_FIRST
    shift   = (xscroll & 0x0F);
    index   = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
    v_line  = (line + (yscroll >> 1)) & pf_row_mask;
#else
    shift   = (xscroll >> 16) & 0x0F;
    index   = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
    v_line  = (line + (yscroll >> 17)) & pf_row_mask;
#endif

    /* Plane A name table */
    nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (((v_line & 7) << 1) | odd) << 3;

    if(shift)
    {
      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x10 + shift + (start << 4)];

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index - 1) & pf_col_mask];
      }

      DRAW_COLUMN_IM2(atbuf, v_line)
    }
    else
    {
      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x20 + (start << 4)];
    }

    for(column = start; column < end; column++, index++)
    {
      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = ((line & 7) << 1 | odd) << 3;

    /* Plane A line buffer */
    dst = (u32 *)&linebuf[1][0x20 + (start << 4)];

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }
  }

  /* Merge background layers */
  merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[(reg[12] & 0x08) >> 2], viewport.w);
}

void render_bg_m5_im2_vs(int line)
{
  int column;
  u32 atex, atbuf, *src, *dst;
  u32 v_line, *nt;

  /* Common data */
  int odd = odd_frame;
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = 0;
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;
  u32 *vs          = (u32 *)&vsram[0];

  /* Window & Plane A */
  int a = (reg[18] & 0x1F) << 3;
  int w = (reg[18] >> 7) & 1;

  /* Plane B width */
  int start = 0;
  int end = viewport.w >> 4;

  /* Plane B horizontal scroll */
#ifdef LSB_FIRST
  u32 shift  = (xscroll >> 16) & 0x0F;
  u32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  u32 shift  = (xscroll & 0x0F);
  u32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* Left-most column vertical scrolling when partially shown horizontally (verified on PAL MD2)  */
  /* TODO: check on Genesis 3 models since it apparently behaves differently  */
  /* In H32 mode, vertical scrolling is disabled, in H40 mode, same value is used for both planes */
  /* See Formula One / Kawasaki Superbike Challenge (H32) & Gynoug / Cutie Suzuki no Ringside Angel (H40) */
  if (reg[12] & 1)
  {
    yscroll = (vs[19] >> 1) & (vs[19] >> 17);
  }

  if(shift)
  {
    /* Plane B vertical scroll */
    v_line = (line + yscroll) & pf_row_mask;

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (((v_line & 7) << 1) | odd) << 3;

    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x10 + shift];

    atbuf = nt[(index - 1) & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }
  else
  {
    /* Plane B line buffer */
    dst = (u32 *)&linebuf[0][0x20];
  }

  for(column = 0; column < end; column++, index++)
  {
    /* Plane B vertical scroll */
#ifdef LSB_FIRST
    v_line = (line + (vs[column] >> 17)) & pf_row_mask;
#else
    v_line = (line + (vs[column] >> 1)) & pf_row_mask;
#endif

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (((v_line & 7) << 1) | odd) << 3;

    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }

  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A horizontal scroll */
#ifdef LSB_FIRST
    shift = (xscroll & 0x0F);
    index = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift = (xscroll >> 16) & 0x0F;
    index = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    if(shift)
    {
      /* Plane A vertical scroll */
      v_line = (line + yscroll) & pf_row_mask;

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (((v_line & 7) << 1) | odd) << 3;

      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x10 + shift + (start << 4)];

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index - 1) & pf_col_mask];
      }

      DRAW_COLUMN_IM2(atbuf, v_line)
    }
    else
    {
      /* Plane A line buffer */
      dst = (u32 *)&linebuf[1][0x20 + (start << 4)];
    }

    for(column = start; column < end; column++, index++)
    {
      /* Plane A vertical scroll */
#ifdef LSB_FIRST
      v_line = (line + (vs[column] >> 1)) & pf_row_mask;
#else
      v_line = (line + (vs[column] >> 17)) & pf_row_mask;
#endif

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (((v_line & 7) << 1) | odd) << 3;

      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = ((line & 7) << 1 | odd) << 3;

    /* Plane A line buffer */
    dst = (u32 *)&linebuf[1][0x20 + (start << 4)];

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }
  }

  /* Merge background layers */
  merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[(reg[12] & 0x08) >> 2], viewport.w);
}

#else

void render_bg_m5(int line)
{
  int column, start, end;
  u32 atex, atbuf, *src, *dst;
  u32 shift, index, v_line, *nt;
  u8 *lb;

  /* Scroll Planes common data */
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = *(u32 *)&vsram[0];
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;

  /* Number of columns to draw */
  int width = viewport.w >> 4;

  /* Layer priority table */
  u8 *table = lut[(reg[12] & 8) >> 2];

  /* Window vertical range (cell 0-31) */
  int a = (reg[18] & 0x1F) << 3;

  /* Window position (0=top, 1=bottom) */
  int w = (reg[18] >> 7) & 1;

  /* Test against current line */
  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A scroll */
#ifdef LSB_FIRST
    shift  = (xscroll & 0x0F);
    index  = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
    v_line = (line + yscroll) & pf_row_mask;
#else
    shift  = (xscroll >> 16) & 0x0F;
    index  = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
    v_line = (line + (yscroll >> 16)) & pf_row_mask;
#endif

    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4) + shift];

    /* Plane A name table */
    nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    if(shift)
    {
      /* Left-most column is partially shown */
      dst -= 4;

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index-1) & pf_col_mask];
      }

      DRAW_COLUMN(atbuf, v_line)
    }

    for(column = start; column < end; column++, index++)
    {
      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }
  else
  {
    /* Window width */
    start = 0;
    end = width;
  }

  /* Window Plane */
  if (w)
  {
    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4)];

    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = (line & 7) << 3;

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }

  /* Plane B scroll */
#ifdef LSB_FIRST
  shift  = (xscroll >> 16) & 0x0F;
  index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
  v_line = (line + (yscroll >> 16)) & pf_row_mask;
#else
  shift  = (xscroll & 0x0F);
  index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
  v_line = (line + yscroll) & pf_row_mask;
#endif

  /* Plane B name table */
  nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

  /* Pattern row index */
  v_line = (v_line & 7) << 3;

  /* Background line buffer */
  lb = &linebuf[0][0x20];

  if(shift)
  {
    /* Left-most column is partially shown */
    lb -= (0x10 - shift);

    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_BG_COLUMN(atbuf, v_line, xscroll, yscroll)
  }

  for(column = 0; column < width; column++, index++)
  {
    atbuf = nt[index & pf_col_mask];
    DRAW_BG_COLUMN(atbuf, v_line, xscroll, yscroll)
  }
}

void render_bg_m5_vs(int line)
{
  int column, start, end;
  u32 atex, atbuf, *src, *dst;
  u32 shift, index, v_line, *nt;
  u8 *lb;

  /* Scroll Planes common data */
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = 0;
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;
  u32 *vs          = (u32 *)&vsram[0];

  /* Number of columns to draw */
  int width = viewport.w >> 4;

  /* Layer priority table */
  u8 *table = lut[(reg[12] & 8) >> 2];

  /* Window vertical range (cell 0-31) */
  int a = (reg[18] & 0x1F) << 3;

  /* Window position (0=top, 1=bottom) */
  int w = (reg[18] >> 7) & 1;

  /* Test against current line */
  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Left-most column vertical scrolling when partially shown horizontally */
  /* Same value for both planes, only in 40-cell mode, verified on PAL MD2 */
  /* See Gynoug, Cutie Suzuki no Ringside Angel, Formula One, Kawasaki Superbike Challenge */
  if (reg[12] & 1)
  {
    yscroll = vs[19] & (vs[19] >> 16);
  }

  /* Plane A*/
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A horizontal scroll */
#ifdef LSB_FIRST
    shift = (xscroll & 0x0F);
    index = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift = (xscroll >> 16) & 0x0F;
    index = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4) + shift];

    if(shift)
    {
      /* Left-most column is partially shown */
      dst -= 4;

      /* Plane A vertical scroll */
      v_line = (line + yscroll) & pf_row_mask;

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index-1) & pf_col_mask];
      }

      DRAW_COLUMN(atbuf, v_line)
    }

    for(column = start; column < end; column++, index++)
    {
      /* Plane A vertical scroll */
#ifdef LSB_FIRST
      v_line = (line + vs[column]) & pf_row_mask;
#else
      v_line = (line + (vs[column] >> 16)) & pf_row_mask;
#endif

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }
  else
  {
    /* Window width */
    start = 0;
    end   = width;
  }

  /* Window Plane */
  if (w)
  {
    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4)];

    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = (line & 7) << 3;

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }

  /* Plane B horizontal scroll */
#ifdef LSB_FIRST
  shift = (xscroll >> 16) & 0x0F;
  index = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  shift = (xscroll & 0x0F);
  index = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* Background line buffer */
  lb = &linebuf[0][0x20];

  if(shift)
  {
    /* Left-most column is partially shown */
    lb -= (0x10 - shift);

    /* Plane B vertical scroll */
    v_line = (line + yscroll) & pf_row_mask;

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_BG_COLUMN(atbuf, v_line, xscroll, yscroll)
  }

  for(column = 0; column < width; column++, index++)
  {
    /* Plane B vertical scroll */
#ifdef LSB_FIRST
    v_line = (line + (vs[column] >> 16)) & pf_row_mask;
#else
    v_line = (line + vs[column]) & pf_row_mask;
#endif

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    atbuf = nt[index & pf_col_mask];
    DRAW_BG_COLUMN(atbuf, v_line, xscroll, yscroll)
  }
}

void render_bg_m5_vs_enhanced(int line)
{
  int column, start, end;
  u32 atex, atbuf, *src, *dst;
  u32 shift, index, v_line, next_v_line, *nt;
  u8 *lb;

  /* Vertical scroll offset */
  int v_offset = 0;

  /* Scroll Planes common data */
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = 0;
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;
  u32 *vs          = (u32 *)&vsram[0];

  /* Number of columns to draw */
  int width = viewport.w >> 4;

  /* Layer priority table */
  u8 *table = lut[(reg[12] & 8) >> 2];

  /* Window vertical range (cell 0-31) */
  int a = (reg[18] & 0x1F) << 3;

  /* Window position (0=top, 1=bottom) */
  int w = (reg[18] >> 7) & 1;

  /* Test against current line */
  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Left-most column vertical scrolling when partially shown horizontally */
  /* Same value for both planes, only in 40-cell mode, verified on PAL MD2 */
  /* See Gynoug, Cutie Suzuki no Ringside Angel, Formula One, Kawasaki Superbike Challenge */
  if (reg[12] & 1)
  {
    yscroll = vs[19] & (vs[19] >> 16);
  }

  /* Plane A*/
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A horizontal scroll */
#ifdef LSB_FIRST
    shift = (xscroll & 0x0F);
    index = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift = (xscroll >> 16) & 0x0F;
    index = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4) + shift];

    if(shift)
    {
      /* Left-most column is partially shown */
      dst -= 4;

      /* Plane A vertical scroll */
      v_line = (line + yscroll) & pf_row_mask;

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index-1) & pf_col_mask];
      }

      DRAW_COLUMN(atbuf, v_line)
    }

    for(column = start; column < end; column++, index++)
    {
      /* Plane A vertical scroll */
#ifdef LSB_FIRST
      v_line = (line + vs[column]) & pf_row_mask;
      next_v_line = (line + vs[column + 1]) & pf_row_mask;
#else
      v_line = (line + (vs[column] >> 16)) & pf_row_mask;
      next_v_line = (line + (vs[column + 1] >> 16)) & pf_row_mask;
#endif

      if (column != end - 1)
      {
        v_offset = ((int)next_v_line - (int)v_line) / 2;
        v_offset = (abs(v_offset) >= core_config.enhanced_vscroll_limit) ? 0 : v_offset;
      }

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      atbuf = nt[index & pf_col_mask];
#ifdef LSB_FIRST
      GET_LSB_TILE(atbuf, v_line)
#else
      GET_MSB_TILE(atbuf, v_line)
#endif
#ifdef ALIGN_LONG
      WRITE_LONG(dst, src[0] | atex);
      dst++;
      WRITE_LONG(dst, src[1] | atex);
      dst++;
#else
      *dst++ = (src[0] | atex);
      *dst++ = (src[1] | atex);
#endif

#ifdef LSB_FIRST
      v_line = (line + v_offset + vs[column]) & pf_row_mask;
#else
      v_line = (line + v_offset + (vs[column] >> 16)) & pf_row_mask;
#endif

      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
      v_line = (v_line & 7) << 3;
      atbuf = nt[index & pf_col_mask];

#ifdef LSB_FIRST
      GET_MSB_TILE(atbuf, v_line)
#else
      GET_LSB_TILE(atbuf, v_line)
#endif
#ifdef ALIGN_LONG
      WRITE_LONG(dst, src[0] | atex);
      dst++;
      WRITE_LONG(dst, src[1] | atex);
      dst++;
#else
      *dst++ = (src[0] | atex);
      *dst++ = (src[1] | atex);
#endif
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }
  else
  {
    /* Window width */
    start = 0;
    end   = width;
  }

  /* Window Plane */
  if (w)
  {
    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4)];

    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = (line & 7) << 3;

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }

  /* Plane B horizontal scroll */
#ifdef LSB_FIRST
  shift = (xscroll >> 16) & 0x0F;
  index = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  shift = (xscroll & 0x0F);
  index = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* Background line buffer */
  lb = &linebuf[0][0x20];

  if(shift)
  {
    /* Left-most column is partially shown */
    lb -= (0x10 - shift);

    /* Plane B vertical scroll */
    v_line = (line + yscroll) & pf_row_mask;

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_BG_COLUMN(atbuf, v_line, xscroll, yscroll)
  }

  for(column = 0; column < width; column++, index++)
  {
    /* Plane B vertical scroll */
#ifdef LSB_FIRST
    v_line = (line + (vs[column] >> 16)) & pf_row_mask;
    next_v_line = (line + (vs[column + 1] >> 16)) & pf_row_mask;
#else
    v_line = (line + vs[column]) & pf_row_mask;
    next_v_line = (line + vs[column + 1]) & pf_row_mask;
#endif

    if (column != width - 1)
    {
      v_offset = ((int)next_v_line - (int)v_line) / 2;
      v_offset = (abs(v_offset) >= core_config.enhanced_vscroll_limit) ? 0 : v_offset;
    }
    
    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (v_line & 7) << 3;

    atbuf = nt[index & pf_col_mask];
#ifdef ALIGN_LONG
#ifdef LSB_FIRST
  GET_LSB_TILE(atbuf, v_line)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)

  v_line = (line + v_offset + (vs[column] >> 16)) & pf_row_mask;
  nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
  v_line = (v_line & 7) << 3;
  atbuf = nt[index & pf_col_mask];
  
  GET_MSB_TILE(atbuf, v_line)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
#else
  GET_MSB_TILE(atbuf, v_line)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)

  v_line = (line + vs[column]) & pf_row_mask;
  nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
  v_line = (v_line & 7) << 3;
  atbuf = nt[index & pf_col_mask];
 
  GET_LSB_TILE(atbuf, v_line)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = READ_LONG((u32 *)lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
#endif
#else /* NOT ALIGNED */
#ifdef LSB_FIRST
  GET_LSB_TILE(atbuf, v_line)
  xscroll = *(u32 *)(lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = *(u32 *)(lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)

  v_line = (line + v_offset + (vs[column] >> 16)) & pf_row_mask;
  nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
  v_line = (v_line & 7) << 3;
  atbuf = nt[index & pf_col_mask];

  GET_MSB_TILE(atbuf, v_line)
  xscroll = *(u32 *)(lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = *(u32 *)(lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
#else
  GET_MSB_TILE(atbuf, v_line)
  xscroll = *(u32 *)(lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = *(u32 *)(lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)

  v_line = (line + vs[column]) & pf_row_mask;
  nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
  v_line = (v_line & 7) << 3;
  atbuf = nt[index & pf_col_mask];

  GET_LSB_TILE(atbuf, v_line)
  xscroll = *(u32 *)(lb);
  yscroll = (src[0] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
  xscroll = *(u32 *)(lb);
  yscroll = (src[1] | atex);
  DRAW_BG_TILE(xscroll, yscroll)
#endif
#endif /* ALIGN_LONG */
  }
}

void render_bg_m5_im2(int line)
{
  int column, start, end;
  u32 atex, atbuf, *src, *dst;
  u32 shift, index, v_line, *nt;
  u8 *lb;

  /* Scroll Planes common data */
  int odd = odd_frame;
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = *(u32 *)&vsram[0];
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;

  /* Number of columns to draw */
  int width = viewport.w >> 4;

  /* Layer priority table */
  u8 *table = lut[(reg[12] & 8) >> 2];

  /* Window vertical range (cell 0-31) */
  int a = (reg[18] & 0x1F) << 3;

  /* Window position (0=top, 1=bottom) */
  int w = (reg[18] >> 7) & 1;

  /* Test against current line */
  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A scroll */
#ifdef LSB_FIRST
    shift  = (xscroll & 0x0F);
    index  = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
    v_line = (line + (yscroll >> 1)) & pf_row_mask;
#else
    shift  = (xscroll >> 16) & 0x0F;
    index  = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
    v_line = (line + (yscroll >> 17)) & pf_row_mask;
#endif

    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4) + shift];

    /* Plane A name table */
    nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (((v_line & 7) << 1) | odd) << 3;

    if(shift)
    {
      /* Left-most column is partially shown */
      dst -= 4;

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index-1) & pf_col_mask];
      }

      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    for(column = start; column < end; column++, index++)
    {
      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }
  else
  {
    /* Window width */
    start = 0;
    end   = width;
  }

  /* Window Plane */
  if (w)
  {
    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4)];

    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = ((line & 7) << 1 | odd) << 3;

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }
  }

  /* Plane B scroll */
#ifdef LSB_FIRST
  shift  = (xscroll >> 16) & 0x0F;
  index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
  v_line = (line + (yscroll >> 17)) & pf_row_mask;
#else
  shift  = (xscroll & 0x0F);
  index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
  v_line = (line + (yscroll >> 1)) & pf_row_mask;
#endif

  /* Plane B name table */
  nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

  /* Pattern row index */
  v_line = (((v_line & 7) << 1) | odd) << 3;

  /* Background line buffer */
  lb = &linebuf[0][0x20];

  if(shift)
  {
    /* Left-most column is partially shown */
    lb -= (0x10 - shift);

    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_BG_COLUMN_IM2(atbuf, v_line, xscroll, yscroll)
  }

  for(column = 0; column < width; column++, index++)
  {
    atbuf = nt[index & pf_col_mask];
    DRAW_BG_COLUMN_IM2(atbuf, v_line, xscroll, yscroll)
  }
}

void render_bg_m5_im2_vs(int line)
{
  int column, start, end;
  u32 atex, atbuf, *src, *dst;
  u32 shift, index, v_line, *nt;
  u8 *lb;

  /* common data */
  int odd = odd_frame;
  u32 xscroll      = *(u32 *)&vram[hscb + ((line & hscroll_mask) << 2)];
  u32 yscroll      = 0;
  u32 pf_col_mask  = playfield_col_mask;
  u32 pf_row_mask  = playfield_row_mask;
  u32 pf_shift     = playfield_shift;
  u32 *vs          = (u32 *)&vsram[0];

  /* Number of columns to draw */
  int width = viewport.w >> 4;

  /* Layer priority table */
  u8 *table = lut[(reg[12] & 8) >> 2];

  /* Window vertical range (cell 0-31) */
  u32 a = (reg[18] & 0x1F) << 3;

  /* Window position (0=top, 1=bottom) */
  u32 w = (reg[18] >> 7) & 1;

  /* Test against current line */
  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Left-most column vertical scrolling when partially shown horizontally */
  /* Same value for both planes, only in 40-cell mode, verified on PAL MD2 */
  /* See Gynoug, Cutie Suzuki no Ringside Angel, Formula One, Kawasaki Superbike Challenge */
  if (reg[12] & 1)
  {
    /* only in 40-cell mode, verified on MD2 */
    yscroll = (vs[19] >> 1) & (vs[19] >> 17);
  }

  /* Plane A */
  if (a)
  {
    /* Plane A width */
    start = clip[0].left;
    end   = clip[0].right;

    /* Plane A horizontal scroll */
#ifdef LSB_FIRST
    shift = (xscroll & 0x0F);
    index = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift = (xscroll >> 16) & 0x0F;
    index = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4) + shift];

    if(shift)
    {
      /* Left-most column is partially shown */
      dst -= 4;

      /* Plane A vertical scroll */
      v_line = (line + yscroll) & pf_row_mask;

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (((v_line & 7) << 1) | odd) << 3;

      /* Window bug */
      if (start)
      {
        atbuf = nt[index & pf_col_mask];
      }
      else
      {
        atbuf = nt[(index-1) & pf_col_mask];
      }

      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    for(column = start; column < end; column++, index++)
    {
      /* Plane A vertical scroll */
#ifdef LSB_FIRST
      v_line = (line + (vs[column] >> 1)) & pf_row_mask;
#else
      v_line = (line + (vs[column] >> 17)) & pf_row_mask;
#endif

      /* Plane A name table */
      nt = (u32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];

      /* Pattern row index */
      v_line = (((v_line & 7) << 1) | odd) << 3;

      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    /* Window width */
    start = clip[1].left;
    end   = clip[1].right;
  }
  else
  {
    /* Window width */
    start = 0;
    end   = width;
  }

  /* Window Plane */
  if (w)
  {
    /* Background line buffer */
    dst = (u32 *)&linebuf[0][0x20 + (start << 4)];

    /* Window name table */
    nt = (u32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];

    /* Pattern row index */
    v_line = ((line & 7) << 1 | odd) << 3;

    for(column = start; column < end; column++)
    {
      atbuf = nt[column];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }
  }

  /* Plane B horizontal scroll */
#ifdef LSB_FIRST
  shift = (xscroll >> 16) & 0x0F;
  index = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  shift = (xscroll & 0x0F);
  index = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* Background line buffer */
  lb = &linebuf[0][0x20];

  if(shift)
  {
    /* Left-most column is partially shown */
    lb -= (0x10 - shift);

    /* Plane B vertical scroll */
    v_line = (line + yscroll) & pf_row_mask;

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (((v_line & 7) << 1) | odd) << 3;

    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_BG_COLUMN_IM2(atbuf, v_line, xscroll, yscroll)
  }

  for(column = 0; column < width; column++, index++)
  {
    /* Plane B vertical scroll */
#ifdef LSB_FIRST
    v_line = (line + (vs[column] >> 17)) & pf_row_mask;
#else
    v_line = (line + (vs[column] >> 1)) & pf_row_mask;
#endif

    /* Plane B name table */
    nt = (u32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];

    /* Pattern row index */
    v_line = (((v_line & 7) << 1) | odd) << 3;

    atbuf = nt[index & pf_col_mask];
    DRAW_BG_COLUMN_IM2(atbuf, v_line, xscroll, yscroll)
  }
}
#endif


/*--------------------------------------------------------------------------*/
/* Sprite layer rendering functions                                         */
/*--------------------------------------------------------------------------*/

void render_obj_m4(int line)
{
  int i, xpos, end;
  u8 *src, *lb;
  u16 temp;

  /* Sprite list for current line */
  object_info_t *object_info = obj_info[line];
  int count = object_count[line];

  /* Default sprite width */
  int width = 8;

  /* Sprite Generator address mask (LSB is masked for 8x16 sprites) */
  u16 sg_mask = (~0x1C0 ^ (reg[6] << 6)) & (~((reg[1] & 0x02) >> 1));

  /* Zoomed sprites (not working on Genesis VDP) */
  if (system_hw < SYSTEM_MD)
  {
    width <<= (reg[1] & 0x01);
  }

  /* Unused bits used as a mask on 315-5124 VDP only */
  if (system_hw > SYSTEM_SMS)
  {
    sg_mask |= 0xC0;
  }

  /* Latch SOVR flag from previous line to VDP status */
  status |= spr_ovr;

  /* Clear SOVR flag for current line */
  spr_ovr = 0;

  /* Draw sprites in front-to-back order */
  while (count--)
  {
    /* Sprite pattern index */
    temp = (object_info->attr | 0x100) & sg_mask;

    /* Pointer to pattern cache line */
    src = (u8 *)&bg_pattern_cache[(temp << 6) | (object_info->ypos << 3)];

    /* Sprite X position */
    xpos = object_info->xpos;

    /* X position shift */
    xpos -= (reg[0] & 0x08);

    if (xpos < 0)
    {
      /* Clip sprites on left edge */
      src = src - xpos;
      end = xpos + width;
      xpos = 0;
    }
    else if ((xpos + width) > 256)
    {
      /* Clip sprites on right edge */
      end = 256 - xpos;
    }
    else
    {
      /* Sprite maximal width */
      end = width;
    }

    /* Pointer to line buffer */
    lb = &linebuf[0][0x20 + xpos];

    if (width > 8)
    {
      /* Draw sprite pattern (zoomed sprites are rendered at half speed) */
      DRAW_SPRITE_TILE_ACCURATE_2X(end,0,lut[5])

      /* 315-5124 VDP specific */
      if (system_hw < SYSTEM_SMS2)
      {
        /* only 4 first sprites can be zoomed */
        if (count == (object_count[line] - 4))
        {
          /* Set default width for remaining sprites */
          width = 8;
        }
      }
    }
    else
    {
      /* Draw sprite pattern */
      DRAW_SPRITE_TILE_ACCURATE(end,0,lut[5])
    }

    /* Next sprite entry */
    object_info++;
  }

  /* handle Game Gear reduced screen (160x144) */
  if ((system_hw == SYSTEM_GG) && !core_config.gg_extra && (v_counter < viewport.h))
  {
    int line = v_counter - (viewport.h - 144) / 2;
    if ((line < 0) || (line >= 144))
    {
      xee::mem::Memset(&linebuf[0][0x20], 0x40, 256);
    }
    else
    {
      if (viewport.x > 0)
      {
        xee::mem::Memset(&linebuf[0][0x20], 0x40, 48);
        xee::mem::Memset(&linebuf[0][0x20+48+160], 0x40, 48);
      }
    }
  }
}

void render_obj_m5(int line)
{
  int i, column;
  int xpos, width;
  int pixelcount = 0;
  int masked = 0;
  int max_pixels = MODE5_MAX_SPRITE_PIXELS;

  u8 *src, *s, *lb;
  u32 temp, v_line;
  u32 attr, name, atex;

  /* Sprite list for current line */
  object_info_t *object_info = obj_info[line];
  int count = object_count[line];

  /* Draw sprites in front-to-back order */
  while (count--)
  {
    /* Sprite X position */
    xpos = object_info->xpos;

    /* Sprite masking  */
    if (xpos)
    {
      /* Requires at least one sprite with xpos > 0 */
      spr_ovr = 1;
    }
    else if (spr_ovr)
    {
      /* Remaining sprites are not drawn */
      masked = 1;
    }

    /* Display area offset */
    xpos = xpos - 0x80;

    /* Sprite size */
    temp = object_info->size;

    /* Sprite width */
    width = 8 + ((temp & 0x0C) << 1);

    /* Update pixel count (off-screen sprites are included) */
    pixelcount += width;

    /* Is sprite across visible area ? */
    if (((xpos + width) > 0) && (xpos < viewport.w) && !masked)
    {
      /* Sprite attributes */
      attr = object_info->attr;

      /* Sprite vertical offset */
      v_line = object_info->ypos;

      /* Sprite priority + palette bits */
      atex = (attr >> 9) & 0x70;

      /* Pattern name base */
      name = attr & 0x07FF;

      /* Mask vflip/hflip */
      attr &= 0x1800;

      /* Pointer into pattern name offset look-up table */
      s = &name_lut[((attr >> 3) & 0x300) | (temp << 4) | ((v_line & 0x18) >> 1)];

      /* Pointer into line buffer */
      lb = &linebuf[0][0x20 + xpos];

      /* Max. number of sprite pixels rendered per line */
      if (pixelcount > max_pixels)
      {
        /* Adjust number of pixels to draw */
        width -= (pixelcount - max_pixels);
      }

      /* Number of tiles to draw */
      width = width >> 3;

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      /* Draw sprite patterns */
      for (column = 0; column < width; column++, lb+=8)
      {
        temp = attr | ((name + s[column]) & 0x07FF);
        src = &bg_pattern_cache[(temp << 6) | (v_line)];
        DRAW_SPRITE_TILE(8,atex,lut[1])
      }
    }

    /* Sprite limit */
    if (pixelcount >= max_pixels)
    {
      /* Sprite masking is effective on next line if max pixel width is reached */
      spr_ovr = (pixelcount >= viewport.w);

      /* Stop sprite rendering */
      return;
    }

    /* Next sprite entry */
    object_info++;
  }

  /* Clear sprite masking for next line  */
  spr_ovr = 0;
}

void render_obj_m5_ste(int line)
{
  int i, column;
  int xpos, width;
  int pixelcount = 0;
  int masked = 0;
  int max_pixels = MODE5_MAX_SPRITE_PIXELS;

  u8 *src, *s, *lb;
  u32 temp, v_line;
  u32 attr, name, atex;

  /* Sprite list for current line */
  object_info_t *object_info = obj_info[line];
  int count = object_count[line];

  /* Clear sprite line buffer */
  xee::mem::Memset(&linebuf[1][0], 0, viewport.w + 0x40);

  /* Draw sprites in front-to-back order */
  while (count--)
  {
    /* Sprite X position */
    xpos = object_info->xpos;

    /* Sprite masking  */
    if (xpos)
    {
      /* Requires at least one sprite with xpos > 0 */
      spr_ovr = 1;
    }
    else if (spr_ovr)
    {
      /* Remaining sprites are not drawn */
      masked = 1;
    }

    /* Display area offset */
    xpos = xpos - 0x80;

    /* Sprite size */
    temp = object_info->size;

    /* Sprite width */
    width = 8 + ((temp & 0x0C) << 1);

    /* Update pixel count (off-screen sprites are included) */
    pixelcount += width;

    /* Is sprite across visible area ? */
    if (((xpos + width) > 0) && (xpos < viewport.w) && !masked)
    {
      /* Sprite attributes */
      attr = object_info->attr;

      /* Sprite vertical offset */
      v_line = object_info->ypos;

      /* Sprite priority + palette bits */
      atex = (attr >> 9) & 0x70;

      /* Pattern name base */
      name = attr & 0x07FF;

      /* Mask vflip/hflip */
      attr &= 0x1800;

      /* Pointer into pattern name offset look-up table */
      s = &name_lut[((attr >> 3) & 0x300) | (temp << 4) | ((v_line & 0x18) >> 1)];

      /* Pointer into line buffer */
      lb = &linebuf[1][0x20 + xpos];

      /* Adjust number of pixels to draw for sprite limit */
      if (pixelcount > max_pixels)
      {
        width -= (pixelcount - max_pixels);
      }

      /* Number of tiles to draw */
      width = width >> 3;

      /* Pattern row index */
      v_line = (v_line & 7) << 3;

      /* Draw sprite patterns */
      for (column = 0; column < width; column++, lb+=8)
      {
        temp = attr | ((name + s[column]) & 0x07FF);
        src = &bg_pattern_cache[(temp << 6) | (v_line)];
        DRAW_SPRITE_TILE(8,atex,lut[3])
      }
    }

    /* Sprite limit */
    if (pixelcount >= max_pixels)
    {
      /* Sprite masking is effective on next line if max pixel width is reached */
      spr_ovr = (pixelcount >= viewport.w);

      /* Merge background & sprite layers */
      merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[4], viewport.w);

      /* Stop sprite rendering */
      return;
    }

    /* Next sprite entry */
    object_info++;
  }

  /* Clear sprite masking for next line  */
  spr_ovr = 0;

  /* Merge background & sprite layers */
  merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[4], viewport.w);
}

void render_obj_m5_im2(int line)
{
  int i, column;
  int xpos, width;
  int pixelcount = 0;
  int masked = 0;
  int odd = odd_frame;
  int max_pixels = MODE5_MAX_SPRITE_PIXELS;

  u8 *src, *s, *lb;
  u32 temp, v_line;
  u32 attr, name, atex;

  /* Sprite list for current line */
  object_info_t *object_info = obj_info[line];
  int count = object_count[line];

  /* Draw sprites in front-to-back order */
  while (count--)
  {
    /* Sprite X position */
    xpos = object_info->xpos;

    /* Sprite masking  */
    if (xpos)
    {
      /* Requires at least one sprite with xpos > 0 */
      spr_ovr = 1;
    }
    else if (spr_ovr)
    {
      /* Remaining sprites are not drawn */
      masked = 1;
    }

    /* Display area offset */
    xpos = xpos - 0x80;

    /* Sprite size */
    temp = object_info->size;

    /* Sprite width */
    width = 8 + ((temp & 0x0C) << 1);

    /* Update pixel count (off-screen sprites are included) */
    pixelcount += width;

    /* Is sprite across visible area ? */
    if (((xpos + width) > 0) && (xpos < viewport.w) && !masked)
    {
      /* Sprite attributes */
      attr = object_info->attr;

      /* Sprite y offset */
      v_line = object_info->ypos;

      /* Sprite priority + palette bits */
      atex = (attr >> 9) & 0x70;

      /* Pattern name base */
      name = attr & 0x03FF;

      /* Mask vflip/hflip */
      attr &= 0x1800;

      /* Pattern name offset lookup table */
      s = &name_lut[((attr >> 3) & 0x300) | (temp << 4) | ((v_line & 0x18) >> 1)];

      /* Pointer into line buffer */
      lb = &linebuf[0][0x20 + xpos];

      /* Adjust width for sprite limit */
      if (pixelcount > max_pixels)
      {
        width -= (pixelcount - max_pixels);
      }

      /* Number of tiles to draw */
      width = width >> 3;

      /* Pattern row index */
      v_line = (((v_line & 7) << 1) | odd) << 3;

      /* Render sprite patterns */
      for(column = 0; column < width; column ++, lb+=8)
      {
        temp = attr | (((name + s[column]) & 0x3ff) << 1);
        src = &bg_pattern_cache[((temp << 6) | (v_line)) ^ ((attr & 0x1000) >> 6)];
        DRAW_SPRITE_TILE(8,atex,lut[1])
      }
    }

    /* Sprite Limit */
    if (pixelcount >= max_pixels)
    {
      /* Sprite masking is effective on next line if max pixel width is reached */
      spr_ovr = (pixelcount >= viewport.w);

      /* Stop sprite rendering */
      return;
    }

    /* Next sprite entry */
    object_info++;
  }

  /* Clear sprite masking for next line */
  spr_ovr = 0;
}

void render_obj_m5_im2_ste(int line)
{
  int i, column;
  int xpos, width;
  int pixelcount = 0;
  int masked = 0;
  int odd = odd_frame;
  int max_pixels = MODE5_MAX_SPRITE_PIXELS;

  u8 *src, *s, *lb;
  u32 temp, v_line;
  u32 attr, name, atex;

  /* Sprite list for current line */
  object_info_t *object_info = obj_info[line];
  int count = object_count[line];

  /* Clear sprite line buffer */
  xee::mem::Memset(&linebuf[1][0], 0, viewport.w + 0x40);

  /* Draw sprites in front-to-back order */
  while (count--)
  {
    /* Sprite X position */
    xpos = object_info->xpos;

    /* Sprite masking  */
    if (xpos)
    {
      /* Requires at least one sprite with xpos > 0 */
      spr_ovr = 1;
    }
    else if (spr_ovr)
    {
      /* Remaining sprites are not drawn */
      masked = 1;
    }

    /* Display area offset */
    xpos = xpos - 0x80;

    /* Sprite size */
    temp = object_info->size;

    /* Sprite width */
    width = 8 + ((temp & 0x0C) << 1);

    /* Update pixel count (off-screen sprites are included) */
    pixelcount += width;

    /* Is sprite across visible area ? */
    if (((xpos + width) > 0) && (xpos < viewport.w) && !masked)
    {
      /* Sprite attributes */
      attr = object_info->attr;

      /* Sprite y offset */
      v_line = object_info->ypos;

      /* Sprite priority + palette bits */
      atex = (attr >> 9) & 0x70;

      /* Pattern name base */
      name = attr & 0x03FF;

      /* Mask vflip/hflip */
      attr &= 0x1800;

      /* Pattern name offset lookup table */
      s = &name_lut[((attr >> 3) & 0x300) | (temp << 4) | ((v_line & 0x18) >> 1)];

      /* Pointer into line buffer */
      lb = &linebuf[1][0x20 + xpos];

      /* Adjust width for sprite limit */
      if (pixelcount > max_pixels)
      {
        width -= (pixelcount - max_pixels);
      }

      /* Number of tiles to draw */
      width = width >> 3;

      /* Pattern row index */
      v_line = (((v_line & 7) << 1) | odd) << 3;

      /* Render sprite patterns */
      for(column = 0; column < width; column ++, lb+=8)
      {
        temp = attr | (((name + s[column]) & 0x3ff) << 1);
        src = &bg_pattern_cache[((temp << 6) | (v_line)) ^ ((attr & 0x1000) >> 6)];
        DRAW_SPRITE_TILE(8,atex,lut[3])
      }
    }

    /* Sprite Limit */
    if (pixelcount >= max_pixels)
    {
      /* Sprite masking is effective on next line if max pixel width is reached */
      spr_ovr = (pixelcount >= viewport.w);

      /* Merge background & sprite layers */
      merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[4], viewport.w);

      /* Stop sprite rendering */
      return;
    }

    /* Next sprite entry */
    object_info++;
  }

  /* Clear sprite masking for next line */
  spr_ovr = 0;

  /* Merge background & sprite layers */
  merge(&linebuf[1][0x20], &linebuf[0][0x20], &linebuf[0][0x20], lut[4], viewport.w);
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
    g_sprite_layer_renderer_m4 = new gpgx::ppu::vdp::M4SpriteLayerRenderer();
  }

  // Initialize renderer of sprite layer in mode 5.
  if (!g_sprite_layer_renderer_m5) {
    g_sprite_layer_renderer_m5 = new gpgx::ppu::vdp::M5SpriteLayerRenderer();
  }

  // Initialize renderer of sprite layer in mode 5 (STE).
  if (!g_sprite_layer_renderer_m5_ste) {
    g_sprite_layer_renderer_m5_ste = new gpgx::ppu::vdp::M5SteSpriteLayerRenderer();
  }

  // Initialize renderer of sprite layer in mode 5 (IM2).
  if (!g_sprite_layer_renderer_m5_im2) {
    g_sprite_layer_renderer_m5_im2 = new gpgx::ppu::vdp::M5Im2SpriteLayerRenderer();
  }

  // Initialize renderer of sprite layer in mode 5 (IM2/STE).
  if (!g_sprite_layer_renderer_m5_im2_ste) {
    g_sprite_layer_renderer_m5_im2_ste = new gpgx::ppu::vdp::M5Im2SteSpriteLayerRenderer();
  }
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
    render_bg(line);

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

  /* Adjust for interlaced output */
  if (interlaced && core_config.render)
  {
    line = (line * 2) + odd_frame;
  }

  {
#ifdef CUSTOM_BLITTER
    CUSTOM_BLITTER(line, width, pixel, src)
#else
    /* Convert VDP pixel data to output pixel format */
    PIXEL_OUT_T *dst = ((PIXEL_OUT_T *)&framebuffer.data[(line * framebuffer.pitch)]);
    if (core_config.lcd)
    {
      do
      {
        RENDER_PIXEL_LCD(src,dst,pixel, core_config.lcd);
      }
      while (--width);
    }
    else
    {
      do
      {
        *dst++ = pixel[*src++];
      }
      while (--width);
    }
 #endif
  }
}
