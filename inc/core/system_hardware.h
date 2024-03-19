/***************************************************************************************
 *  Genesis Plus
 *  Virtual System emulation
 *
 *  Support for 16-bit & 8-bit hardware modes
 *
 *  Copyright (C) 1998-2003  Charles Mac Donald (original code)
 *  Copyright (C) 2007-2024  Eke-Eke (Genesis Plus GX)
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

#ifndef __CORE_SYSTEM_HARDWARE_H__
#define __CORE_SYSTEM_HARDWARE_H__

#include "xee/fnd/data_type.h"

//==============================================================================

//------------------------------------------------------------------------------
// Supported hardware models.

#define SYSTEM_SG           0x01
#define SYSTEM_SGII         0x02
#define SYSTEM_SGII_RAM_EXT 0x03
#define SYSTEM_MARKIII      0x10
#define SYSTEM_SMS          0x20
#define SYSTEM_SMS2         0x21
#define SYSTEM_GG           0x40
#define SYSTEM_GGMS         0x41
#define SYSTEM_MD           0x80
#define SYSTEM_PBC          0x81
#define SYSTEM_PICO         0x82
#define SYSTEM_MCD          0x84

//------------------------------------------------------------------------------

extern u8 system_hw;

#endif // #ifndef __CORE_SYSTEM_HARDWARE_H__