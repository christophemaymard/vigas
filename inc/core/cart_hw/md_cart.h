/****************************************************************************
 *  Genesis Plus
 *  Mega Drive cartridge hardware support
 *
 *  Copyright (C) 2007-2023  Eke-Eke (Genesis Plus GX)
 *
 *  Most cartridge protections were initially documented by Haze
 *  (http://haze.mameworld.info/)
 *
 *  Realtec mapper was documented by TascoDeluxe
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

#ifndef _MD_CART_H_
#define _MD_CART_H_

#include "xee/fnd/data_type.h"

#define cart ext.md_cart

/* Lock-On cartridge type */
#define TYPE_GG 0x01  /* Game Genie */
#define TYPE_AR 0x02  /* (Pro) Action Replay */
#define TYPE_SK 0x03  /* Sonic & Knuckles */

/* CD hardware add-on (MD mode) */
#define HW_ADDON_AUTO    0x00
#define HW_ADDON_MEGACD  0x01
#define HW_ADDON_MEGASD  0x02
#define HW_ADDON_NONE    0x03

/* Special hardware (0x01 & 0x02 reserved for Master System 3-D glasses & Terebi Oekaki) */
#define HW_J_CART   0x04
#define HW_LOCK_ON  0x08
#define HW_MEGASD   0x10

/* Function prototypes */
extern void md_cart_init(void);
extern void md_cart_reset(int hard_reset);
extern int md_cart_context_save(u8 *state);
extern int md_cart_context_load(u8 *state);

#endif
