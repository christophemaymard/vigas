/*
 * Copyright (C) 2017-2022 Alexey Khokholov (Nuke.YKT)
 *
 * This file is part of Nuked OPN2.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *  Nuked OPN2(Yamaha YM3438) emulator.
 *  Thanks:
 *      Silicon Pr0n:
 *          Yamaha YM3438 decap and die shot(digshadow).
 *      OPLx decapsulated(Matthew Gambrell, Olli Niemitalo):
 *          OPL2 ROMs.
 *
 * version: 1.0.12
 */

#include "gpgx/ic/ym3438/ym3438.h"

#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "core/state.h"

#include "gpgx/ic/ym3438/ym3438_mode.h"

namespace gpgx::ic::ym3438 {

//==============================================================================
// Ym3438

//------------------------------------------------------------------------------

#define SIGN_EXTEND(bit_index, value) (((value) & ((1u << (bit_index)) - 1u)) - ((value) & (1u << (bit_index))))

//------------------------------------------------------------------------------

enum
{
  eg_num_attack = 0,
  eg_num_decay = 1,
  eg_num_sustain = 2,
  eg_num_release = 3
};

//------------------------------------------------------------------------------

// logsin table.
const u16 Ym3438::kLogSinTable[256] = {
    0x859, 0x6c3, 0x607, 0x58b, 0x52e, 0x4e4, 0x4a6, 0x471,
    0x443, 0x41a, 0x3f5, 0x3d3, 0x3b5, 0x398, 0x37e, 0x365,
    0x34e, 0x339, 0x324, 0x311, 0x2ff, 0x2ed, 0x2dc, 0x2cd,
    0x2bd, 0x2af, 0x2a0, 0x293, 0x286, 0x279, 0x26d, 0x261,
    0x256, 0x24b, 0x240, 0x236, 0x22c, 0x222, 0x218, 0x20f,
    0x206, 0x1fd, 0x1f5, 0x1ec, 0x1e4, 0x1dc, 0x1d4, 0x1cd,
    0x1c5, 0x1be, 0x1b7, 0x1b0, 0x1a9, 0x1a2, 0x19b, 0x195,
    0x18f, 0x188, 0x182, 0x17c, 0x177, 0x171, 0x16b, 0x166,
    0x160, 0x15b, 0x155, 0x150, 0x14b, 0x146, 0x141, 0x13c,
    0x137, 0x133, 0x12e, 0x129, 0x125, 0x121, 0x11c, 0x118,
    0x114, 0x10f, 0x10b, 0x107, 0x103, 0x0ff, 0x0fb, 0x0f8,
    0x0f4, 0x0f0, 0x0ec, 0x0e9, 0x0e5, 0x0e2, 0x0de, 0x0db,
    0x0d7, 0x0d4, 0x0d1, 0x0cd, 0x0ca, 0x0c7, 0x0c4, 0x0c1,
    0x0be, 0x0bb, 0x0b8, 0x0b5, 0x0b2, 0x0af, 0x0ac, 0x0a9,
    0x0a7, 0x0a4, 0x0a1, 0x09f, 0x09c, 0x099, 0x097, 0x094,
    0x092, 0x08f, 0x08d, 0x08a, 0x088, 0x086, 0x083, 0x081,
    0x07f, 0x07d, 0x07a, 0x078, 0x076, 0x074, 0x072, 0x070,
    0x06e, 0x06c, 0x06a, 0x068, 0x066, 0x064, 0x062, 0x060,
    0x05e, 0x05c, 0x05b, 0x059, 0x057, 0x055, 0x053, 0x052,
    0x050, 0x04e, 0x04d, 0x04b, 0x04a, 0x048, 0x046, 0x045,
    0x043, 0x042, 0x040, 0x03f, 0x03e, 0x03c, 0x03b, 0x039,
    0x038, 0x037, 0x035, 0x034, 0x033, 0x031, 0x030, 0x02f,
    0x02e, 0x02d, 0x02b, 0x02a, 0x029, 0x028, 0x027, 0x026,
    0x025, 0x024, 0x023, 0x022, 0x021, 0x020, 0x01f, 0x01e,
    0x01d, 0x01c, 0x01b, 0x01a, 0x019, 0x018, 0x017, 0x017,
    0x016, 0x015, 0x014, 0x014, 0x013, 0x012, 0x011, 0x011,
    0x010, 0x00f, 0x00f, 0x00e, 0x00d, 0x00d, 0x00c, 0x00c,
    0x00b, 0x00a, 0x00a, 0x009, 0x009, 0x008, 0x008, 0x007,
    0x007, 0x007, 0x006, 0x006, 0x005, 0x005, 0x005, 0x004,
    0x004, 0x004, 0x003, 0x003, 0x003, 0x002, 0x002, 0x002,
    0x002, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};

//------------------------------------------------------------------------------

// exp table.
const u16 Ym3438::kExpTable[256] = {
    0x000, 0x003, 0x006, 0x008, 0x00b, 0x00e, 0x011, 0x014,
    0x016, 0x019, 0x01c, 0x01f, 0x022, 0x025, 0x028, 0x02a,
    0x02d, 0x030, 0x033, 0x036, 0x039, 0x03c, 0x03f, 0x042,
    0x045, 0x048, 0x04b, 0x04e, 0x051, 0x054, 0x057, 0x05a,
    0x05d, 0x060, 0x063, 0x066, 0x069, 0x06c, 0x06f, 0x072,
    0x075, 0x078, 0x07b, 0x07e, 0x082, 0x085, 0x088, 0x08b,
    0x08e, 0x091, 0x094, 0x098, 0x09b, 0x09e, 0x0a1, 0x0a4,
    0x0a8, 0x0ab, 0x0ae, 0x0b1, 0x0b5, 0x0b8, 0x0bb, 0x0be,
    0x0c2, 0x0c5, 0x0c8, 0x0cc, 0x0cf, 0x0d2, 0x0d6, 0x0d9,
    0x0dc, 0x0e0, 0x0e3, 0x0e7, 0x0ea, 0x0ed, 0x0f1, 0x0f4,
    0x0f8, 0x0fb, 0x0ff, 0x102, 0x106, 0x109, 0x10c, 0x110,
    0x114, 0x117, 0x11b, 0x11e, 0x122, 0x125, 0x129, 0x12c,
    0x130, 0x134, 0x137, 0x13b, 0x13e, 0x142, 0x146, 0x149,
    0x14d, 0x151, 0x154, 0x158, 0x15c, 0x160, 0x163, 0x167,
    0x16b, 0x16f, 0x172, 0x176, 0x17a, 0x17e, 0x181, 0x185,
    0x189, 0x18d, 0x191, 0x195, 0x199, 0x19c, 0x1a0, 0x1a4,
    0x1a8, 0x1ac, 0x1b0, 0x1b4, 0x1b8, 0x1bc, 0x1c0, 0x1c4,
    0x1c8, 0x1cc, 0x1d0, 0x1d4, 0x1d8, 0x1dc, 0x1e0, 0x1e4,
    0x1e8, 0x1ec, 0x1f0, 0x1f5, 0x1f9, 0x1fd, 0x201, 0x205,
    0x209, 0x20e, 0x212, 0x216, 0x21a, 0x21e, 0x223, 0x227,
    0x22b, 0x230, 0x234, 0x238, 0x23c, 0x241, 0x245, 0x249,
    0x24e, 0x252, 0x257, 0x25b, 0x25f, 0x264, 0x268, 0x26d,
    0x271, 0x276, 0x27a, 0x27f, 0x283, 0x288, 0x28c, 0x291,
    0x295, 0x29a, 0x29e, 0x2a3, 0x2a8, 0x2ac, 0x2b1, 0x2b5,
    0x2ba, 0x2bf, 0x2c4, 0x2c8, 0x2cd, 0x2d2, 0x2d6, 0x2db,
    0x2e0, 0x2e5, 0x2e9, 0x2ee, 0x2f3, 0x2f8, 0x2fd, 0x302,
    0x306, 0x30b, 0x310, 0x315, 0x31a, 0x31f, 0x324, 0x329,
    0x32e, 0x333, 0x338, 0x33d, 0x342, 0x347, 0x34c, 0x351,
    0x356, 0x35b, 0x360, 0x365, 0x36a, 0x370, 0x375, 0x37a,
    0x37f, 0x384, 0x38a, 0x38f, 0x394, 0x399, 0x39f, 0x3a4,
    0x3a9, 0x3ae, 0x3b4, 0x3b9, 0x3bf, 0x3c4, 0x3c9, 0x3cf,
    0x3d4, 0x3da, 0x3df, 0x3e4, 0x3ea, 0x3ef, 0x3f5, 0x3fa
};

//------------------------------------------------------------------------------

// Note table.
const u32 Ym3438::kFnNote[16] = {
    0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3
};

//------------------------------------------------------------------------------
// Envelope generator.

const u32 Ym3438::kEgStephi[4][4] = {
    { 0, 0, 0, 0 },
    { 1, 0, 0, 0 },
    { 1, 0, 1, 0 },
    { 1, 1, 1, 0 }
};

const u8 Ym3438::kEgAmShift[4] = {
    7, 3, 1, 0
};

//------------------------------------------------------------------------------
// Phase generator.

const u32 Ym3438::kPgDetune[8] = { 16, 17, 19, 20, 22, 24, 27, 29 };

const u32 Ym3438::kPgLfoSh1[8][8] = {
    { 7, 7, 7, 7, 7, 7, 7, 7 },
    { 7, 7, 7, 7, 7, 7, 7, 7 },
    { 7, 7, 7, 7, 7, 7, 1, 1 },
    { 7, 7, 7, 7, 1, 1, 1, 1 },
    { 7, 7, 7, 1, 1, 1, 1, 0 },
    { 7, 7, 1, 1, 0, 0, 0, 0 },
    { 7, 7, 1, 1, 0, 0, 0, 0 },
    { 7, 7, 1, 1, 0, 0, 0, 0 }
};

const u32 Ym3438::kPgLfoSh2[8][8] = {
    { 7, 7, 7, 7, 7, 7, 7, 7 },
    { 7, 7, 7, 7, 2, 2, 2, 2 },
    { 7, 7, 7, 2, 2, 2, 7, 7 },
    { 7, 7, 2, 2, 7, 7, 2, 2 },
    { 7, 7, 2, 7, 7, 7, 2, 7 },
    { 7, 7, 7, 2, 7, 7, 2, 1 },
    { 7, 7, 7, 2, 7, 7, 2, 1 },
    { 7, 7, 7, 2, 7, 7, 2, 1 }
};

//------------------------------------------------------------------------------

// Address decoder.

const u32 Ym3438::kOpOffset[12] = {
    0x000, // Ch1 OP1/OP2
    0x001, // Ch2 OP1/OP2
    0x002, // Ch3 OP1/OP2
    0x100, // Ch4 OP1/OP2
    0x101, // Ch5 OP1/OP2
    0x102, // Ch6 OP1/OP2
    0x004, // Ch1 OP3/OP4
    0x005, // Ch2 OP3/OP4
    0x006, // Ch3 OP3/OP4
    0x104, // Ch4 OP3/OP4
    0x105, // Ch5 OP3/OP4
    0x106  // Ch6 OP3/OP4
};

const u32 Ym3438::kChOffset[6] = {
    0x000, // Ch1
    0x001, // Ch2
    0x002, // Ch3
    0x100, // Ch4
    0x101, // Ch5
    0x102  // Ch6
};

//------------------------------------------------------------------------------

// LFO.
const u32 Ym3438::kLfoCycles[8] = {
    108, 77, 71, 67, 62, 44, 8, 5
};

//------------------------------------------------------------------------------

// FM algorithm.
const u32 Ym3438::kFmAlgorithm[4][6][8] = {
    {
        { 1, 1, 1, 1, 1, 1, 1, 1 }, // OP1_0
        { 1, 1, 1, 1, 1, 1, 1, 1 }, // OP1_1
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // OP2
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // Last operator
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // Last operator
        { 0, 0, 0, 0, 0, 0, 0, 1 }  // Out
    },
    {
        { 0, 1, 0, 0, 0, 1, 0, 0 }, // OP1_0
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // OP1_1
        { 1, 1, 1, 0, 0, 0, 0, 0 }, // OP2
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // Last operator
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // Last operator
        { 0, 0, 0, 0, 0, 1, 1, 1 }  // Out
    },
    {
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // OP1_0
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // OP1_1
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // OP2
        { 1, 0, 0, 1, 1, 1, 1, 0 }, // Last operator
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // Last operator
        { 0, 0, 0, 0, 1, 1, 1, 1 }  // Out
    },
    {
        { 0, 0, 1, 0, 0, 1, 0, 0 }, // OP1_0
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // OP1_1
        { 0, 0, 0, 1, 0, 0, 0, 0 }, // OP2
        { 1, 1, 0, 1, 1, 0, 0, 0 }, // Last operator
        { 0, 0, 1, 0, 0, 0, 0, 0 }, // Last operator
        { 1, 1, 1, 1, 1, 1, 1, 1 }  // Out
    }
};

//------------------------------------------------------------------------------

void Ym3438::OPN2_DoIO()
{
  // Write signal check.
  m_opn2_ctx.write_a_en = (m_opn2_ctx.write_a & 0x03) == 0x01;
  m_opn2_ctx.write_d_en = (m_opn2_ctx.write_d & 0x03) == 0x01;
  m_opn2_ctx.write_a <<= 1;
  m_opn2_ctx.write_d <<= 1;

  // Busy counter.
  m_opn2_ctx.busy = m_opn2_ctx.write_busy;
  m_opn2_ctx.write_busy_cnt += m_opn2_ctx.write_busy;
  m_opn2_ctx.write_busy = (m_opn2_ctx.write_busy && !(m_opn2_ctx.write_busy_cnt >> 5)) || m_opn2_ctx.write_d_en;
  m_opn2_ctx.write_busy_cnt &= 0x1f;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_DoRegWrite()
{
  u32 i;
  u32 slot = m_opn2_ctx.cycles % 12;
  u32 address;
  u32 channel = m_opn2_ctx.channel;
  // Update registers.
  if (m_opn2_ctx.write_fm_data) {
    // Slot.
    if (kOpOffset[slot] == (m_opn2_ctx.address & 0x107)) {
      if (m_opn2_ctx.address & 0x08) {
        // OP2, OP4.
        slot += 12;
      }
      address = m_opn2_ctx.address & 0xf0;
      switch (address) {
        case 0x30: // DT, MULTI.
          m_opn2_ctx.multi[slot] = m_opn2_ctx.data & 0x0f;
          if (!m_opn2_ctx.multi[slot]) {
            m_opn2_ctx.multi[slot] = 1;
          } else {
            m_opn2_ctx.multi[slot] <<= 1;
          }
          m_opn2_ctx.dt[slot] = (m_opn2_ctx.data >> 4) & 0x07;
          break;
        case 0x40: // TL.
          m_opn2_ctx.tl[slot] = m_opn2_ctx.data & 0x7f;
          break;
        case 0x50: // KS, AR.
          m_opn2_ctx.ar[slot] = m_opn2_ctx.data & 0x1f;
          m_opn2_ctx.ks[slot] = (m_opn2_ctx.data >> 6) & 0x03;
          break;
        case 0x60: // AM, DR.
          m_opn2_ctx.dr[slot] = m_opn2_ctx.data & 0x1f;
          m_opn2_ctx.am[slot] = (m_opn2_ctx.data >> 7) & 0x01;
          break;
        case 0x70: // SR.
          m_opn2_ctx.sr[slot] = m_opn2_ctx.data & 0x1f;
          break;
        case 0x80: // SL, RR.
          m_opn2_ctx.rr[slot] = m_opn2_ctx.data & 0x0f;
          m_opn2_ctx.sl[slot] = (m_opn2_ctx.data >> 4) & 0x0f;
          m_opn2_ctx.sl[slot] |= (m_opn2_ctx.sl[slot] + 1) & 0x10;
          break;
        case 0x90: // SSG-EG.
          m_opn2_ctx.ssg_eg[slot] = m_opn2_ctx.data & 0x0f;
          break;
        default:
          break;
      }
    }

    // Channel.
    if (kChOffset[channel] == (m_opn2_ctx.address & 0x103)) {
      address = m_opn2_ctx.address & 0xfc;
      switch (address) {
        case 0xa0:
          m_opn2_ctx.fnum[channel] = (m_opn2_ctx.data & 0xff) | ((m_opn2_ctx.reg_a4 & 0x07) << 8);
          m_opn2_ctx.block[channel] = (m_opn2_ctx.reg_a4 >> 3) & 0x07;
          m_opn2_ctx.kcode[channel] = (m_opn2_ctx.block[channel] << 2) | kFnNote[m_opn2_ctx.fnum[channel] >> 7];
          break;
        case 0xa4:
          m_opn2_ctx.reg_a4 = m_opn2_ctx.data & 0xff;
          break;
        case 0xa8:
          m_opn2_ctx.fnum_3ch[channel] = (m_opn2_ctx.data & 0xff) | ((m_opn2_ctx.reg_ac & 0x07) << 8);
          m_opn2_ctx.block_3ch[channel] = (m_opn2_ctx.reg_ac >> 3) & 0x07;
          m_opn2_ctx.kcode_3ch[channel] = (m_opn2_ctx.block_3ch[channel] << 2) | kFnNote[m_opn2_ctx.fnum_3ch[channel] >> 7];
          break;
        case 0xac:
          m_opn2_ctx.reg_ac = m_opn2_ctx.data & 0xff;
          break;
        case 0xb0:
          m_opn2_ctx.connect[channel] = m_opn2_ctx.data & 0x07;
          m_opn2_ctx.fb[channel] = (m_opn2_ctx.data >> 3) & 0x07;
          break;
        case 0xb4:
          m_opn2_ctx.pms[channel] = m_opn2_ctx.data & 0x07;
          m_opn2_ctx.ams[channel] = (m_opn2_ctx.data >> 4) & 0x03;
          m_opn2_ctx.pan_l[channel] = (m_opn2_ctx.data >> 7) & 0x01;
          m_opn2_ctx.pan_r[channel] = (m_opn2_ctx.data >> 6) & 0x01;
          break;
        default:
          break;
      }
    }
  }

  if (m_opn2_ctx.write_a_en || m_opn2_ctx.write_d_en) {
    // Data.
    if (m_opn2_ctx.write_a_en) {
      m_opn2_ctx.write_fm_data = 0;
    }

    if (m_opn2_ctx.write_fm_address && m_opn2_ctx.write_d_en) {
      m_opn2_ctx.write_fm_data = 1;
    }

    // Address.
    if (m_opn2_ctx.write_a_en) {
      if ((m_opn2_ctx.write_data & 0xf0) != 0x00) {
        // FM Write.
        m_opn2_ctx.address = m_opn2_ctx.write_data;
        m_opn2_ctx.write_fm_address = 1;
      } else {
        // SSG write.
        m_opn2_ctx.write_fm_address = 0;
      }
    }

    // FM Mode.
    // Data.
    if (m_opn2_ctx.write_d_en && (m_opn2_ctx.write_data & 0x100) == 0) {
      switch (m_opn2_ctx.write_fm_mode_a) {
        case 0x21: // LSI test 1.
          for (i = 0; i < 8; i++) {
            m_opn2_ctx.mode_test_21[i] = (m_opn2_ctx.write_data >> i) & 0x01;
          }
          break;
        case 0x22: // LFO control.
          if ((m_opn2_ctx.write_data >> 3) & 0x01) {
            m_opn2_ctx.lfo_en = 0x7f;
          } else {
            m_opn2_ctx.lfo_en = 0;
          }
          m_opn2_ctx.lfo_freq = m_opn2_ctx.write_data & 0x07;
          break;
        case 0x24: // Timer A.
          m_opn2_ctx.timer_a_reg &= 0x03;
          m_opn2_ctx.timer_a_reg |= (m_opn2_ctx.write_data & 0xff) << 2;
          break;
        case 0x25:
          m_opn2_ctx.timer_a_reg &= 0x3fc;
          m_opn2_ctx.timer_a_reg |= m_opn2_ctx.write_data & 0x03;
          break;
        case 0x26: // Timer B.
          m_opn2_ctx.timer_b_reg = m_opn2_ctx.write_data & 0xff;
          break;
        case 0x27: // CSM, Timer control.
          m_opn2_ctx.mode_ch3 = (m_opn2_ctx.write_data & 0xc0) >> 6;
          m_opn2_ctx.mode_csm = m_opn2_ctx.mode_ch3 == 2;
          m_opn2_ctx.timer_a_load = m_opn2_ctx.write_data & 0x01;
          m_opn2_ctx.timer_a_enable = (m_opn2_ctx.write_data >> 2) & 0x01;
          m_opn2_ctx.timer_a_reset = (m_opn2_ctx.write_data >> 4) & 0x01;
          m_opn2_ctx.timer_b_load = (m_opn2_ctx.write_data >> 1) & 0x01;
          m_opn2_ctx.timer_b_enable = (m_opn2_ctx.write_data >> 3) & 0x01;
          m_opn2_ctx.timer_b_reset = (m_opn2_ctx.write_data >> 5) & 0x01;
          break;
        case 0x28: // Key on/off.
          for (i = 0; i < 4; i++) {
            m_opn2_ctx.mode_kon_operator[i] = (m_opn2_ctx.write_data >> (4 + i)) & 0x01;
          }
          if ((m_opn2_ctx.write_data & 0x03) == 0x03) {
            // Invalid address.
            m_opn2_ctx.mode_kon_channel = 0xff;
          } else {
            m_opn2_ctx.mode_kon_channel = (m_opn2_ctx.write_data & 0x03) + ((m_opn2_ctx.write_data >> 2) & 1) * 3;
          }
          break;
        case 0x2a: // DAC data.
          m_opn2_ctx.dacdata &= 0x01;
          m_opn2_ctx.dacdata |= (m_opn2_ctx.write_data ^ 0x80) << 1;
          break;
        case 0x2b: // DAC enable.
          m_opn2_ctx.dacen = m_opn2_ctx.write_data >> 7;
          break;
        case 0x2c: // LSI test 2.
          for (i = 0; i < 8; i++) {
            m_opn2_ctx.mode_test_2c[i] = (m_opn2_ctx.write_data >> i) & 0x01;
          }
          m_opn2_ctx.dacdata &= 0x1fe;
          m_opn2_ctx.dacdata |= m_opn2_ctx.mode_test_2c[3];
          m_opn2_ctx.eg_custom_timer = !m_opn2_ctx.mode_test_2c[7] && m_opn2_ctx.mode_test_2c[6];
          break;
        default:
          break;
      }
    }

    // Address.
    if (m_opn2_ctx.write_a_en) {
      m_opn2_ctx.write_fm_mode_a = m_opn2_ctx.write_data & 0x1ff;
    }
  }

  if (m_opn2_ctx.write_fm_data) {
    m_opn2_ctx.data = m_opn2_ctx.write_data & 0xff;
  }
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_PhaseCalcIncrement()
{
  u32 chan = m_opn2_ctx.channel;
  u32 slot = m_opn2_ctx.cycles;
  u32 fnum = m_opn2_ctx.pg_fnum;
  u32 fnum_h = fnum >> 4;
  u32 fm;
  u32 basefreq;
  u8 lfo = m_opn2_ctx.lfo_pm;
  u8 lfo_l = lfo & 0x0f;
  u8 pms = m_opn2_ctx.pms[chan];
  u8 dt = m_opn2_ctx.dt[slot];
  u8 dt_l = dt & 0x03;
  u8 detune = 0;
  u8 block, note;
  u8 sum, sum_h, sum_l;
  u8 kcode = m_opn2_ctx.pg_kcode;

  fnum <<= 1;
  // Apply LFO.
  if (lfo_l & 0x08) {
    lfo_l ^= 0x0f;
  }
  fm = (fnum_h >> kPgLfoSh1[pms][lfo_l]) + (fnum_h >> kPgLfoSh2[pms][lfo_l]);
  if (pms > 5) {
    fm <<= pms - 5;
  }
  fm >>= 2;
  if (lfo & 0x10) {
    fnum -= fm;
  } else {
    fnum += fm;
  }
  fnum &= 0xfff;

  basefreq = (fnum << m_opn2_ctx.pg_block) >> 2;

  // Apply detune.
  if (dt_l) {
    if (kcode > 0x1c) {
      kcode = 0x1c;
    }
    block = kcode >> 2;
    note = kcode & 0x03;
    sum = block + 9 + ((dt_l == 3) | (dt_l & 0x02));
    sum_h = sum >> 1;
    sum_l = sum & 0x01;
    detune = kPgDetune[(sum_l << 2) | note] >> (9 - sum_h);
  }
  if (dt & 0x04) {
    basefreq -= detune;
  } else {
    basefreq += detune;
  }
  basefreq &= 0x1ffff;
  m_opn2_ctx.pg_inc[slot] = (basefreq * m_opn2_ctx.multi[slot]) >> 1;
  m_opn2_ctx.pg_inc[slot] &= 0xfffff;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_PhaseGenerate()
{
  u32 slot;
  // Mask increment.
  slot = (m_opn2_ctx.cycles + 20) % 24;
  if (m_opn2_ctx.pg_reset[slot]) {
    m_opn2_ctx.pg_inc[slot] = 0;
  }
  // Phase step.
  slot = (m_opn2_ctx.cycles + 19) % 24;
  if (m_opn2_ctx.pg_reset[slot] || m_opn2_ctx.mode_test_21[3]) {
    m_opn2_ctx.pg_phase[slot] = 0;
  }
  m_opn2_ctx.pg_phase[slot] += m_opn2_ctx.pg_inc[slot];
  m_opn2_ctx.pg_phase[slot] &= 0xfffff;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_EnvelopeSSGEG()
{
  u32 slot = m_opn2_ctx.cycles;
  u8 direction = 0;
  m_opn2_ctx.eg_ssg_pgrst_latch[slot] = 0;
  m_opn2_ctx.eg_ssg_repeat_latch[slot] = 0;
  m_opn2_ctx.eg_ssg_hold_up_latch[slot] = 0;
  if (m_opn2_ctx.ssg_eg[slot] & 0x08) {
    direction = m_opn2_ctx.eg_ssg_dir[slot];
    if (m_opn2_ctx.eg_level[slot] & 0x200) {
      // Reset.
      if ((m_opn2_ctx.ssg_eg[slot] & 0x03) == 0x00) {
        m_opn2_ctx.eg_ssg_pgrst_latch[slot] = 1;
      }
      // Repeat.
      if ((m_opn2_ctx.ssg_eg[slot] & 0x01) == 0x00) {
        m_opn2_ctx.eg_ssg_repeat_latch[slot] = 1;
      }
      // Inverse.
      if ((m_opn2_ctx.ssg_eg[slot] & 0x03) == 0x02) {
        direction ^= 1;
      }
      if ((m_opn2_ctx.ssg_eg[slot] & 0x03) == 0x03) {
        direction = 1;
      }
    }
    // Hold up.
    if (m_opn2_ctx.eg_kon_latch[slot]
      && ((m_opn2_ctx.ssg_eg[slot] & 0x07) == 0x05 || (m_opn2_ctx.ssg_eg[slot] & 0x07) == 0x03)) {
      m_opn2_ctx.eg_ssg_hold_up_latch[slot] = 1;
    }
    direction &= m_opn2_ctx.eg_kon[slot];
  }
  m_opn2_ctx.eg_ssg_dir[slot] = direction;
  m_opn2_ctx.eg_ssg_enable[slot] = (m_opn2_ctx.ssg_eg[slot] >> 3) & 0x01;
  m_opn2_ctx.eg_ssg_inv[slot] = (m_opn2_ctx.eg_ssg_dir[slot] ^ (((m_opn2_ctx.ssg_eg[slot] >> 2) & 0x01) & ((m_opn2_ctx.ssg_eg[slot] >> 3) & 0x01)))
    & m_opn2_ctx.eg_kon[slot];
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_EnvelopeADSR()
{
  u32 slot = (m_opn2_ctx.cycles + 22) % 24;

  u8 nkon = m_opn2_ctx.eg_kon_latch[slot];
  u8 okon = m_opn2_ctx.eg_kon[slot];
  u8 kon_event;
  u8 koff_event;
  u8 eg_off;
  s16 level;
  s16 nextlevel = 0;
  s16 ssg_level;
  u8 nextstate = m_opn2_ctx.eg_state[slot];
  s16 inc = 0;
  m_opn2_ctx.eg_read[0] = m_opn2_ctx.eg_read_inc;
  m_opn2_ctx.eg_read_inc = m_opn2_ctx.eg_inc > 0;

  // Reset phase generator.
  m_opn2_ctx.pg_reset[slot] = (nkon && !okon) || m_opn2_ctx.eg_ssg_pgrst_latch[slot];

  // KeyOn/Off.
  kon_event = (nkon && !okon) || (okon && m_opn2_ctx.eg_ssg_repeat_latch[slot]);
  koff_event = okon && !nkon;

  ssg_level = level = (s16)m_opn2_ctx.eg_level[slot];

  if (m_opn2_ctx.eg_ssg_inv[slot]) {
    // Inverse.
    ssg_level = 512 - level;
    ssg_level &= 0x3ff;
  }
  if (koff_event) {
    level = ssg_level;
  }
  if (m_opn2_ctx.eg_ssg_enable[slot]) {
    eg_off = level >> 9;
  } else {
    eg_off = (level & 0x3f0) == 0x3f0;
  }
  nextlevel = level;
  if (kon_event) {
    nextstate = eg_num_attack;
    // Instant attack.
    if (m_opn2_ctx.eg_ratemax) {
      nextlevel = 0;
    } else if (m_opn2_ctx.eg_state[slot] == eg_num_attack && level != 0 && m_opn2_ctx.eg_inc && nkon) {
      inc = (~level << m_opn2_ctx.eg_inc) >> 5;
    }
  } else {
    switch (m_opn2_ctx.eg_state[slot]) {
      case eg_num_attack:
        if (level == 0) {
          nextstate = eg_num_decay;
        } else if (m_opn2_ctx.eg_inc && !m_opn2_ctx.eg_ratemax && nkon) {
          inc = (~level << m_opn2_ctx.eg_inc) >> 5;
        }
        break;
      case eg_num_decay:
        if ((level >> 4) == (m_opn2_ctx.eg_sl[1] << 1)) {
          nextstate = eg_num_sustain;
        } else if (!eg_off && m_opn2_ctx.eg_inc) {
          inc = 1 << (m_opn2_ctx.eg_inc - 1);
          if (m_opn2_ctx.eg_ssg_enable[slot]) {
            inc <<= 2;
          }
        }
        break;
      case eg_num_sustain:
      case eg_num_release:
        if (!eg_off && m_opn2_ctx.eg_inc) {
          inc = 1 << (m_opn2_ctx.eg_inc - 1);
          if (m_opn2_ctx.eg_ssg_enable[slot]) {
            inc <<= 2;
          }
        }
        break;
      default:
        break;
    }
    if (!nkon) {
      nextstate = eg_num_release;
    }
  }
  if (m_opn2_ctx.eg_kon_csm[slot]) {
    nextlevel |= m_opn2_ctx.eg_tl[1] << 3;
  }

  // Envelope off.
  if (!kon_event && !m_opn2_ctx.eg_ssg_hold_up_latch[slot] && m_opn2_ctx.eg_state[slot] != eg_num_attack && eg_off) {
    nextstate = eg_num_release;
    nextlevel = 0x3ff;
  }

  nextlevel += inc;

  m_opn2_ctx.eg_kon[slot] = m_opn2_ctx.eg_kon_latch[slot];
  m_opn2_ctx.eg_level[slot] = (u16)nextlevel & 0x3ff;
  m_opn2_ctx.eg_state[slot] = nextstate;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_EnvelopePrepare()
{
  u8 rate;
  u8 sum;
  u8 inc = 0;
  u32 slot = m_opn2_ctx.cycles;
  u8 rate_sel;

  // Prepare increment.
  rate = (m_opn2_ctx.eg_rate << 1) + m_opn2_ctx.eg_ksv;

  if (rate > 0x3f) {
    rate = 0x3f;
  }

  sum = ((rate >> 2) + m_opn2_ctx.eg_shift_lock) & 0x0f;
  if (m_opn2_ctx.eg_rate != 0 && m_opn2_ctx.eg_quotient == 2) {
    if (rate < 48) {
      switch (sum) {
        case 12:
          inc = 1;
          break;
        case 13:
          inc = (rate >> 1) & 0x01;
          break;
        case 14:
          inc = rate & 0x01;
          break;
        default:
          break;
      }
    } else {
      inc = kEgStephi[rate & 0x03][m_opn2_ctx.eg_timer_low_lock] + (rate >> 2) - 11;
      if (inc > 4) {
        inc = 4;
      }
    }
  }
  m_opn2_ctx.eg_inc = inc;
  m_opn2_ctx.eg_ratemax = (rate >> 1) == 0x1f;

  // Prepare rate & ksv.
  rate_sel = m_opn2_ctx.eg_state[slot];
  if ((m_opn2_ctx.eg_kon[slot] && m_opn2_ctx.eg_ssg_repeat_latch[slot])
    || (!m_opn2_ctx.eg_kon[slot] && m_opn2_ctx.eg_kon_latch[slot])) {
    rate_sel = eg_num_attack;
  }
  switch (rate_sel) {
    case eg_num_attack:
      m_opn2_ctx.eg_rate = m_opn2_ctx.ar[slot];
      break;
    case eg_num_decay:
      m_opn2_ctx.eg_rate = m_opn2_ctx.dr[slot];
      break;
    case eg_num_sustain:
      m_opn2_ctx.eg_rate = m_opn2_ctx.sr[slot];
      break;
    case eg_num_release:
      m_opn2_ctx.eg_rate = (m_opn2_ctx.rr[slot] << 1) | 0x01;
      break;
    default:
      break;
  }
  m_opn2_ctx.eg_ksv = m_opn2_ctx.pg_kcode >> (m_opn2_ctx.ks[slot] ^ 0x03);
  if (m_opn2_ctx.am[slot]) {
    m_opn2_ctx.eg_lfo_am = m_opn2_ctx.lfo_am >> kEgAmShift[m_opn2_ctx.ams[m_opn2_ctx.channel]];
  } else {
    m_opn2_ctx.eg_lfo_am = 0;
  }
  // Delay TL & SL value.
  m_opn2_ctx.eg_tl[1] = m_opn2_ctx.eg_tl[0];
  m_opn2_ctx.eg_tl[0] = m_opn2_ctx.tl[slot];
  m_opn2_ctx.eg_sl[1] = m_opn2_ctx.eg_sl[0];
  m_opn2_ctx.eg_sl[0] = m_opn2_ctx.sl[slot];
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_EnvelopeGenerate()
{
  u32 slot = (m_opn2_ctx.cycles + 23) % 24;
  u16 level;

  level = m_opn2_ctx.eg_level[slot];

  if (m_opn2_ctx.eg_ssg_inv[slot]) {
    // Inverse.
    level = 512 - level;
  }
  if (m_opn2_ctx.mode_test_21[5]) {
    level = 0;
  }
  level &= 0x3ff;

  // Apply AM LFO.
  level += m_opn2_ctx.eg_lfo_am;

  // Apply TL.
  if (!(m_opn2_ctx.mode_csm && m_opn2_ctx.channel == 2 + 1)) {
    level += m_opn2_ctx.eg_tl[0] << 3;
  }
  if (level > 0x3ff) {
    level = 0x3ff;
  }
  m_opn2_ctx.eg_out[slot] = level;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_UpdateLFO()
{
  if ((m_opn2_ctx.lfo_quotient & kLfoCycles[m_opn2_ctx.lfo_freq]) == kLfoCycles[m_opn2_ctx.lfo_freq]) {
    m_opn2_ctx.lfo_quotient = 0;
    m_opn2_ctx.lfo_cnt++;
  } else {
    m_opn2_ctx.lfo_quotient += m_opn2_ctx.lfo_inc;
  }
  m_opn2_ctx.lfo_cnt &= m_opn2_ctx.lfo_en;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_FMPrepare()
{
  u32 slot = (m_opn2_ctx.cycles + 6) % 24;
  u32 channel = m_opn2_ctx.channel;
  s16 mod, mod1, mod2;
  u32 op = slot / 6;
  u8 connect = m_opn2_ctx.connect[channel];
  u32 prevslot = (m_opn2_ctx.cycles + 18) % 24;

  // Calculate modulation.
  mod1 = mod2 = 0;

  if (kFmAlgorithm[op][0][connect]) {
    mod2 |= m_opn2_ctx.fm_op1[channel][0];
  }
  if (kFmAlgorithm[op][1][connect]) {
    mod1 |= m_opn2_ctx.fm_op1[channel][1];
  }
  if (kFmAlgorithm[op][2][connect]) {
    mod1 |= m_opn2_ctx.fm_op2[channel];
  }
  if (kFmAlgorithm[op][3][connect]) {
    mod2 |= m_opn2_ctx.fm_out[prevslot];
  }
  if (kFmAlgorithm[op][4][connect]) {
    mod1 |= m_opn2_ctx.fm_out[prevslot];
  }
  mod = mod1 + mod2;
  if (op == 0) {
    // Feedback.
    mod = mod >> (10 - m_opn2_ctx.fb[channel]);
    if (!m_opn2_ctx.fb[channel]) {
      mod = 0;
    }
  } else {
    mod >>= 1;
  }
  m_opn2_ctx.fm_mod[slot] = mod;

  slot = (m_opn2_ctx.cycles + 18) % 24;
  // OP1.
  if (slot / 6 == 0) {
    m_opn2_ctx.fm_op1[channel][1] = m_opn2_ctx.fm_op1[channel][0];
    m_opn2_ctx.fm_op1[channel][0] = m_opn2_ctx.fm_out[slot];
  }
  // OP2.
  if (slot / 6 == 2) {
    m_opn2_ctx.fm_op2[channel] = m_opn2_ctx.fm_out[slot];
  }
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_ChGenerate()
{
  u32 slot = (m_opn2_ctx.cycles + 18) % 24;
  u32 channel = m_opn2_ctx.channel;
  u32 op = slot / 6;
  u32 test_dac = m_opn2_ctx.mode_test_2c[5];
  s16 acc = m_opn2_ctx.ch_acc[channel];
  s16 add = test_dac;
  s16 sum = 0;
  if (op == 0 && !test_dac) {
    acc = 0;
  }
  if (kFmAlgorithm[op][5][m_opn2_ctx.connect[channel]] && !test_dac) {
    add += m_opn2_ctx.fm_out[slot] >> 5;
  }
  sum = acc + add;
  // Clamp.
  if (sum > 255) {
    sum = 255;
  } else if (sum < -256) {
    sum = -256;
  }

  if (op == 0 || test_dac) {
    m_opn2_ctx.ch_out[channel] = m_opn2_ctx.ch_acc[channel];
  }
  m_opn2_ctx.ch_acc[channel] = sum;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_ChOutput()
{
  u32 cycles = m_opn2_ctx.cycles;
  u32 slot = m_opn2_ctx.cycles;
  u32 channel = m_opn2_ctx.channel;
  u32 test_dac = m_opn2_ctx.mode_test_2c[5];
  s16 out;
  s16 sign;
  u32 out_en;
  m_opn2_ctx.ch_read = m_opn2_ctx.ch_lock;
  if (slot < 12) {
    // Ch 4,5,6.
    channel++;
  }
  if ((cycles & 3) == 0) {
    if (!test_dac) {
      // Lock value.
      m_opn2_ctx.ch_lock = m_opn2_ctx.ch_out[channel];
    }
    m_opn2_ctx.ch_lock_l = m_opn2_ctx.pan_l[channel];
    m_opn2_ctx.ch_lock_r = m_opn2_ctx.pan_r[channel];
  }
  // Ch 6.
  if (((cycles >> 2) == 1 && m_opn2_ctx.dacen) || test_dac) {
    out = (s16)m_opn2_ctx.dacdata;
    out = SIGN_EXTEND(8, out);
  } else {
    out = m_opn2_ctx.ch_lock;
  }
  m_opn2_ctx.mol = 0;
  m_opn2_ctx.mor = 0;

  if (m_chip_type & ym3438_mode_ym2612) {
    out_en = ((cycles & 3) == 3) || test_dac;
    // YM2612 DAC emulation(not verified).
    sign = out >> 8;
    if (out >= 0) {
      out++;
      sign++;
    }
    if (m_opn2_ctx.ch_lock_l && out_en) {
      m_opn2_ctx.mol = out;
    } else {
      m_opn2_ctx.mol = sign;
    }
    if (m_opn2_ctx.ch_lock_r && out_en) {
      m_opn2_ctx.mor = out;
    } else {
      m_opn2_ctx.mor = sign;
    }
    // Amplify signal.
    m_opn2_ctx.mol *= 3;
    m_opn2_ctx.mor *= 3;
  } else {
    out_en = ((cycles & 3) != 0) || test_dac;
    if (m_opn2_ctx.ch_lock_l && out_en) {
      m_opn2_ctx.mol = out;
    }
    if (m_opn2_ctx.ch_lock_r && out_en) {
      m_opn2_ctx.mor = out;
    }
  }
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_FMGenerate()
{
  u32 slot = (m_opn2_ctx.cycles + 19) % 24;
  // Calculate phase.
  u16 phase = (m_opn2_ctx.fm_mod[slot] + (m_opn2_ctx.pg_phase[slot] >> 10)) & 0x3ff;
  u16 quarter;
  u16 level;
  s16 output;
  if (phase & 0x100) {
    quarter = (phase ^ 0xff) & 0xff;
  } else {
    quarter = phase & 0xff;
  }
  level = kLogSinTable[quarter];
  // Apply envelope.
  level += m_opn2_ctx.eg_out[slot] << 2;
  // Transform.
  if (level > 0x1fff) {
    level = 0x1fff;
  }
  output = ((kExpTable[(level & 0xff) ^ 0xff] | 0x400) << 2) >> (level >> 8);
  if (phase & 0x200) {
    output = ((~output) ^ (m_opn2_ctx.mode_test_21[4] << 13)) + 1;
  } else {
    output = output ^ (m_opn2_ctx.mode_test_21[4] << 13);
  }
  output = SIGN_EXTEND(13, output);
  m_opn2_ctx.fm_out[slot] = output;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_DoTimerA()
{
  u16 time;
  u8 load;
  load = m_opn2_ctx.timer_a_overflow;
  if (m_opn2_ctx.cycles == 2) {
    // Lock load value.
    load |= (!m_opn2_ctx.timer_a_load_lock && m_opn2_ctx.timer_a_load);
    m_opn2_ctx.timer_a_load_lock = m_opn2_ctx.timer_a_load;
    if (m_opn2_ctx.mode_csm) {
      // CSM KeyOn.
      m_opn2_ctx.mode_kon_csm = load;
    } else {
      m_opn2_ctx.mode_kon_csm = 0;
    }
  }
  // Load counter.
  if (m_opn2_ctx.timer_a_load_latch) {
    time = m_opn2_ctx.timer_a_reg;
  } else {
    time = m_opn2_ctx.timer_a_cnt;
  }
  m_opn2_ctx.timer_a_load_latch = load;
  // Increase counter.
  if ((m_opn2_ctx.cycles == 1 && m_opn2_ctx.timer_a_load_lock) || m_opn2_ctx.mode_test_21[2]) {
    time++;
  }
  // Set overflow flag.
  if (m_opn2_ctx.timer_a_reset) {
    m_opn2_ctx.timer_a_reset = 0;
    m_opn2_ctx.timer_a_overflow_flag = 0;
  } else {
    m_opn2_ctx.timer_a_overflow_flag |= m_opn2_ctx.timer_a_overflow & m_opn2_ctx.timer_a_enable;
  }
  m_opn2_ctx.timer_a_overflow = (time >> 10);
  m_opn2_ctx.timer_a_cnt = time & 0x3ff;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_DoTimerB()
{
  u16 time;
  u8 load;
  load = m_opn2_ctx.timer_b_overflow;
  if (m_opn2_ctx.cycles == 2) {
    // Lock load value.
    load |= (!m_opn2_ctx.timer_b_load_lock && m_opn2_ctx.timer_b_load);
    m_opn2_ctx.timer_b_load_lock = m_opn2_ctx.timer_b_load;
  }
  // Load counter.
  if (m_opn2_ctx.timer_b_load_latch) {
    time = m_opn2_ctx.timer_b_reg;
  } else {
    time = m_opn2_ctx.timer_b_cnt;
  }
  m_opn2_ctx.timer_b_load_latch = load;
  // Increase counter.
  if (m_opn2_ctx.cycles == 1) {
    m_opn2_ctx.timer_b_subcnt++;
  }
  if ((m_opn2_ctx.timer_b_subcnt == 0x10 && m_opn2_ctx.timer_b_load_lock) || m_opn2_ctx.mode_test_21[2]) {
    time++;
  }
  m_opn2_ctx.timer_b_subcnt &= 0x0f;
  // Set overflow flag.
  if (m_opn2_ctx.timer_b_reset) {
    m_opn2_ctx.timer_b_reset = 0;
    m_opn2_ctx.timer_b_overflow_flag = 0;
  } else {
    m_opn2_ctx.timer_b_overflow_flag |= m_opn2_ctx.timer_b_overflow & m_opn2_ctx.timer_b_enable;
  }
  m_opn2_ctx.timer_b_overflow = (time >> 8);
  m_opn2_ctx.timer_b_cnt = time & 0xff;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_KeyOn()
{
  u32 slot = m_opn2_ctx.cycles;
  u32 chan = m_opn2_ctx.channel;
  // Key On.
  m_opn2_ctx.eg_kon_latch[slot] = m_opn2_ctx.mode_kon[slot];
  m_opn2_ctx.eg_kon_csm[slot] = 0;
  if (m_opn2_ctx.channel == 2 && m_opn2_ctx.mode_kon_csm) {
    // CSM Key On.
    m_opn2_ctx.eg_kon_latch[slot] = 1;
    m_opn2_ctx.eg_kon_csm[slot] = 1;
  }
  if (m_opn2_ctx.cycles == m_opn2_ctx.mode_kon_channel) {
    // OP1.
    m_opn2_ctx.mode_kon[chan] = m_opn2_ctx.mode_kon_operator[0];
    // OP2.
    m_opn2_ctx.mode_kon[chan + 12] = m_opn2_ctx.mode_kon_operator[1];
    // OP3.
    m_opn2_ctx.mode_kon[chan + 6] = m_opn2_ctx.mode_kon_operator[2];
    // OP4.
    m_opn2_ctx.mode_kon[chan + 18] = m_opn2_ctx.mode_kon_operator[3];
  }
}

//------------------------------------------------------------------------------

Ym3438::Ym3438()
{
  m_chip_type = ym3438_mode_readmode;

  xee::mem::Memset(&m_opn2_ctx, 0, sizeof(m_opn2_ctx));
  xee::mem::Memset(&m_accm, 0, sizeof(m_accm));
  xee::mem::Memset(&m_sample, 0, sizeof(m_sample));
  m_cycles = 0;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_Reset()
{
  xee::mem::Memset(&m_opn2_ctx, 0, sizeof(m_opn2_ctx));

  u32 i;

  for (i = 0; i < 24; i++) {
    m_opn2_ctx.eg_out[i] = 0x3ff;
    m_opn2_ctx.eg_level[i] = 0x3ff;
    m_opn2_ctx.eg_state[i] = eg_num_release;
    m_opn2_ctx.multi[i] = 1;
  }

  for (i = 0; i < 6; i++) {
    m_opn2_ctx.pan_l[i] = 1;
    m_opn2_ctx.pan_r[i] = 1;
  }
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_SetChipType(u32 type)
{
  m_chip_type = type;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_Clock(s16* buffer)
{
  u32 slot = m_opn2_ctx.cycles;
  m_opn2_ctx.lfo_inc = m_opn2_ctx.mode_test_21[1];
  m_opn2_ctx.pg_read >>= 1;
  m_opn2_ctx.eg_read[1] >>= 1;
  m_opn2_ctx.eg_cycle++;

  // Lock envelope generator timer value.
  if (m_opn2_ctx.cycles == 1 && m_opn2_ctx.eg_quotient == 2) {
    if (m_opn2_ctx.eg_cycle_stop) {
      m_opn2_ctx.eg_shift_lock = 0;
    } else {
      m_opn2_ctx.eg_shift_lock = m_opn2_ctx.eg_shift + 1;
    }

    m_opn2_ctx.eg_timer_low_lock = m_opn2_ctx.eg_timer & 0x03;
  }

  // Cycle specific functions.
  switch (m_opn2_ctx.cycles) {
    case 0:
      m_opn2_ctx.lfo_pm = m_opn2_ctx.lfo_cnt >> 2;
      if (m_opn2_ctx.lfo_cnt & 0x40) {
        m_opn2_ctx.lfo_am = m_opn2_ctx.lfo_cnt & 0x3f;
      } else {
        m_opn2_ctx.lfo_am = m_opn2_ctx.lfo_cnt ^ 0x3f;
      }
      m_opn2_ctx.lfo_am <<= 1;
      break;
    case 1:
      m_opn2_ctx.eg_quotient++;
      m_opn2_ctx.eg_quotient %= 3;
      m_opn2_ctx.eg_cycle = 0;
      m_opn2_ctx.eg_cycle_stop = 1;
      m_opn2_ctx.eg_shift = 0;
      m_opn2_ctx.eg_timer_inc |= m_opn2_ctx.eg_quotient >> 1;
      m_opn2_ctx.eg_timer = m_opn2_ctx.eg_timer + m_opn2_ctx.eg_timer_inc;
      m_opn2_ctx.eg_timer_inc = m_opn2_ctx.eg_timer >> 12;
      m_opn2_ctx.eg_timer &= 0xfff;
      break;
    case 2:
      m_opn2_ctx.pg_read = m_opn2_ctx.pg_phase[21] & 0x3ff;
      m_opn2_ctx.eg_read[1] = m_opn2_ctx.eg_out[0];
      break;
    case 13:
      m_opn2_ctx.eg_cycle = 0;
      m_opn2_ctx.eg_cycle_stop = 1;
      m_opn2_ctx.eg_shift = 0;
      m_opn2_ctx.eg_timer = m_opn2_ctx.eg_timer + m_opn2_ctx.eg_timer_inc;
      m_opn2_ctx.eg_timer_inc = m_opn2_ctx.eg_timer >> 12;
      m_opn2_ctx.eg_timer &= 0xfff;
      break;
    case 23:
      m_opn2_ctx.lfo_inc |= 1;
      break;
  }

  m_opn2_ctx.eg_timer &= ~(m_opn2_ctx.mode_test_21[5] << m_opn2_ctx.eg_cycle);

  if (((m_opn2_ctx.eg_timer >> m_opn2_ctx.eg_cycle) | (m_opn2_ctx.pin_test_in & m_opn2_ctx.eg_custom_timer)) & m_opn2_ctx.eg_cycle_stop) {
    m_opn2_ctx.eg_shift = m_opn2_ctx.eg_cycle;
    m_opn2_ctx.eg_cycle_stop = 0;
  }

  OPN2_DoIO();

  OPN2_DoTimerA();
  OPN2_DoTimerB();
  OPN2_KeyOn();

  OPN2_ChOutput();
  OPN2_ChGenerate();

  OPN2_FMPrepare();
  OPN2_FMGenerate();

  OPN2_PhaseGenerate();
  OPN2_PhaseCalcIncrement();

  OPN2_EnvelopeADSR();
  OPN2_EnvelopeGenerate();
  OPN2_EnvelopeSSGEG();
  OPN2_EnvelopePrepare();

  // Prepare fnum & block.
  if (m_opn2_ctx.mode_ch3) {
    // Channel 3 special mode.
    switch (slot) {
      case 1: // OP1.
        m_opn2_ctx.pg_fnum = m_opn2_ctx.fnum_3ch[1];
        m_opn2_ctx.pg_block = m_opn2_ctx.block_3ch[1];
        m_opn2_ctx.pg_kcode = m_opn2_ctx.kcode_3ch[1];
        break;
      case 7: // OP3.
        m_opn2_ctx.pg_fnum = m_opn2_ctx.fnum_3ch[0];
        m_opn2_ctx.pg_block = m_opn2_ctx.block_3ch[0];
        m_opn2_ctx.pg_kcode = m_opn2_ctx.kcode_3ch[0];
        break;
      case 13: // OP2.
        m_opn2_ctx.pg_fnum = m_opn2_ctx.fnum_3ch[2];
        m_opn2_ctx.pg_block = m_opn2_ctx.block_3ch[2];
        m_opn2_ctx.pg_kcode = m_opn2_ctx.kcode_3ch[2];
        break;
      case 19: // OP4.
      default:
        m_opn2_ctx.pg_fnum = m_opn2_ctx.fnum[(m_opn2_ctx.channel + 1) % 6];
        m_opn2_ctx.pg_block = m_opn2_ctx.block[(m_opn2_ctx.channel + 1) % 6];
        m_opn2_ctx.pg_kcode = m_opn2_ctx.kcode[(m_opn2_ctx.channel + 1) % 6];
        break;
    }
  } else {
    m_opn2_ctx.pg_fnum = m_opn2_ctx.fnum[(m_opn2_ctx.channel + 1) % 6];
    m_opn2_ctx.pg_block = m_opn2_ctx.block[(m_opn2_ctx.channel + 1) % 6];
    m_opn2_ctx.pg_kcode = m_opn2_ctx.kcode[(m_opn2_ctx.channel + 1) % 6];
  }

  OPN2_UpdateLFO();
  OPN2_DoRegWrite();
  m_opn2_ctx.cycles = (m_opn2_ctx.cycles + 1) % 24;
  m_opn2_ctx.channel = m_opn2_ctx.cycles % 6;

  buffer[0] = m_opn2_ctx.mol;
  buffer[1] = m_opn2_ctx.mor;

  if (m_opn2_ctx.status_time)
    m_opn2_ctx.status_time--;
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_Write(u32 port, u8 data)
{
  port &= 3;
  m_opn2_ctx.write_data = ((port << 7) & 0x100) | data;

  if (port & 1) {
    // Data.
    m_opn2_ctx.write_d |= 1;
  } else {
    // Address.
    m_opn2_ctx.write_a |= 1;
  }
}

//------------------------------------------------------------------------------

void Ym3438::OPN2_SetTestPin(u32 value)
{
  m_opn2_ctx.pin_test_in = value & 1;
}

//------------------------------------------------------------------------------

u32 Ym3438::OPN2_ReadTestPin()
{
  if (!m_opn2_ctx.mode_test_2c[7]) {
    return 0;
  }

  return m_opn2_ctx.cycles == 23;
}

//------------------------------------------------------------------------------

u32 Ym3438::OPN2_ReadIRQPin()
{
  return m_opn2_ctx.timer_a_overflow_flag | m_opn2_ctx.timer_b_overflow_flag;
}

//------------------------------------------------------------------------------

u8 Ym3438::OPN2_Read(u32 port)
{
  if ((port & 3) == 0 || (m_chip_type & ym3438_mode_readmode)) {
    if (m_opn2_ctx.mode_test_21[6]) {
      // Read test data.
      u32 slot = (m_opn2_ctx.cycles + 18) % 24;
      u16 testdata = ((m_opn2_ctx.pg_read & 0x01) << 15)
        | ((m_opn2_ctx.eg_read[m_opn2_ctx.mode_test_21[0]] & 0x01) << 14);

      if (m_opn2_ctx.mode_test_2c[4]) {
        testdata |= m_opn2_ctx.ch_read & 0x1ff;
      } else {
        testdata |= m_opn2_ctx.fm_out[slot] & 0x3fff;
      }

      if (m_opn2_ctx.mode_test_21[7]) {
        m_opn2_ctx.status = testdata & 0xff;
      } else {
        m_opn2_ctx.status = testdata >> 8;
      }
    } else {
      m_opn2_ctx.status = (m_opn2_ctx.busy << 7) | (m_opn2_ctx.timer_b_overflow_flag << 1)
        | m_opn2_ctx.timer_a_overflow_flag;
    }
    if (m_chip_type & ym3438_mode_ym2612) {
      m_opn2_ctx.status_time = 300000;
    } else {
      m_opn2_ctx.status_time = 40000000;
    }
  }

  if (m_opn2_ctx.status_time) {
    return m_opn2_ctx.status;
  }

  return 0;
}

//------------------------------------------------------------------------------

void Ym3438::Init()
{
  xee::mem::Memset(&m_opn2_ctx, 0, sizeof(m_opn2_ctx));
  xee::mem::Memset(&m_accm, 0, sizeof(m_accm));
  xee::mem::Memset(&m_sample, 0, sizeof(m_sample));
  m_cycles = 0;
}

//------------------------------------------------------------------------------

// Synchronize FM chip with CPU and reset FM chip.
void Ym3438::SyncAndReset(unsigned int cycles)
{
  // synchronize FM chip with CPU.
  Update(cycles);

  // reset FM chip.
  OPN2_Reset();
}

//------------------------------------------------------------------------------

void Ym3438::Write(unsigned int cycles, unsigned int address, unsigned int data)
{
  // synchronize FM chip with CPU.
  Update(cycles);

  // write FM register.
  OPN2_Write(address, data);
}

//------------------------------------------------------------------------------

unsigned int Ym3438::Read(unsigned int cycles, unsigned int address)
{
  // synchronize FM chip with CPU.
  Update(cycles);

  // read FM status.
  return OPN2_Read(address);
}

//------------------------------------------------------------------------------

void Ym3438::UpdateSampleBuffer(int* buffer, int length)
{
  int j;

  for (int i = 0; i < length; i++) {
    OPN2_Clock(m_accm[m_cycles]);
    m_cycles = (m_cycles + 1) % kUpdateClock;

    if (m_cycles == 0) {
      m_sample[0] = 0;
      m_sample[1] = 0;

      for (j = 0; j < kUpdateClock; j++) {
        m_sample[0] += m_accm[j][0];
        m_sample[1] += m_accm[j][1];
      }
    }

    *buffer++ = m_sample[0] * kUpdateSampleAmp;
    *buffer++ = m_sample[1] * kUpdateSampleAmp;
  }
}

//------------------------------------------------------------------------------

int Ym3438::LoadChipContext(unsigned char* state)
{
  int bufferptr = 0;

  save_param(&m_opn2_ctx, sizeof(m_opn2_ctx));
  save_param(&m_accm, sizeof(m_accm));
  save_param(&m_sample, sizeof(m_sample));
  save_param(&m_cycles, sizeof(m_cycles));

  return bufferptr;
}

//------------------------------------------------------------------------------

int Ym3438::SaveChipContext(unsigned char* state)
{
  int bufferptr = 0;

  load_param(&m_opn2_ctx, sizeof(m_opn2_ctx));
  load_param(&m_accm, sizeof(m_accm));
  load_param(&m_sample, sizeof(m_sample));
  load_param(&m_cycles, sizeof(m_cycles));

  return bufferptr;
}

} // namespace gpgx::ic::ym3438

