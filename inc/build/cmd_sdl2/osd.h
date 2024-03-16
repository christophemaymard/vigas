/***************************************************************************************
 *  Genesis Plus
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

#ifndef _OSD_H_
#define _OSD_H_

#include "build/cmd_sdl2/main.h"
#include "build/cmd_sdl2/config.h"
#include "build/cmd_sdl2/error.h"
#include "build/cmd_sdl2/fileio.h"

#define osd_input_update sdl_input_update

#define GG_ROM      "./ggenie.bin"
#define AR_ROM      "./areplay.bin"
#define SK_ROM      "./sk.bin"
#define SK_UPMEM    "./sk2chip.bin"
#define CD_BIOS_US  "./bios_CD_U.bin"
#define CD_BIOS_EU  "./bios_CD_E.bin"
#define CD_BIOS_JP  "./bios_CD_J.bin"
#define MD_BIOS     "./bios_MD.bin"
#define MS_BIOS_US  "./bios_U.sms"
#define MS_BIOS_EU  "./bios_E.sms"
#define MS_BIOS_JP  "./bios_J.sms"
#define GG_BIOS     "./bios.gg"

unsigned int crc32(unsigned int crc, const unsigned char* buffer, unsigned int len);

#endif /* _OSD_H_ */
