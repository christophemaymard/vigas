/***************************************************************************************
 *  Genesis Plus GX
 *  Video Display Processor (output pixel)
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

#ifndef __CORE_VDP_PIXEL_H__
#define __CORE_VDP_PIXEL_H__

#include "xee/fnd/data_type.h"

//==============================================================================

//------------------------------------------------------------------------------
//  Output pixels type.

#if defined(USE_8BPP_RENDERING)
/// 8BPP.
#define PIXEL_OUT_T u8
#elif defined(USE_15BPP_RENDERING)
/// 15BPP.
#define PIXEL_OUT_T u16
#elif defined(USE_16BPP_RENDERING)
/// 16BPP.
#define PIXEL_OUT_T u16
#elif defined(USE_32BPP_RENDERING)
/// 32BPP.
#define PIXEL_OUT_T u32
#endif

//------------------------------------------------------------------------------
// Pixels conversion macro.

// 4-bit color channels are either compressed to 2/3-bit or dithered to 5/6/8-bit equivalents.

#if defined(USE_8BPP_RENDERING)
/// 3:3:2 RGB.
#define MAKE_PIXEL(r,g,b)  (((r) >> 1) << 5 | ((g) >> 1) << 2 | (b) >> 2)

#elif defined(USE_15BPP_RENDERING)

#if defined(USE_ABGR)
/// 5:5:5 RGB (ABGR).
#define MAKE_PIXEL(r,g,b) ((1 << 15) | (b) << 11 | ((b) >> 3) << 10 | (g) << 6 | ((g) >> 3) << 5 | (r) << 1 | (r) >> 3)
#else
/// 5:5:5 RGB.
#define MAKE_PIXEL(r,g,b) ((1 << 15) | (r) << 11 | ((r) >> 3) << 10 | (g) << 6 | ((g) >> 3) << 5 | (b) << 1 | (b) >> 3)
#endif // #if defined(USE_ABGR)

#elif defined(USE_16BPP_RENDERING)
/// 5:6:5 RGB.
#define MAKE_PIXEL(r,g,b) ((r) << 12 | ((r) >> 3) << 11 | (g) << 7 | ((g) >> 2) << 5 | (b) << 1 | (b) >> 3)

#elif defined(USE_32BPP_RENDERING)
/// 8:8:8 RGB.
#define MAKE_PIXEL(r,g,b) ((0xff << 24) | (r) << 20 | (r) << 16 | (g) << 12 | (g)  << 8 | (b) << 4 | (b))

#endif

#endif // #ifndef __CORE_VDP_PIXEL_H__

