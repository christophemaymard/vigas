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

#ifndef __CORE_CD_HW_CD_HW_T_H__
#define __CORE_CD_HW_CD_HW_T_H__

#include "xee/fnd/data_type.h"

#include "core/types.h"
#include "core/cd_hw/cd_cart_t.h"
#include "core/cd_hw/cdd.h"
#include "core/cd_hw/cdc_t.h"
#include "core/cd_hw/gfx_t.h"
#include "core/cd_hw/pcm.h"

//==============================================================================

//------------------------------------------------------------------------------

// CD hardware.
struct cd_hw_t
{
  cd_cart_t cartridge;      // ROM/RAM Cartridge.
  u8 bootrom[0x20000];      // 128K internal BOOT ROM.
  u8 prg_ram[0x80000];      // 512K PRG-RAM.
  u8 word_ram[2][0x20000];  // 2 x 128K Word RAM (1M mode).
  u8 word_ram_2M[0x40000];  // 256K Word RAM (2M mode).
  u8 bram[0x2000];          // 8K Backup RAM.
  reg16_t regs[0x100];      // 256 x 16-bit ASIC registers.
  u32 cycles;               // CD Master clock counter.
  u32 cycles_per_line;      // CD Master clock count per scanline.
  s32 stopwatch;            // Stopwatch counter.
  s32 timer;                // Timer counter.
  u8 pending;               // Pending interrupts.
  u8 dmna;                  // Pending DMNA write status.
  u8 type;                  // CD hardware model.
  gfx_t gfx_hw;             // Graphics processor.
  cdc_t cdc_hw;             // CD data controller.
  cdd_t cdd_hw;             // CD drive processor.
  pcm_t pcm_hw;             // PCM chip.
};

#endif // #ifndef __CORE_CD_HW_CD_HW_T_H__

