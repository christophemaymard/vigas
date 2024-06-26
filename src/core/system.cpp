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

#include "core/system.h"

#include "xee/fnd/data_type.h"

#include "osd.h" // For osd_input_update();
#include "core/m68k/m68k.h"
#include "core/audio_subsystem.h"
#include "core/core_config.h"
#include "core/system_cycle.h"
#include "core/system_hw.h"
#include "core/system_model.h"
#include "core/system_timing.h"
#include "core/ext.h" // For cart and scd.
#include "core/viewport.h"
#include "core/work_ram.h"
#include "core/zstate.h"
#include "core/genesis.h" // For gen_init() and gen_reset().
#include "core/cd_hw/scd.h"
#include "core/vdp_ctrl.h"
#include "core/vdp_render.h"
#include "core/io_ctrl.h"
#include "core/input_hw/input.h"
#include "core/cart_hw/special_hw_sms.h"
#include "core/cart_hw/svp/svp.h"

#include "core/cart_hw/svp/ssp16.h"

#include "gpgx/cpu/z80/z80_line_state.h"
#include "gpgx/hid/input.h"
#include "gpgx/g_audio_renderer.h"
#include "gpgx/g_hid_system.h"
#include "gpgx/g_z80.h"

static u8 pause_b;

/****************************************************************
 * Virtual System emulation
 ****************************************************************/
void system_init(void)
{
  gen_init();
  io_init();
  vdp_init();
  render_init();
  gpgx::g_audio_renderer->Init();
}

void system_reset(void)
{
  gen_reset(1);
  io_reset();
  render_reset();
  vdp_reset();
  gpgx::g_audio_renderer->ResetChips();
  audio_reset();
}

void system_frame_gen(int do_skip)
{
  /* line counters */
  int start, end, line;

  /* reset frame cycle counter */
  mcycles_vdp = 0;

  /* reset VDP FIFO */
  fifo_cycles[0] = 0;
  fifo_cycles[1] = 0;
  fifo_cycles[2] = 0;
  fifo_cycles[3] = 0;

  /* check if display setings have changed during previous frame */
  if (viewport.changed & 2)
  {
    /* interlaced modes */
    int old_interlaced = interlaced;
    interlaced = (reg[12] & 0x02) >> 1;

    if (old_interlaced != interlaced)
    {
      /* double resolution mode */
      im2_flag = ((reg[12] & 0x06) == 0x06);

      /* reset field status flag */
      odd_frame = interlaced;

      /* video mode has changed */
      viewport.changed = 5;

      /* update rendering mode */
      if (reg[1] & 0x04)
      {
        if (im2_flag)
        {
          if (reg[11] & 0x04) {
            g_bg_layer_renderer = g_bg_layer_renderer_m5_im2_vs;
          } else {
            g_bg_layer_renderer = g_bg_layer_renderer_m5_im2;
          }

          if (reg[12] & 0x08) {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5_im2_ste;
          } else {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5_im2;
          }
        }
        else
        {
          if (reg[11] & 0x04) {
            g_bg_layer_renderer = g_bg_layer_renderer_m5_vs;
          } else {
            g_bg_layer_renderer = g_bg_layer_renderer_m5;
          }

          if (reg[12] & 0x08) {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5_ste;
          } else {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5;
          }
        }
      }
    }
    else
    {
      /* clear flag */
      viewport.changed &= ~2;
    }

    /* active screen height */
    if (reg[1] & 0x04)
    {
      /* Mode 5 */
      if (reg[1] & 0x08)
      {
        /* 240 active lines */
        viewport.h = 240;
        viewport.y = (core_config.overscan & 1) * 24 * vdp_pal;
      }
      else
      {
        /* 224 active lines */
        viewport.h = 224;
        viewport.y = (core_config.overscan & 1) * (8 + (24 * vdp_pal));
      }
    }
    else
    {
      /* Mode 4 (192 active lines) */
      viewport.h = 192;
      viewport.y = (core_config.overscan & 1) * 24 * (vdp_pal + 1);
    }

    /* active screen width */
    viewport.w = 256 + ((reg[12] & 0x01) << 6);

    /* check viewport changes */
    if (viewport.h != viewport.oh)
    {
      viewport.oh = viewport.h;
      viewport.changed |= 1;
    }
  }

  /* first line of overscan */
  if (viewport.y)
  {
    blank_line(viewport.h, -viewport.x, viewport.w + 2*viewport.x);
  }
  
  /* clear DMA Busy, FIFO FULL & field flags */
  status &= 0xFEED;

  /* set VBLANK flag */
  status |= 0x08;

  /* check interlaced modes */
  if (interlaced)
  {
    /* switch even/odd field flag */
    odd_frame ^= 1;
    status |= (odd_frame << 4);
  }

  /* run VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(0);
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();
  
  /* H-Int counter */
  if (h_counter == 0)
  {
    /* Horizontal Interrupt is pending */
    hint_pending = 0x10;
    if (reg[0] & 0x10)
    {
      /* level 4 interrupt */
      m68k_update_irq(4);
    }
  }

  /* refresh inputs just before VINT (Warriors of Eternal Sun) */
  osd_input_update();

  /* VDP always starts after VBLANK so VINT cannot occur on first frame after a VDP reset (verified on real hardware) */
  if (v_counter != viewport.h)
  {
    /* reinitialize VCounter */
    v_counter = viewport.h;

    /* delay between VBLANK flag & Vertical Interrupt (Dracula, OutRunners, VR Troopers) */
    m68k_run(vint_cycle);
    if (zstate == 1)
    {
      gpgx::g_z80->Run(vint_cycle);
    }

    /* set VINT flag */
    status |= 0x80;
   
    /* Vertical Interrupt */
    vint_pending = 0x20;
    if (reg[1] & 0x20)
    {
      /* level 6 interrupt */
      m68k_set_irq(6);
    }

    /* assert Z80 interrupt */
    gpgx::g_z80->SetIRQLine(gpgx::cpu::z80::LineState::kAssertLine);
  }

  /* run 68k & Z80 until end of line */
  m68k_run(MCYCLES_PER_LINE);
  if (zstate == 1)
  {
    gpgx::g_z80->Run(MCYCLES_PER_LINE);
  }

  /* Z80 interrupt is cleared at the end of the line */
  gpgx::g_z80->SetIRQLine(gpgx::cpu::z80::LineState::kClearLine);

  /* run SVP chip */
  if (svp)
  {
    ssp1601_run(SVP_cycles);
  }

  /* update VDP cycle count */
  mcycles_vdp = MCYCLES_PER_LINE;

  /* initialize line count */
  line = viewport.h + 1; 

  /* initialize overscan area */
  start = lines_per_frame - viewport.y;
  end = viewport.h + viewport.y;

  /* Vertical Blanking */
  do
  {
    /* update VCounter */
    v_counter = line;

    /* render overscan */
    if ((line < end) || (line >= start))
    {
      blank_line(line, -viewport.x, viewport.w + 2*viewport.x);
    }

    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* run 68k & Z80 until end of line */
    m68k_run(mcycles_vdp + MCYCLES_PER_LINE);
    if (zstate == 1)
    {
      gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);
    }

    /* run SVP chip */
    if (svp)
    {
      ssp1601_run(SVP_cycles);
    }

    /* update VDP cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < (lines_per_frame - 1));
  
  /* update VCounter */
  v_counter = line;

  /* last line of overscan */
  if (viewport.y)
  {
    blank_line(line, -viewport.x, viewport.w + 2*viewport.x);
  }

  /* reload H-Int counter */
  h_counter = reg[10];
  
  /* clear VBLANK flag */
  status &= ~0x08;
 
  /* run VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(mcycles_vdp);
  }

  /* parse first line of sprites */
  if (reg[1] & 0x40)
  {
    g_satb_parser->ParseSpriteAttributeTable(-1);
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();

  /* run 68k & Z80 until end of line */
  m68k_run(mcycles_vdp + MCYCLES_PER_LINE);
  if (zstate == 1)
  {
    gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);
  }

  /* run SVP chip */
  if (svp)
  {
    ssp1601_run(SVP_cycles);
  }

  /* update VDP cycle count */
  mcycles_vdp += MCYCLES_PER_LINE;

  /* reset line count */
  line = 0;
  
  /* Active Display */
  do
  {
    /* update VCounter */
    v_counter = line;

    /* run VDP DMA */
    if (dma_length)
    {
      vdp_dma_update(mcycles_vdp);
    }

    /* render scanline */
    if (!do_skip)
    {
      render_line(line);
    }

    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* H-Int counter */
    if (h_counter == 0)
    {
      /* reload H-Int counter */
      h_counter = reg[10];
      
      /* Horizontal Interrupt is pending */
      hint_pending = 0x10;
      if (reg[0] & 0x10)
      {
        /* level 4 interrupt */
        m68k_update_irq(4);
      }
    }
    else
    {
      /* decrement H-Int counter */
      h_counter--;
    }

    /* run 68k & Z80 until end of line */
    m68k_run(mcycles_vdp + MCYCLES_PER_LINE);
    if (zstate == 1)
    {
      gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);
    }

    /* run SVP chip */
    if (svp)
    {
      ssp1601_run(SVP_cycles);
    }

    /* update VDP cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < viewport.h);

  /* check viewport changes */
  if (viewport.w != viewport.ow)
  {
    viewport.ow = viewport.w;
    viewport.changed |= 1;
  }

  /* adjust timings for next frame */
  input_end_frame(mcycles_vdp);
  m68k.refresh_cycles -= mcycles_vdp;
  m68k.cycles -= mcycles_vdp;
  gpgx::g_z80->SubCycles(mcycles_vdp);
  dma_endCycles = 0;
}

void system_frame_scd(int do_skip)
{
  /* line counters */
  int start, end, line;

  /* reset frame cycle counter */
  mcycles_vdp = 0;
  scd.cycles = 0;

  /* reset VDP FIFO */
  fifo_cycles[0] = 0;
  fifo_cycles[1] = 0;
  fifo_cycles[2] = 0;
  fifo_cycles[3] = 0;

  /* check if display setings have changed during previous frame */
  if (viewport.changed & 2)
  {
    /* interlaced modes */
    int old_interlaced = interlaced;
    interlaced = (reg[12] & 0x02) >> 1;

    if (old_interlaced != interlaced)
    {
      /* double resolution mode */
      im2_flag = ((reg[12] & 0x06) == 0x06);

      /* reset field status flag */
      odd_frame = interlaced;

      /* video mode has changed */
      viewport.changed = 5;

      /* update rendering mode */
      if (reg[1] & 0x04)
      {
        if (im2_flag)
        {
          if (reg[11] & 0x04) {
            g_bg_layer_renderer = g_bg_layer_renderer_m5_im2_vs;
          } else {
            g_bg_layer_renderer = g_bg_layer_renderer_m5_im2;
          }

          if (reg[12] & 0x08) {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5_im2_ste;
          } else {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5_im2;
          }
        }
        else
        {
          if (reg[11] & 0x04) {
            g_bg_layer_renderer = g_bg_layer_renderer_m5_vs;
          } else {
            g_bg_layer_renderer = g_bg_layer_renderer_m5;
          }

          if (reg[12] & 0x08) {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5_ste;
          } else {
            g_sprite_layer_renderer = g_sprite_layer_renderer_m5;
          }
        }
      }
    }
    else
    {
      /* clear flag */
      viewport.changed &= ~2;
    }

    /* active screen height */
    if (reg[1] & 0x04)
    {
      /* Mode 5 */
      if (reg[1] & 0x08)
      {
        /* 240 active lines */
        viewport.h = 240;
        viewport.y = (core_config.overscan & 1) * 24 * vdp_pal;
      }
      else
      {
        /* 224 active lines */
        viewport.h = 224;
        viewport.y = (core_config.overscan & 1) * (8 + (24 * vdp_pal));
      }
    }
    else
    {
      /* Mode 4 (192 active lines) */
      viewport.h = 192;
      viewport.y = (core_config.overscan & 1) * 24 * (vdp_pal + 1);
    }

    /* active screen width */
    viewport.w = 256 + ((reg[12] & 0x01) << 6);

    /* check viewport changes */
    if (viewport.h != viewport.oh)
    {
      viewport.oh = viewport.h;
      viewport.changed |= 1;
    }
  }

  /* first line of overscan */
  if (viewport.y)
  {
    blank_line(viewport.h, -viewport.x, viewport.w + 2*viewport.x);
  }
  
  /* clear DMA Busy, FIFO FULL & field flags */
  status &= 0xFEED;

  /* set VBLANK flag */
  status |= 0x08;

  /* check interlaced modes */
  if (interlaced)
  {
    /* switch even/odd field flag */
    odd_frame ^= 1;
    status |= (odd_frame << 4);
  }

  /* run VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(0);
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();

  /* H-Int counter */
  if (h_counter == 0)
  {
    /* Horizontal Interrupt is pending */
    hint_pending = 0x10;
    if (reg[0] & 0x10)
    {
      /* level 4 interrupt */
      m68k_update_irq(4);
    }
  }

  /* refresh inputs just before VINT */
  osd_input_update();

  /* VDP always starts after VBLANK so VINT cannot occur on first frame after a VDP reset (verified on real hardware) */
  if (v_counter != viewport.h)
  {
    /* reinitialize VCounter */
    v_counter = viewport.h;

    /* delay between VBLANK flag & Vertical Interrupt (Dracula, OutRunners, VR Troopers) */
    m68k_run(vint_cycle);
    if (zstate == 1)
    {
      gpgx::g_z80->Run(vint_cycle);
    }

    /* set VINT flag */
    status |= 0x80;

    /* Vertical Interrupt */
    vint_pending = 0x20;
    if (reg[1] & 0x20)
    {
      /* level 6 interrupt */
      m68k_set_irq(6);
    }

    /* assert Z80 interrupt */
    gpgx::g_z80->SetIRQLine(gpgx::cpu::z80::LineState::kAssertLine);
  }

  /* run both 68k & CD hardware until end of line */
  scd_update(MCYCLES_PER_LINE);

  /* run Z80 until end of line */
  if (zstate == 1)
  {
    gpgx::g_z80->Run(MCYCLES_PER_LINE);
  }

  /* Z80 interrupt is cleared at the end of the line */
  gpgx::g_z80->SetIRQLine(gpgx::cpu::z80::LineState::kClearLine);

  /* update VDP cycle count */
  mcycles_vdp = MCYCLES_PER_LINE;

  /* initialize line count */
  line = viewport.h + 1; 

  /* initialize overscan area */
  start = lines_per_frame - viewport.y;
  end = viewport.h + viewport.y;

  /* Vertical Blanking */
  do
  {
    /* update VCounter */
    v_counter = line;

    /* render overscan */
    if ((line < end) || (line >= start))
    {
      blank_line(line, -viewport.x, viewport.w + 2*viewport.x);
    }

    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* run both 68k & CD hardware until end of line */
    scd_update(mcycles_vdp + MCYCLES_PER_LINE);

    /* run Z80 until end of line */
    if (zstate == 1)
    {
      gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);
    }

    /* update VDP cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < (lines_per_frame - 1));
  
  /* update VCounter */
  v_counter = line;

  /* last line of overscan */
  if (viewport.y)
  {
    blank_line(line, -viewport.x, viewport.w + 2*viewport.x);
  }

  /* reload H-Int counter */
  h_counter = reg[10];
  
  /* clear VBLANK flag */
  status &= ~0x08;
 
  /* run VDP DMA */
  if (dma_length)
  {
    vdp_dma_update(mcycles_vdp);
  }

  /* parse first line of sprites */
  if (reg[1] & 0x40)
  {
    g_satb_parser->ParseSpriteAttributeTable(-1);
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();

  /* run both 68k & CD hardware until end of line */
  scd_update(mcycles_vdp + MCYCLES_PER_LINE);

  /* run Z80 until end of line */
  if (zstate == 1)
  {
    gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);
  }

  /* update VDP cycle count */
  mcycles_vdp += MCYCLES_PER_LINE;

  /* reset line count */
  line = 0;
  
  /* Active Display */
  do
  {
    /* update VCounter */
    v_counter = line;

    /* run VDP DMA */
    if (dma_length)
    {
      vdp_dma_update(mcycles_vdp);
    }

    /* render scanline */
    if (!do_skip)
    {
      render_line(line);
    }
    
    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* H-Int counter */
    if (h_counter == 0)
    {
      /* reload H-Int counter */
      h_counter = reg[10];
      
      /* Horizontal Interrupt is pending */
      hint_pending = 0x10;
      if (reg[0] & 0x10)
      {
        /* level 4 interrupt */
        m68k_update_irq(4);
      }
    }
    else
    {
      /* decrement H-Int counter */
      h_counter--;
    }

    /* run both 68k & CD hardware until end of line */
    scd_update(mcycles_vdp + MCYCLES_PER_LINE);

    /* run Z80 until end of line */
    if (zstate == 1)
    {
      gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);
    }

    /* update VDP cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < viewport.h);

  /* check viewport changes */
  if (viewport.w != viewport.ow)
  {
    viewport.ow = viewport.w;
    viewport.changed |= 1;
  }
  
  /* adjust timings for next frame */
  scd_end_frame(scd.cycles);
  input_end_frame(mcycles_vdp);
  m68k.refresh_cycles -= mcycles_vdp;
  m68k.cycles -= mcycles_vdp;
  gpgx::g_z80->SubCycles(mcycles_vdp);
  dma_endCycles = 0;
}

void system_frame_sms(int do_skip)
{
  /* line counter */
  int start, end, line;

  /* reset frame cycle count */
  mcycles_vdp = 0;

  /* reset VDP FIFO */
  fifo_cycles[0] = 0;
  fifo_cycles[1] = 0;
  fifo_cycles[2] = 0;
  fifo_cycles[3] = 0;

  /* check if display settings has changed during previous frame */
  if (viewport.changed & 2)
  {
    viewport.changed &= ~2;

    if (system_hw & SYSTEM_MD)
    {
      /* interlaced modes */
      int old_interlaced = interlaced;
      interlaced = (reg[12] & 0x02) >> 1;

      if (old_interlaced != interlaced)
      {
        /* double resolution mode */
        im2_flag = ((reg[12] & 0x06) == 0x06);

        /* reset field status flag */
        odd_frame = interlaced;

        /* video mode has changed */
        viewport.changed = 5;

        /* update rendering mode */
        if (reg[1] & 0x04)
        {
          if (im2_flag)
          {
            if (reg[11] & 0x04) {
              g_bg_layer_renderer = g_bg_layer_renderer_m5_im2_vs;
            } else {
              g_bg_layer_renderer = g_bg_layer_renderer_m5_im2;
            }

            if (reg[12] & 0x08) {
              g_sprite_layer_renderer = g_sprite_layer_renderer_m5_im2_ste;
            } else {
              g_sprite_layer_renderer = g_sprite_layer_renderer_m5_im2;
            }
          }
          else
          {
            if (reg[11] & 0x04) {
              g_bg_layer_renderer = g_bg_layer_renderer_m5_vs;
            } else {
              g_bg_layer_renderer = g_bg_layer_renderer_m5;
            }

            if (reg[12] & 0x08) {
              g_sprite_layer_renderer = g_sprite_layer_renderer_m5_ste;
            } else {
              g_sprite_layer_renderer = g_sprite_layer_renderer_m5;
            }
          }
        }
      }

      /* active screen height */
      if (reg[1] & 0x04)
      {
        /* Mode 5 */
        if (reg[1] & 0x08)
        {
          /* 240 active lines */
          viewport.h = 240;
          viewport.y = (core_config.overscan & 1) * 24 * vdp_pal;
        }
        else
        {
          /* 224 active lines */
          viewport.h = 224;
          viewport.y = (core_config.overscan & 1) * (8 + (24 * vdp_pal));
        }
      }
      else
      {
        viewport.h = 192;
        viewport.y = (core_config.overscan & 1) * 24 * (vdp_pal + 1);
      }
    }
    else
    {
      /* check for VDP extended modes */
      int mode = (reg[0] & 0x06) | (reg[1] & 0x18);

      /* update active height */
      if (mode == 0x0E)
      {
        viewport.h = 240;
      }
      else if (mode == 0x16)
      {
        viewport.h = 224;
      }
      else
      {
        viewport.h = 192;
      }

      /* update vertical overscan */
      if (core_config.overscan & 1)
      {
        viewport.y = (240 + 48*vdp_pal - viewport.h) >> 1;
      }
      else
      {
        if ((system_hw == SYSTEM_GG) && !core_config.gg_extra)
        {
          /* Display area reduced to 160x144 */
          viewport.y = (144 - viewport.h) / 2;
        }
        else
        {
          viewport.y = 0;
        }
      }
    }

    /* active screen width */
    viewport.w = 256 + ((reg[12] & 0x01) << 6);

    /* check viewport changes */
    if (viewport.h != viewport.oh)
    {
      viewport.oh = viewport.h;
      viewport.changed |= 1;
    }
  }

  /* initialize VCounter */
  v_counter = viewport.h;

  /* first line of overscan */
  if (viewport.y > 0)
  {
    blank_line(v_counter, -viewport.x, viewport.w + 2*viewport.x);
  }

  /* Mega Drive VDP specific */
  if (system_hw & SYSTEM_MD)
  {
    /* clear DMA Busy & field flags */
    status &= 0xED;

    /* set VBLANK flag */
    status |= 0x08;

    /* interlaced modes only */
    if (interlaced)
    {
      /* switch even/odd field flag */
      odd_frame ^= 1;
      status |= (odd_frame << 4);
    }

    /* run VDP DMA */
    if (dma_length)
    {
      vdp_dma_update(0);
    }
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();

  /* H-Int counter */
  if (h_counter == 0)
  {
    /* Horizontal Interrupt is pending */
    hint_pending = 0x10;
    if (reg[0] & 0x10)
    {
      /* Cycle-accurate HINT */
      /* IRQ line is latched between instructions, during instruction last cycle.       */
      /* This means that if Z80 cycle count is exactly a multiple of MCYCLES_PER_LINE,  */
      /* interrupt should be triggered AFTER the next instruction.                      */
      if ((gpgx::g_z80->GetCycles() % MCYCLES_PER_LINE) == 0)
      {
        gpgx::g_z80->Run(gpgx::g_z80->GetCycles() + 1);
      }

      /* Z80 interrupt */
      gpgx::g_z80->SetIRQLine(gpgx::cpu::z80::LineState::kAssertLine);
    }
  }

  /* refresh inputs just before VINT */
  osd_input_update();

  /* run Z80 until end of line */
  gpgx::g_z80->Run(MCYCLES_PER_LINE);

  /* make sure VINT flag was not read (then cleared) by last instruction */
  if (v_counter == viewport.h)
  {
    /* Set VINT flag */
    status |= 0x80;

    /* Vertical Interrupt */
    vint_pending = 0x20;
    if (reg[1] & 0x20)
    {
      gpgx::g_z80->SetIRQLine(gpgx::cpu::z80::LineState::kAssertLine);
    }
  }

  /* update VDP cycle count */
  mcycles_vdp = MCYCLES_PER_LINE;

  /* initialize line count */
  line = viewport.h + 1;

  /* initialize overscan area */
  start = lines_per_frame - viewport.y;
  end   = viewport.h + viewport.y;

  /* Vertical Blanking */
  do
  {
    /* update VCounter */
    v_counter = line;

    /* render overscan */
    if ((line < end) || (line >= start))
    {
      /* Master System & Game Gear VDP specific */
      if ((system_hw < SYSTEM_MD) && (line > (lines_per_frame - 16)))
      {
        /* Sprites are still processed during top border */
        if (reg[1] & 0x40)
        {
          g_sprite_layer_renderer->RenderSprites((line - lines_per_frame) & 1);
        }
        
        /* Sprites pre-processing occurs even when display is disabled */
        g_satb_parser->ParseSpriteAttributeTable(line - lines_per_frame);
      }

      blank_line(line, -viewport.x, viewport.w + 2*viewport.x);
    }

    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* run Z80 until end of line */
    gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);

    /* update VDP cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < (lines_per_frame - 1));

  /* update VCounter */
  v_counter = line;

  /* last line of overscan */
  if (viewport.y > 0)
  {
    /* Master System & Game Gear VDP specific */
    if (system_hw < SYSTEM_MD)
    {
      /* Sprites are still processed during top border */
      if (reg[1] & 0x40)
      {
        g_sprite_layer_renderer->RenderSprites(1);
      }
    }

    blank_line(line, -viewport.x, viewport.w + 2*viewport.x);
  }

  /* reload H-Int counter */
  h_counter = reg[10];

  /* Detect pause button input (in Game Gear Mode, NMI is not generated) */
  if (system_hw != SYSTEM_GG)
  {
    if (gpgx::g_hid_system->GetController(0)->IsButtonPressed(gpgx::hid::Button::kStart))
    {
      /* NMI is edge-triggered */
      if (!pause_b)
      {
        pause_b = 1;
        gpgx::g_z80->SetNMILine(gpgx::cpu::z80::LineState::kAssertLine);
        gpgx::g_z80->SetNMILine(gpgx::cpu::z80::LineState::kClearLine);
      }
    }
    else
    {
      pause_b = 0;
    }
  }

  /* 3-D glasses faking: skip rendering of left lens frame */
  do_skip |= (work_ram[0x1ffb] & cart.special & HW_3D_GLASSES);

  /* Mega Drive VDP specific */
  if (system_hw & SYSTEM_MD)
  {
    /* clear VBLANK flag */
    status &= ~0x08;

    /* run VDP DMA */
    if (dma_length)
    {
      vdp_dma_update(mcycles_vdp);
    }
    
    /* parse first line of sprites */
    if (reg[1] & 0x40)
    {
      g_satb_parser->ParseSpriteAttributeTable(-1);
    }
  }

  /* Master System & Game Gear VDP specific */
  else
  {    
    /* Sprites pre-processing occurs even when display is disabled */
    g_satb_parser->ParseSpriteAttributeTable(-1);
  }

  /* update 6-Buttons & Lightguns */
  input_refresh();

  /* run Z80 until end of line */
  gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);

  /* update VDP cycle count */
  mcycles_vdp += MCYCLES_PER_LINE;

  /* latch Vertical Scroll register */
  vscroll = reg[9];
  
  /* reset line count */
  line = 0;

  /* Active Display */
  do
  {
    /* run VDP DMA (Mega Drive VDP specific) */
    if (dma_length)
    {
      vdp_dma_update(mcycles_vdp);
    }

    /* make sure that line has not already been rendered */
    if (v_counter != line)
    {
      /* update VCounter */
      v_counter = line;

      /* render scanline */
      if (!do_skip)
      {
        render_line(line);
      }
    }

    /* update 6-Buttons & Lightguns */
    input_refresh();

    /* H-Int counter */
    if (h_counter == 0)
    {
      /* reload H-Int counter */
      h_counter = reg[10];
      
      /* Horizontal Interrupt is pending */
      hint_pending = 0x10;
      if (reg[0] & 0x10)
      {
        /* Cycle-accurate HINT */
        /* IRQ line is latched between instructions, during instruction last cycle.       */
        /* This means that if Z80 cycle count is exactly a multiple of MCYCLES_PER_LINE,  */
        /* interrupt should be triggered AFTER the next instruction.                      */
        if ((gpgx::g_z80->GetCycles() % MCYCLES_PER_LINE) == 0)
        {
          gpgx::g_z80->Run(gpgx::g_z80->GetCycles() + 1);
        }

        /* assert Z80 interrupt */
        gpgx::g_z80->SetIRQLine(gpgx::cpu::z80::LineState::kAssertLine);
      }
    }
    else
    {
      /* decrement H-Int counter */
      h_counter--;
    }

    /* run Z80 until end of line */
    gpgx::g_z80->Run(mcycles_vdp + MCYCLES_PER_LINE);

    /* update VDP cycle count */
    mcycles_vdp += MCYCLES_PER_LINE;
  }
  while (++line < viewport.h);

  /* check viewport changes */
  if (viewport.w != viewport.ow)
  {
    viewport.ow = viewport.w;
    viewport.changed |= 1;
  }

  /* adjust timings for next frame */
  input_end_frame(mcycles_vdp);
  gpgx::g_z80->SubCycles(mcycles_vdp);
}
