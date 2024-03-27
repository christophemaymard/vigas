/***************************************************************************************
 *  Genesis Plus
 *  Mega-CD / Sega CD hardware
 *
 *  Copyright (C) 2012-2024  Eke-Eke (Genesis Plus GX)
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
#ifndef _HW_SCD_
#define _HW_SCD_

#include "xee/fnd/data_type.h"

#define scd ext.cd_hw

/* CD hardware models */
#define CD_TYPE_DEFAULT       0x00
#define CD_TYPE_WONDERMEGA    0x01
#define CD_TYPE_WONDERMEGA_M2 0x02
#define CD_TYPE_CDX           0x03

/* CD hardware Master Clock (50 MHz) */
#define SCD_CLOCK 50000000

/* ~3184 SCD clocks/line on NTSC system (53.693175 MHz Master Clock) */
/* ~3214 SCD clocks/line on PAL system (53.203424 MHz Master Clock) */
#define SCYCLES_PER_LINE scd.cycles_per_line

/* Timer & Stopwatch clocks divider */
#define TIMERS_SCYCLES_RATIO (384 * 4)

/* Function prototypes */
extern void scd_init(void);
extern void scd_reset(int hard);
extern void scd_update(unsigned int cycles);
extern void scd_end_frame(unsigned int cycles);
extern int scd_context_load(u8 *state, char *version);
extern int scd_context_save(u8 *state);
extern int scd_68k_irq_ack(int level);
extern void prg_ram_dma_w(unsigned int length);

#endif
