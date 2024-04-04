/***************************************************************************************
 *  Genesis Plus
 *  Internal Hardware & Bus controllers
 *
 *  Support for SG-1000, Mark-III, Master System, Game Gear, Mega Drive & Mega CD hardware
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

#include "core/genesis.h"

#include <stdlib.h>

#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "gpgx/g_fm_synthesizer.h"
#include "gpgx/g_z80.h"

#include "core/boot_rom.h"
#include "core/core_config.h"
#include "core/m68k/m68k.h"
#include "core/pico_current.h"
#include "core/region_code.h"
#include "core/system_bios.h"
#include "core/system_hardware.h"
#include "core/system_timing.h"
#include "core/ext.h" // For scd, cart.
#include "core/vdp_ctrl.h"
#include "core/work_ram.h"
#include "core/zbank.h"
#include "core/zbank_memory_map.h"
#include "core/zram.h"
#include "core/zstate.h"
#include "core/mem68k.h"
#include "core/memz80.h"
#include "core/membnk.h"

#include "core/cart_hw/md_cart.h" // Fo md_cart_init() and md_cart_reset().
#include "core/cart_hw/sms_cart.h" // For sms_cart_init() and sms_cart_reset().
#include "core/cd_hw/scd.h" // For scd_init() and scd_reset().

static u8 tmss[4];     /* TMSS security register */

/*--------------------------------------------------------------------------*/
/* Init, reset, shutdown functions                                          */
/*--------------------------------------------------------------------------*/

void gen_init(void)
{
  int i;

  /* initialize Z80 */
  gpgx::g_z80->Init(z80_irq_callback);

  /* 8-bit / 16-bit modes */
  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
  {
    /* initialize main 68k */
    m68k_init();
    m68k.aerr_enabled = core_config.addr_error;

    /* initialize main 68k memory map */

    /* $800000-$DFFFFF : illegal access by default */
    for (i=0x80; i<0xe0; i++)
    {
      m68k.memory_map[i].base     = work_ram; /* for VDP DMA */
      m68k.memory_map[i].read8    = m68k_lockup_r_8;
      m68k.memory_map[i].read16   = m68k_lockup_r_16;
      m68k.memory_map[i].write8   = m68k_lockup_w_8;
      m68k.memory_map[i].write16  = m68k_lockup_w_16;
      zbank_memory_map[i].read    = zbank_lockup_r;
      zbank_memory_map[i].write   = zbank_lockup_w;
    }

    /* $C0xxxx, $C8xxxx, $D0xxxx, $D8xxxx : VDP ports */
    for (i=0xc0; i<0xe0; i+=8)
    {
      m68k.memory_map[i].read8    = vdp_read_byte;
      m68k.memory_map[i].read16   = vdp_read_word;
      m68k.memory_map[i].write8   = vdp_write_byte;
      m68k.memory_map[i].write16  = vdp_write_word;
      zbank_memory_map[i].read    = zbank_read_vdp;
      zbank_memory_map[i].write   = zbank_write_vdp;
    }

    /* $E00000-$FFFFFF : Work RAM (64k) */
    for (i=0xe0; i<0x100; i++)
    {
      m68k.memory_map[i].base     = work_ram;
      m68k.memory_map[i].read8    = NULL;
      m68k.memory_map[i].read16   = NULL;
      m68k.memory_map[i].write8   = NULL;
      m68k.memory_map[i].write16  = NULL;

      /* Z80 can ONLY write to 68k RAM, not read it */
      zbank_memory_map[i].read    = zbank_unused_r; 
      zbank_memory_map[i].write   = NULL;
    }

    if (system_hw == SYSTEM_PICO)
    {
      /* additional registers mapped to $800000-$80FFFF */
      m68k.memory_map[0x80].read8   = pico_read_byte;
      m68k.memory_map[0x80].read16  = pico_read_word;
      m68k.memory_map[0x80].write8  = m68k_unused_8_w;
      m68k.memory_map[0x80].write16 = m68k_unused_16_w;

      /* there is no I/O area (Notaz) */
      m68k.memory_map[0xa1].read8   = m68k_read_bus_8;
      m68k.memory_map[0xa1].read16  = m68k_read_bus_16;
      m68k.memory_map[0xa1].write8  = m68k_unused_8_w;
      m68k.memory_map[0xa1].write16 = m68k_unused_16_w;

      /* initialize page index (closed) */
      pico_current = 0;
    }
    else
    {
      /* $A10000-$A1FFFF : I/O & Control registers */
      m68k.memory_map[0xa1].read8   = ctrl_io_read_byte;
      m68k.memory_map[0xa1].read16  = ctrl_io_read_word;
      m68k.memory_map[0xa1].write8  = ctrl_io_write_byte;
      m68k.memory_map[0xa1].write16 = ctrl_io_write_word;
      zbank_memory_map[0xa1].read   = zbank_read_ctrl_io;
      zbank_memory_map[0xa1].write  = zbank_write_ctrl_io;

      /* initialize Z80 memory map */
      /* $0000-$3FFF is mapped to Z80 RAM (8K mirrored) */
      /* $4000-$FFFF is mapped to hardware but Z80 PC should never point there */
      for (i=0; i<64; i++)
      {
        gpgx::g_z80->SetReadMemoryMapBase(i, &zram[(i & 7) << 10]);
      }

      /* initialize Z80 memory handlers */
      gpgx::g_z80->SetMemoryHandlers(z80_memory_r, z80_memory_w);

      /* initialize Z80 port handlers */
      gpgx::g_z80->SetPortHandlers(z80_unused_port_r, z80_unused_port_w);
    }

    /* $000000-$7FFFFF : external hardware area */
    if (system_hw == SYSTEM_MCD)
    {
      /* initialize SUB-CPU */
      s68k_init();
      s68k.aerr_enabled = core_config.addr_error;

      /* initialize CD hardware */
      scd_init();
    }
    else
    {
      /* Cartridge hardware */
      md_cart_init();
    }
  }
  else
  {
    /* initialize cartridge hardware & Z80 memory handlers */
    sms_cart_init();

    /* initialize Z80 ports handlers */
    switch (system_hw)
    {
      /* Master System compatibility mode */
      case SYSTEM_PBC:
      {
        gpgx::g_z80->SetPortHandlers(z80_md_port_r, z80_md_port_w);
        break;
      }

      /* Game Gear hardware */
      case SYSTEM_GG:
      case SYSTEM_GGMS:
      {
        /* initialize cartridge hardware & Z80 memory handlers */
        sms_cart_init();

        /* initialize Z80 ports handlers */
        gpgx::g_z80->SetPortHandlers(z80_gg_port_r, z80_gg_port_w);
        break;
      }

      /* Master System hardware */
      case SYSTEM_SMS:
      case SYSTEM_SMS2:
      {
        gpgx::g_z80->SetPortHandlers(z80_ms_port_r, z80_ms_port_w);
        break;
      }

      /* Mark-III hardware */
      case SYSTEM_MARKIII:
      {
        gpgx::g_z80->SetPortHandlers(z80_m3_port_r, z80_m3_port_w);
        break;
      }

      /* SG-1000 hardware */
      case SYSTEM_SG:
      case SYSTEM_SGII:
      case SYSTEM_SGII_RAM_EXT:
      {
        gpgx::g_z80->SetPortHandlers(z80_sg_port_r, z80_sg_port_w);
        break;
      }
    }
  }
}

void gen_reset(int hard_reset)
{
  /* System Reset */
  if (hard_reset)
  {
    /* On hard reset, 68k CPU always starts at the same point in VDP frame */
    /* Tests performed on VA4 PAL MD1 showed that the first HVC value read */
    /* with 'move.w #0x8104,0xC00004' , 'move.w 0xC00008,%d0' sequence was */
    /* 0x9F21 in 60HZ mode (0x9F00 if Mode 5 is not enabled by first MOVE) */
    /* 0x8421 in 50HZ mode (0x8400 if Mode 5 is not enabled by first MOVE) */
    /* Same value is returned on every power ON, indicating VDP is always  */
    /* starting at the same fixed point in frame (probably at the start of */
    /* VSYNC and HSYNC) while 68k /VRES line remains asserted a fixed time */
    /* after /SRES line has been released (13 msec approx). The difference */
    /* between PAL & NTSC is caused by the top border area being 27 lines  */
    /* larger in PAL mode than in NTSC mode. CPU cycle counter is adjusted */
    /* to match these results (taking in account emulated frame is started */
    /* on line 192 */
    m68k.cycles = ((lines_per_frame - 192 + 159 - (27 * vdp_pal)) * MCYCLES_PER_LINE) + 1004;

    /* clear RAM (on real hardware, RAM values are random / undetermined on Power ON) */
    xee::mem::Memset(work_ram, 0x00, sizeof (work_ram));
    xee::mem::Memset(zram, 0x00, sizeof (zram));
  }
  else
  {
    /* when RESET button is pressed, 68k could be anywhere in VDP frame (Bonkers, Eternal Champions, X-Men 2) */
    m68k.cycles = (u32)((MCYCLES_PER_LINE * lines_per_frame) * ((f64)rand() / (f64)RAND_MAX));

    /* reset YM2612 (on hard reset, this is done by sound_reset) */
    gpgx::g_fm_synthesizer->SyncAndReset(0);
  }

  /* 68k M-cycles should be a multiple of 7 */
  m68k.cycles = (m68k.cycles / 7) * 7;

  /* Z80 M-cycles should be a multiple of 15 */
  gpgx::g_z80->SetCycles((m68k.cycles / 15) * 15);

  /* 8-bit / 16-bit modes */
  if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
  {
    if (system_hw == SYSTEM_MCD)
    {
      /* FRES is only asserted on Power ON */
      if (hard_reset)
      {
        /* reset CD hardware */
        scd_reset(1);
      }

      /* reset MD cartridge hardware (only when booting from cartridge) */
      if (scd.cartridge.boot)
      {
        md_cart_reset(hard_reset);
      }
    }
    else
    {
      /* reset MD cartridge hardware */
      md_cart_reset(hard_reset);
    }

    /* Z80 bus is released & Z80 is reseted */
    m68k.memory_map[0xa0].read8   = m68k_read_bus_8;
    m68k.memory_map[0xa0].read16  = m68k_read_bus_16;
    m68k.memory_map[0xa0].write8  = m68k_unused_8_w;
    m68k.memory_map[0xa0].write16 = m68k_unused_16_w;
    zstate = 0;

    /* assume default bank is $000000-$007FFF */
    zbank = 0;  

    /* TMSS support */
    if ((core_config.bios & 1) && (system_hw == SYSTEM_MD) && hard_reset)
    {
      int i;

      /* clear TMSS register */
      xee::mem::Memset(tmss, 0x00, sizeof(tmss));

      /* VDP access is locked by default */
      for (i=0xc0; i<0xe0; i+=8)
      {
        m68k.memory_map[i].read8   = m68k_lockup_r_8;
        m68k.memory_map[i].read16  = m68k_lockup_r_16;
        m68k.memory_map[i].write8  = m68k_lockup_w_8;
        m68k.memory_map[i].write16 = m68k_lockup_w_16;
        zbank_memory_map[i].read   = zbank_lockup_r;
        zbank_memory_map[i].write  = zbank_lockup_w;
      }

      /* check if BOOT ROM is loaded */
      if (system_bios & SYSTEM_MD)
      {
        /* save default cartridge slot mapping */
        cart.base = m68k.memory_map[0].base;

        /* BOOT ROM is mapped at $000000-$0007FF */
        m68k.memory_map[0].base = boot_rom;
      }
    }

    /* reset MAIN-CPU */
    m68k_pulse_reset();
  }
  else
  {
    /* RAM state at power-on is undefined on some systems */
    if ((system_hw == SYSTEM_MARKIII) || ((system_hw & SYSTEM_SMS) && (region_code == REGION_JAPAN_NTSC)))
    {
      /* some korean games rely on RAM to be initialized with values different from $00 or $ff */
      xee::mem::Memset(work_ram, 0xf0, sizeof(work_ram));
    }

    /* reset cartridge hardware */
    sms_cart_reset();

    /* halt 68k (/VRES is forced low) */
    m68k_pulse_halt();
  }

  /* reset Z80 */
  gpgx::g_z80->Reset();

  /* some Z80 registers need to be initialized on Power ON */
  if (hard_reset)
  {
    /* Power Base Converter specific */
    if (system_hw == SYSTEM_PBC)
    {
      /* startup code logic (verified on real hardware): */
      /* 21 01 E1 : LD HL, $E101
         25 -- -- : DEC H
         F9 -- -- : LD SP,HL
         C7 -- -- : RST $00
         01 01 -- : LD BC, $xx01
      */
      gpgx::g_z80->SetHLRegister(0xE001);
      gpgx::g_z80->SetSPRegister(0xDFFF);
      gpgx::g_z80->SetRRegister(4);
    }

    /* Master System & Game Gear specific */
    else if (system_hw & (SYSTEM_SMS | SYSTEM_GG))
    {
      /* check if BIOS is not being used */
      if ((!(core_config.bios & 1) || !(system_bios & (SYSTEM_SMS | SYSTEM_GG))))
      {
        /* a few Master System (Ace of Aces, Shadow Dancer) & Game Gear (Ecco the Dolphin, Evander Holyfield Real Deal Boxing) games crash if SP is not properly initialized */
        gpgx::g_z80->SetSPRegister(0xDFF0);
      }
    }
  }
}

/*-----------------------------------------------------------------------*/
/*  OS ROM / TMSS register control functions (Genesis mode)              */
/*-----------------------------------------------------------------------*/

void gen_tmss_w(unsigned int offset, unsigned int data)
{
  int i;

  /* write TMSS register */
  WRITE_WORD(tmss, offset, data);

  /* VDP requires "SEGA" value to be written in TMSS register */
  if (xee::mem::Memcmp((char *)tmss, "SEGA", 4) == 0)
  {
    for (i=0xc0; i<0xe0; i+=8)
    {
      m68k.memory_map[i].read8    = vdp_read_byte;
      m68k.memory_map[i].read16   = vdp_read_word;
      m68k.memory_map[i].write8   = vdp_write_byte;
      m68k.memory_map[i].write16  = vdp_write_word;
      zbank_memory_map[i].read    = zbank_read_vdp;
      zbank_memory_map[i].write   = zbank_write_vdp;
    }
  }
  else
  {
    for (i=0xc0; i<0xe0; i+=8)
    {
      m68k.memory_map[i].read8    = m68k_lockup_r_8;
      m68k.memory_map[i].read16   = m68k_lockup_r_16;
      m68k.memory_map[i].write8   = m68k_lockup_w_8;
      m68k.memory_map[i].write16  = m68k_lockup_w_16;
      zbank_memory_map[i].read    = zbank_lockup_r;
      zbank_memory_map[i].write   = zbank_lockup_w;
    }
  }
}

void gen_bankswitch_w(unsigned int data)
{
  /* check if BOOT ROM is loaded */
  if (system_bios & SYSTEM_MD)
  {
    if (data & 1)
    {
      /* enable cartridge ROM */
      m68k.memory_map[0].base = cart.base;
    }
    else
    {
      /* enable internal BOOT ROM */
      m68k.memory_map[0].base = boot_rom;
    }
  }
}

unsigned int gen_bankswitch_r(void)
{
  /* check if BOOT ROM is loaded */
  if (system_bios & SYSTEM_MD)
  {
    return (m68k.memory_map[0].base == cart.base);
  }
  
  return 0xff;
}


/*-----------------------------------------------------------------------*/
/* Z80 Bus controller chip functions (Genesis mode)                      */
/* ----------------------------------------------------------------------*/

void gen_zbusreq_w(unsigned int data, unsigned int cycles)
{
  if (data)  /* !ZBUSREQ asserted */
  {
    /* check if Z80 is going to be stopped */
    if (zstate == 1)
    {
      /* resynchronize with 68k */ 
      gpgx::g_z80->Run(cycles);

      /* enable 68k access to Z80 bus */
      m68k.memory_map[0xa0].read8   = z80_read_byte;
      m68k.memory_map[0xa0].read16  = z80_read_word;
      m68k.memory_map[0xa0].write8  = z80_write_byte;
      m68k.memory_map[0xa0].write16 = z80_write_word;
    }

    /* update Z80 bus status */
    zstate |= 2;
  }
  else  /* !ZBUSREQ released */
  {
    /* check if Z80 is going to be restarted */
    if (zstate == 3)
    {
      /* resynchronize with 68k (Z80 cycles should remain a multiple of 15 MClocks) */
      gpgx::g_z80->SetCycles(((cycles + 14) / 15) * 15);

      /* disable 68k access to Z80 bus */
      m68k.memory_map[0xa0].read8   = m68k_read_bus_8;
      m68k.memory_map[0xa0].read16  = m68k_read_bus_16;
      m68k.memory_map[0xa0].write8  = m68k_unused_8_w;
      m68k.memory_map[0xa0].write16 = m68k_unused_16_w;
    }

    /* update Z80 bus status */
    zstate &= 1;
  }
}

void gen_zreset_w(unsigned int data, unsigned int cycles)
{
  if (data)  /* !ZRESET released */
  {
    /* check if Z80 is going to be restarted */
    if (zstate == 0)
    {
      /* resynchronize with 68k (Z80 cycles should remain a multiple of 15 MClocks) */
      gpgx::g_z80->SetCycles(((cycles + 14) / 15) * 15);

      /* reset Z80 & YM2612 */
      gpgx::g_z80->Reset();
      gpgx::g_fm_synthesizer->SyncAndReset(cycles);
    }

    /* check if 68k access to Z80 bus is granted */
    else if (zstate == 2)
    {
      /* enable 68k access to Z80 bus */
      m68k.memory_map[0xa0].read8   = z80_read_byte;
      m68k.memory_map[0xa0].read16  = z80_read_word;
      m68k.memory_map[0xa0].write8  = z80_write_byte;
      m68k.memory_map[0xa0].write16 = z80_write_word;

      /* reset Z80 & YM2612 */
      gpgx::g_z80->Reset();
      gpgx::g_fm_synthesizer->SyncAndReset(cycles);
    }

    /* update Z80 bus status */
    zstate |= 1;
  }
  else  /* !ZRESET asserted */
  {
    /* check if Z80 is going to be stopped */
    if (zstate == 1)
    {
      /* resynchronize with 68k */
      gpgx::g_z80->Run(cycles);
    }

    /* check if 68k had access to Z80 bus */
    else if (zstate == 3)
    {
      /* disable 68k access to Z80 bus */
      m68k.memory_map[0xa0].read8   = m68k_read_bus_8;
      m68k.memory_map[0xa0].read16  = m68k_read_bus_16;
      m68k.memory_map[0xa0].write8  = m68k_unused_8_w;
      m68k.memory_map[0xa0].write16 = m68k_unused_16_w;
    }

    /* stop YM2612 */
    gpgx::g_fm_synthesizer->SyncAndReset(cycles);

    /* update Z80 bus status */
    zstate &= 2;
  }
}

void gen_zbank_w (unsigned int data)
{
  zbank = ((zbank >> 1) | ((data & 1) << 23)) & 0xFF8000;
}


/*-----------------------------------------------------------------------*/
/* Z80 interrupt callback                                                */
/* ----------------------------------------------------------------------*/

int z80_irq_callback (int param)
{
  return -1;
}
