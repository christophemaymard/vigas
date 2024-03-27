/****************************************************************************
 *  Genesis Plus
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

#ifndef __CORE_CART_HW_CART_HW_T_H__
#define __CORE_CART_HW_CART_HW_T_H__

#include "xee/fnd/data_type.h"

//==============================================================================

//------------------------------------------------------------------------------

// Cartridge extra hardware.
struct cart_hw_t
{
  u8 regs[4];     // internal registers (R/W).
  u32 mask[4];    // registers address mask.
  u32 addr[4];    // registers address.
  u16 realtec;    // realtec mapper.
  u16 bankshift;  // cartridge with bankshift mecanism reseted on software reset.
  unsigned int (*time_r)(unsigned int address);             // !TIME signal ($a130xx) read handler.
  void (*time_w)(unsigned int address, unsigned int data);  // !TIME signal ($a130xx) write handler.
  unsigned int (*regs_r)(unsigned int address);             // cart hardware registers read handler.
  void (*regs_w)(unsigned int address, unsigned int data);  // cart hardware registers write handler.
};

#endif // #ifndef __CORE_CART_HW_CART_HW_T_H__

