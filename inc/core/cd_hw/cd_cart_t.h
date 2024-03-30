/***************************************************************************************
 *  Genesis Plus
 *  CD compatible ROM/RAM cartridge support
 *
 *  Copyright (C) 2012-2021 Eke-Eke (Genesis Plus GX)
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

#ifndef __CORE_CD_HW_CD_CART_T_H__
#define __CORE_CD_HW_CD_CART_T_H__

#include "xee/fnd/data_type.h"

//==============================================================================

//------------------------------------------------------------------------------

// CD compatible ROM/RAM cartridge.
struct cd_cart_t
{
  u8 reserved[0x80];  // reserved for ROM cartridge infos (see md_cart.h).
  u8 area[0x810000];  // cartridge ROM/RAM area (max. 8MB ROM + Pro Action Replay 64KB ROM).
  u8 boot;            // cartridge boot mode (0x00: boot from CD with ROM/RAM cartridge enabled, 0x40: boot from ROM cartridge with CD enabled).
  u8 id;              // RAM cartridge ID (related to RAM size, 0 if disabled).
  u8 prot;            // RAM cartridge write protection.
  u32 mask;           // RAM cartridge size mask.
};

#endif // #ifndef __CORE_CD_HW_CD_CART_T_H__
