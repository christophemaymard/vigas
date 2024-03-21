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

#include "core/sound/ym3438.h"

#include "xee/mem/memory.h"

#define SIGN_EXTEND(bit_index, value) (((value) & ((1u << (bit_index)) - 1u)) - ((value) & (1u << (bit_index))))

enum {
    eg_num_attack = 0,
    eg_num_decay = 1,
    eg_num_sustain = 2,
    eg_num_release = 3
};

/* logsin table */
static const u16 logsinrom[256] = {
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

/* exp table */
static const u16 exprom[256] = {
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

/* Note table */
static const u32 fn_note[16] = {
    0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3
};

/* Envelope generator */
static const u32 eg_stephi[4][4] = {
    { 0, 0, 0, 0 },
    { 1, 0, 0, 0 },
    { 1, 0, 1, 0 },
    { 1, 1, 1, 0 }
};

static const u8 eg_am_shift[4] = {
    7, 3, 1, 0
};

/* Phase generator */
static const u32 pg_detune[8] = { 16, 17, 19, 20, 22, 24, 27, 29 };

static const u32 pg_lfo_sh1[8][8] = {
    { 7, 7, 7, 7, 7, 7, 7, 7 },
    { 7, 7, 7, 7, 7, 7, 7, 7 },
    { 7, 7, 7, 7, 7, 7, 1, 1 },
    { 7, 7, 7, 7, 1, 1, 1, 1 },
    { 7, 7, 7, 1, 1, 1, 1, 0 },
    { 7, 7, 1, 1, 0, 0, 0, 0 },
    { 7, 7, 1, 1, 0, 0, 0, 0 },
    { 7, 7, 1, 1, 0, 0, 0, 0 }
};

static const u32 pg_lfo_sh2[8][8] = {
    { 7, 7, 7, 7, 7, 7, 7, 7 },
    { 7, 7, 7, 7, 2, 2, 2, 2 },
    { 7, 7, 7, 2, 2, 2, 7, 7 },
    { 7, 7, 2, 2, 7, 7, 2, 2 },
    { 7, 7, 2, 7, 7, 7, 2, 7 },
    { 7, 7, 7, 2, 7, 7, 2, 1 },
    { 7, 7, 7, 2, 7, 7, 2, 1 },
    { 7, 7, 7, 2, 7, 7, 2, 1 }
};

/* Address decoder */
static const u32 op_offset[12] = {
    0x000, /* Ch1 OP1/OP2 */
    0x001, /* Ch2 OP1/OP2 */
    0x002, /* Ch3 OP1/OP2 */
    0x100, /* Ch4 OP1/OP2 */
    0x101, /* Ch5 OP1/OP2 */
    0x102, /* Ch6 OP1/OP2 */
    0x004, /* Ch1 OP3/OP4 */
    0x005, /* Ch2 OP3/OP4 */
    0x006, /* Ch3 OP3/OP4 */
    0x104, /* Ch4 OP3/OP4 */
    0x105, /* Ch5 OP3/OP4 */
    0x106  /* Ch6 OP3/OP4 */
};

static const u32 ch_offset[6] = {
    0x000, /* Ch1 */
    0x001, /* Ch2 */
    0x002, /* Ch3 */
    0x100, /* Ch4 */
    0x101, /* Ch5 */
    0x102  /* Ch6 */
};

/* LFO */
static const u32 lfo_cycles[8] = {
    108, 77, 71, 67, 62, 44, 8, 5
};

/* FM algorithm */
static const u32 fm_algorithm[4][6][8] = {
    {
        { 1, 1, 1, 1, 1, 1, 1, 1 }, /* OP1_0         */
        { 1, 1, 1, 1, 1, 1, 1, 1 }, /* OP1_1         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP2           */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 1 }  /* Out           */
    },
    {
        { 0, 1, 0, 0, 0, 1, 0, 0 }, /* OP1_0         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_1         */
        { 1, 1, 1, 0, 0, 0, 0, 0 }, /* OP2           */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 1, 1, 1 }  /* Out           */
    },
    {
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_0         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_1         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP2           */
        { 1, 0, 0, 1, 1, 1, 1, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 1, 1, 1, 1 }  /* Out           */
    },
    {
        { 0, 0, 1, 0, 0, 1, 0, 0 }, /* OP1_0         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_1         */
        { 0, 0, 0, 1, 0, 0, 0, 0 }, /* OP2           */
        { 1, 1, 0, 1, 1, 0, 0, 0 }, /* Last operator */
        { 0, 0, 1, 0, 0, 0, 0, 0 }, /* Last operator */
        { 1, 1, 1, 1, 1, 1, 1, 1 }  /* Out           */
    }
};

static u32 chip_type = ym3438_mode_readmode;

static void OPN2_DoIO(ym3438_t *chip)
{
    /* Write signal check */
    chip->write_a_en = (chip->write_a & 0x03) == 0x01;
    chip->write_d_en = (chip->write_d & 0x03) == 0x01;
    chip->write_a <<= 1;
    chip->write_d <<= 1;
    /* Busy counter */
    chip->busy = chip->write_busy;
    chip->write_busy_cnt += chip->write_busy;
    chip->write_busy = (chip->write_busy && !(chip->write_busy_cnt >> 5)) || chip->write_d_en;
    chip->write_busy_cnt &= 0x1f;
}

static void OPN2_DoRegWrite(ym3438_t *chip)
{
    u32 i;
    u32 slot = chip->cycles % 12;
    u32 address;
    u32 channel = chip->channel;
    /* Update registers */
    if (chip->write_fm_data)
    {
        /* Slot */
        if (op_offset[slot] == (chip->address & 0x107))
        {
            if (chip->address & 0x08)
            {
                /* OP2, OP4 */
                slot += 12;
            }
            address = chip->address & 0xf0;
            switch (address)
            {
            case 0x30: /* DT, MULTI */
                chip->multi[slot] = chip->data & 0x0f;
                if (!chip->multi[slot])
                {
                    chip->multi[slot] = 1;
                }
                else
                {
                    chip->multi[slot] <<= 1;
                }
                chip->dt[slot] = (chip->data >> 4) & 0x07;
                break;
            case 0x40: /* TL */
                chip->tl[slot] = chip->data & 0x7f;
                break;
            case 0x50: /* KS, AR */
                chip->ar[slot] = chip->data & 0x1f;
                chip->ks[slot] = (chip->data >> 6) & 0x03;
                break;
            case 0x60: /* AM, DR */
                chip->dr[slot] = chip->data & 0x1f;
                chip->am[slot] = (chip->data >> 7) & 0x01;
                break;
            case 0x70: /* SR */
                chip->sr[slot] = chip->data & 0x1f;
                break;
            case 0x80: /* SL, RR */
                chip->rr[slot] = chip->data & 0x0f;
                chip->sl[slot] = (chip->data >> 4) & 0x0f;
                chip->sl[slot] |= (chip->sl[slot] + 1) & 0x10;
                break;
            case 0x90: /* SSG-EG */
                chip->ssg_eg[slot] = chip->data & 0x0f;
                break;
            default:
                break;
            }
        }

        /* Channel */
        if (ch_offset[channel] == (chip->address & 0x103))
        {
            address = chip->address & 0xfc;
            switch (address)
            {
            case 0xa0:
                chip->fnum[channel] = (chip->data & 0xff) | ((chip->reg_a4 & 0x07) << 8);
                chip->block[channel] = (chip->reg_a4 >> 3) & 0x07;
                chip->kcode[channel] = (chip->block[channel] << 2) | fn_note[chip->fnum[channel] >> 7];
                break;
            case 0xa4:
                chip->reg_a4 = chip->data & 0xff;
                break;
            case 0xa8:
                chip->fnum_3ch[channel] = (chip->data & 0xff) | ((chip->reg_ac & 0x07) << 8);
                chip->block_3ch[channel] = (chip->reg_ac >> 3) & 0x07;
                chip->kcode_3ch[channel] = (chip->block_3ch[channel] << 2) | fn_note[chip->fnum_3ch[channel] >> 7];
                break;
            case 0xac:
                chip->reg_ac = chip->data & 0xff;
                break;
            case 0xb0:
                chip->connect[channel] = chip->data & 0x07;
                chip->fb[channel] = (chip->data >> 3) & 0x07;
                break;
            case 0xb4:
                chip->pms[channel] = chip->data & 0x07;
                chip->ams[channel] = (chip->data >> 4) & 0x03;
                chip->pan_l[channel] = (chip->data >> 7) & 0x01;
                chip->pan_r[channel] = (chip->data >> 6) & 0x01;
                break;
            default:
                break;
            }
        }
    }

    if (chip->write_a_en || chip->write_d_en)
    {
        /* Data */
        if (chip->write_a_en)
        {
            chip->write_fm_data = 0;
        }

        if (chip->write_fm_address && chip->write_d_en)
        {
            chip->write_fm_data = 1;
        }

        /* Address */
        if (chip->write_a_en)
        {
            if ((chip->write_data & 0xf0) != 0x00)
            {
                /* FM Write */
                chip->address = chip->write_data;
                chip->write_fm_address = 1;
            }
            else
            {
                /* SSG write */
                chip->write_fm_address = 0;
            }
        }

        /* FM Mode */
        /* Data */
        if (chip->write_d_en && (chip->write_data & 0x100) == 0)
        {
            switch (chip->write_fm_mode_a)
            {
            case 0x21: /* LSI test 1 */
                for (i = 0; i < 8; i++)
                {
                    chip->mode_test_21[i] = (chip->write_data >> i) & 0x01;
                }
                break;
            case 0x22: /* LFO control */
                if ((chip->write_data >> 3) & 0x01)
                {
                    chip->lfo_en = 0x7f;
                }
                else
                {
                    chip->lfo_en = 0;
                }
                chip->lfo_freq = chip->write_data & 0x07;
                break;
            case 0x24: /* Timer A */
                chip->timer_a_reg &= 0x03;
                chip->timer_a_reg |= (chip->write_data & 0xff) << 2;
                break;
            case 0x25:
                chip->timer_a_reg &= 0x3fc;
                chip->timer_a_reg |= chip->write_data & 0x03;
                break;
            case 0x26: /* Timer B */
                chip->timer_b_reg = chip->write_data & 0xff;
                break;
            case 0x27: /* CSM, Timer control */
                chip->mode_ch3 = (chip->write_data & 0xc0) >> 6;
                chip->mode_csm = chip->mode_ch3 == 2;
                chip->timer_a_load = chip->write_data & 0x01;
                chip->timer_a_enable = (chip->write_data >> 2) & 0x01;
                chip->timer_a_reset = (chip->write_data >> 4) & 0x01;
                chip->timer_b_load = (chip->write_data >> 1) & 0x01;
                chip->timer_b_enable = (chip->write_data >> 3) & 0x01;
                chip->timer_b_reset = (chip->write_data >> 5) & 0x01;
                break;
            case 0x28: /* Key on/off */
                for (i = 0; i < 4; i++)
                {
                    chip->mode_kon_operator[i] = (chip->write_data >> (4 + i)) & 0x01;
                }
                if ((chip->write_data & 0x03) == 0x03)
                {
                    /* Invalid address */
                    chip->mode_kon_channel = 0xff;
                }
                else
                {
                    chip->mode_kon_channel = (chip->write_data & 0x03) + ((chip->write_data >> 2) & 1) * 3;
                }
                break;
            case 0x2a: /* DAC data */
                chip->dacdata &= 0x01;
                chip->dacdata |= (chip->write_data ^ 0x80) << 1;
                break;
            case 0x2b: /* DAC enable */
                chip->dacen = chip->write_data >> 7;
                break;
            case 0x2c: /* LSI test 2 */
                for (i = 0; i < 8; i++)
                {
                    chip->mode_test_2c[i] = (chip->write_data >> i) & 0x01;
                }
                chip->dacdata &= 0x1fe;
                chip->dacdata |= chip->mode_test_2c[3];
                chip->eg_custom_timer = !chip->mode_test_2c[7] && chip->mode_test_2c[6];
                break;
            default:
                break;
            }
        }

        /* Address */
        if (chip->write_a_en)
        {
            chip->write_fm_mode_a = chip->write_data & 0x1ff;
        }
    }

    if (chip->write_fm_data)
    {
        chip->data = chip->write_data & 0xff;
    }
}

static void OPN2_PhaseCalcIncrement(ym3438_t *chip)
{
    u32 chan = chip->channel;
    u32 slot = chip->cycles;
    u32 fnum = chip->pg_fnum;
    u32 fnum_h = fnum >> 4;
    u32 fm;
    u32 basefreq;
    u8 lfo = chip->lfo_pm;
    u8 lfo_l = lfo & 0x0f;
    u8 pms = chip->pms[chan];
    u8 dt = chip->dt[slot];
    u8 dt_l = dt & 0x03;
    u8 detune = 0;
    u8 block, note;
    u8 sum, sum_h, sum_l;
    u8 kcode = chip->pg_kcode;

    fnum <<= 1;
    /* Apply LFO */
    if (lfo_l & 0x08)
    {
        lfo_l ^= 0x0f;
    }
    fm = (fnum_h >> pg_lfo_sh1[pms][lfo_l]) + (fnum_h >> pg_lfo_sh2[pms][lfo_l]);
    if (pms > 5)
    {
        fm <<= pms - 5;
    }
    fm >>= 2;
    if (lfo & 0x10)
    {
        fnum -= fm;
    }
    else
    {
        fnum += fm;
    }
    fnum &= 0xfff;

    basefreq = (fnum << chip->pg_block) >> 2;

    /* Apply detune */
    if (dt_l)
    {
        if (kcode > 0x1c)
        {
            kcode = 0x1c;
        }
        block = kcode >> 2;
        note = kcode & 0x03;
        sum = block + 9 + ((dt_l == 3) | (dt_l & 0x02));
        sum_h = sum >> 1;
        sum_l = sum & 0x01;
        detune = pg_detune[(sum_l << 2) | note] >> (9 - sum_h);
    }
    if (dt & 0x04)
    {
        basefreq -= detune;
    }
    else
    {
        basefreq += detune;
    }
    basefreq &= 0x1ffff;
    chip->pg_inc[slot] = (basefreq * chip->multi[slot]) >> 1;
    chip->pg_inc[slot] &= 0xfffff;
}

static void OPN2_PhaseGenerate(ym3438_t *chip)
{
    u32 slot;
    /* Mask increment */
    slot = (chip->cycles + 20) % 24;
    if (chip->pg_reset[slot])
    {
        chip->pg_inc[slot] = 0;
    }
    /* Phase step */
    slot = (chip->cycles + 19) % 24;
    if (chip->pg_reset[slot] || chip->mode_test_21[3])
    {
        chip->pg_phase[slot] = 0;
    }
    chip->pg_phase[slot] += chip->pg_inc[slot];
    chip->pg_phase[slot] &= 0xfffff;
}

static void OPN2_EnvelopeSSGEG(ym3438_t *chip)
{
    u32 slot = chip->cycles;
    u8 direction = 0;
    chip->eg_ssg_pgrst_latch[slot] = 0;
    chip->eg_ssg_repeat_latch[slot] = 0;
    chip->eg_ssg_hold_up_latch[slot] = 0;
    if (chip->ssg_eg[slot] & 0x08)
    {
        direction = chip->eg_ssg_dir[slot];
        if (chip->eg_level[slot] & 0x200)
        {
            /* Reset */
            if ((chip->ssg_eg[slot] & 0x03) == 0x00)
            {
                chip->eg_ssg_pgrst_latch[slot] = 1;
            }
            /* Repeat */
            if ((chip->ssg_eg[slot] & 0x01) == 0x00)
            {
                chip->eg_ssg_repeat_latch[slot] = 1;
            }
            /* Inverse */
            if ((chip->ssg_eg[slot] & 0x03) == 0x02)
            {
                direction ^= 1;
            }
            if ((chip->ssg_eg[slot] & 0x03) == 0x03)
            {
                direction = 1;
            }
        }
        /* Hold up */
        if (chip->eg_kon_latch[slot]
         && ((chip->ssg_eg[slot] & 0x07) == 0x05 || (chip->ssg_eg[slot] & 0x07) == 0x03))
        {
            chip->eg_ssg_hold_up_latch[slot] = 1;
        }
        direction &= chip->eg_kon[slot];
    }
    chip->eg_ssg_dir[slot] = direction;
    chip->eg_ssg_enable[slot] = (chip->ssg_eg[slot] >> 3) & 0x01;
    chip->eg_ssg_inv[slot] = (chip->eg_ssg_dir[slot] ^ (((chip->ssg_eg[slot] >> 2) & 0x01) & ((chip->ssg_eg[slot] >> 3) & 0x01)))
                           & chip->eg_kon[slot];
}

static void OPN2_EnvelopeADSR(ym3438_t *chip)
{
    u32 slot = (chip->cycles + 22) % 24;

    u8 nkon = chip->eg_kon_latch[slot];
    u8 okon = chip->eg_kon[slot];
    u8 kon_event;
    u8 koff_event;
    u8 eg_off;
    s16 level;
    s16 nextlevel = 0;
    s16 ssg_level;
    u8 nextstate = chip->eg_state[slot];
    s16 inc = 0;
    chip->eg_read[0] = chip->eg_read_inc;
    chip->eg_read_inc = chip->eg_inc > 0;

    /* Reset phase generator */
    chip->pg_reset[slot] = (nkon && !okon) || chip->eg_ssg_pgrst_latch[slot];

    /* KeyOn/Off */
    kon_event = (nkon && !okon) || (okon && chip->eg_ssg_repeat_latch[slot]);
    koff_event = okon && !nkon;

    ssg_level = level = (s16)chip->eg_level[slot];

    if (chip->eg_ssg_inv[slot])
    {
        /* Inverse */
        ssg_level = 512 - level;
        ssg_level &= 0x3ff;
    }
    if (koff_event)
    {
        level = ssg_level;
    }
    if (chip->eg_ssg_enable[slot])
    {
        eg_off = level >> 9;
    }
    else
    {
        eg_off = (level & 0x3f0) == 0x3f0;
    }
    nextlevel = level;
    if (kon_event)
    {
        nextstate = eg_num_attack;
        /* Instant attack */
        if (chip->eg_ratemax)
        {
            nextlevel = 0;
        }
        else if (chip->eg_state[slot] == eg_num_attack && level != 0 && chip->eg_inc && nkon)
        {
            inc = (~level << chip->eg_inc) >> 5;
        }
    }
    else
    {
        switch (chip->eg_state[slot])
        {
        case eg_num_attack:
            if (level == 0)
            {
                nextstate = eg_num_decay;
            }
            else if(chip->eg_inc && !chip->eg_ratemax && nkon)
            {
                inc = (~level << chip->eg_inc) >> 5;
            }
            break;
        case eg_num_decay:
            if ((level >> 4) == (chip->eg_sl[1] << 1))
            {
                nextstate = eg_num_sustain;
            }
            else if (!eg_off && chip->eg_inc)
            {
                inc = 1 << (chip->eg_inc - 1);
                if (chip->eg_ssg_enable[slot])
                {
                    inc <<= 2;
                }
            }
            break;
        case eg_num_sustain:
        case eg_num_release:
            if (!eg_off && chip->eg_inc)
            {
                inc = 1 << (chip->eg_inc - 1);
                if (chip->eg_ssg_enable[slot])
                {
                    inc <<= 2;
                }
            }
            break;
        default:
            break;
        }
        if (!nkon)
        {
            nextstate = eg_num_release;
        }
    }
    if (chip->eg_kon_csm[slot])
    {
        nextlevel |= chip->eg_tl[1] << 3;
    }

    /* Envelope off */
    if (!kon_event && !chip->eg_ssg_hold_up_latch[slot] && chip->eg_state[slot] != eg_num_attack && eg_off)
    {
        nextstate = eg_num_release;
        nextlevel = 0x3ff;
    }

    nextlevel += inc;

    chip->eg_kon[slot] = chip->eg_kon_latch[slot];
    chip->eg_level[slot] = (u16)nextlevel & 0x3ff;
    chip->eg_state[slot] = nextstate;
}

static void OPN2_EnvelopePrepare(ym3438_t *chip)
{
    u8 rate;
    u8 sum;
    u8 inc = 0;
    u32 slot = chip->cycles;
    u8 rate_sel;

    /* Prepare increment */
    rate = (chip->eg_rate << 1) + chip->eg_ksv;

    if (rate > 0x3f)
    {
        rate = 0x3f;
    }

    sum = ((rate >> 2) + chip->eg_shift_lock) & 0x0f;
    if (chip->eg_rate != 0 && chip->eg_quotient == 2)
    {
        if (rate < 48)
        {
            switch (sum)
            {
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
        }
        else
        {
            inc = eg_stephi[rate & 0x03][chip->eg_timer_low_lock] + (rate >> 2) - 11;
            if (inc > 4)
            {
                inc = 4;
            }
        }
    }
    chip->eg_inc = inc;
    chip->eg_ratemax = (rate >> 1) == 0x1f;

    /* Prepare rate & ksv */
    rate_sel = chip->eg_state[slot];
    if ((chip->eg_kon[slot] && chip->eg_ssg_repeat_latch[slot])
     || (!chip->eg_kon[slot] && chip->eg_kon_latch[slot]))
    {
        rate_sel = eg_num_attack;
    }
    switch (rate_sel)
    {
    case eg_num_attack:
        chip->eg_rate = chip->ar[slot];
        break;
    case eg_num_decay:
        chip->eg_rate = chip->dr[slot];
        break;
    case eg_num_sustain:
        chip->eg_rate = chip->sr[slot];
        break;
    case eg_num_release:
        chip->eg_rate = (chip->rr[slot] << 1) | 0x01;
        break;
    default:
        break;
    }
    chip->eg_ksv = chip->pg_kcode >> (chip->ks[slot] ^ 0x03);
    if (chip->am[slot])
    {
        chip->eg_lfo_am = chip->lfo_am >> eg_am_shift[chip->ams[chip->channel]];
    }
    else
    {
        chip->eg_lfo_am = 0;
    }
    /* Delay TL & SL value */
    chip->eg_tl[1] = chip->eg_tl[0];
    chip->eg_tl[0] = chip->tl[slot];
    chip->eg_sl[1] = chip->eg_sl[0];
    chip->eg_sl[0] = chip->sl[slot];
}

static void OPN2_EnvelopeGenerate(ym3438_t *chip)
{
    u32 slot = (chip->cycles + 23) % 24;
    u16 level;

    level = chip->eg_level[slot];

    if (chip->eg_ssg_inv[slot])
    {
        /* Inverse */
        level = 512 - level;
    }
    if (chip->mode_test_21[5])
    {
        level = 0;
    }
    level &= 0x3ff;

    /* Apply AM LFO */
    level += chip->eg_lfo_am;

    /* Apply TL */
    if (!(chip->mode_csm && chip->channel == 2 + 1))
    {
        level += chip->eg_tl[0] << 3;
    }
    if (level > 0x3ff)
    {
        level = 0x3ff;
    }
    chip->eg_out[slot] = level;
}

static void OPN2_UpdateLFO(ym3438_t *chip)
{
    if ((chip->lfo_quotient & lfo_cycles[chip->lfo_freq]) == lfo_cycles[chip->lfo_freq])
    {
        chip->lfo_quotient = 0;
        chip->lfo_cnt++;
    }
    else
    {
        chip->lfo_quotient += chip->lfo_inc;
    }
    chip->lfo_cnt &= chip->lfo_en;
}

static void OPN2_FMPrepare(ym3438_t *chip)
{
    u32 slot = (chip->cycles + 6) % 24;
    u32 channel = chip->channel;
    s16 mod, mod1, mod2;
    u32 op = slot / 6;
    u8 connect = chip->connect[channel];
    u32 prevslot = (chip->cycles + 18) % 24;

    /* Calculate modulation */
    mod1 = mod2 = 0;

    if (fm_algorithm[op][0][connect])
    {
        mod2 |= chip->fm_op1[channel][0];
    }
    if (fm_algorithm[op][1][connect])
    {
        mod1 |= chip->fm_op1[channel][1];
    }
    if (fm_algorithm[op][2][connect])
    {
        mod1 |= chip->fm_op2[channel];
    }
    if (fm_algorithm[op][3][connect])
    {
        mod2 |= chip->fm_out[prevslot];
    }
    if (fm_algorithm[op][4][connect])
    {
        mod1 |= chip->fm_out[prevslot];
    }
    mod = mod1 + mod2;
    if (op == 0)
    {
        /* Feedback */
        mod = mod >> (10 - chip->fb[channel]);
        if (!chip->fb[channel])
        {
            mod = 0;
        }
    }
    else
    {
        mod >>= 1;
    }
    chip->fm_mod[slot] = mod;

    slot = (chip->cycles + 18) % 24;
    /* OP1 */
    if (slot / 6 == 0)
    {
        chip->fm_op1[channel][1] = chip->fm_op1[channel][0];
        chip->fm_op1[channel][0] = chip->fm_out[slot];
    }
    /* OP2 */
    if (slot / 6 == 2)
    {
        chip->fm_op2[channel] = chip->fm_out[slot];
    }
}

static void OPN2_ChGenerate(ym3438_t *chip)
{
    u32 slot = (chip->cycles + 18) % 24;
    u32 channel = chip->channel;
    u32 op = slot / 6;
    u32 test_dac = chip->mode_test_2c[5];
    s16 acc = chip->ch_acc[channel];
    s16 add = test_dac;
    s16 sum = 0;
    if (op == 0 && !test_dac)
    {
        acc = 0;
    }
    if (fm_algorithm[op][5][chip->connect[channel]] && !test_dac)
    {
        add += chip->fm_out[slot] >> 5;
    }
    sum = acc + add;
    /* Clamp */
    if (sum > 255)
    {
        sum = 255;
    }
    else if(sum < -256)
    {
        sum = -256;
    }

    if (op == 0 || test_dac)
    {
        chip->ch_out[channel] = chip->ch_acc[channel];
    }
    chip->ch_acc[channel] = sum;
}

static void OPN2_ChOutput(ym3438_t *chip)
{
    u32 cycles = chip->cycles;
    u32 slot = chip->cycles;
    u32 channel = chip->channel;
    u32 test_dac = chip->mode_test_2c[5];
    s16 out;
    s16 sign;
    u32 out_en;
    chip->ch_read = chip->ch_lock;
    if (slot < 12)
    {
        /* Ch 4,5,6 */
        channel++;
    }
    if ((cycles & 3) == 0)
    {
        if (!test_dac)
        {
            /* Lock value */
            chip->ch_lock = chip->ch_out[channel];
        }
        chip->ch_lock_l = chip->pan_l[channel];
        chip->ch_lock_r = chip->pan_r[channel];
    }
    /* Ch 6 */
    if (((cycles >> 2) == 1 && chip->dacen) || test_dac)
    {
        out = (s16)chip->dacdata;
        out = SIGN_EXTEND(8, out);
    }
    else
    {
        out = chip->ch_lock;
    }
    chip->mol = 0;
    chip->mor = 0;

    if (chip_type & ym3438_mode_ym2612)
    {
        out_en = ((cycles & 3) == 3) || test_dac;
        /* YM2612 DAC emulation(not verified) */
        sign = out >> 8;
        if (out >= 0)
        {
            out++;
            sign++;
        }
        if (chip->ch_lock_l && out_en)
        {
            chip->mol = out;
        }
        else
        {
            chip->mol = sign;
        }
        if (chip->ch_lock_r && out_en)
        {
            chip->mor = out;
        }
        else
        {
            chip->mor = sign;
        }
        /* Amplify signal */
        chip->mol *= 3;
        chip->mor *= 3;
    }
    else
    {
        out_en = ((cycles & 3) != 0) || test_dac;
        if (chip->ch_lock_l && out_en)
        {
            chip->mol = out;
        }
        if (chip->ch_lock_r && out_en)
        {
            chip->mor = out;
        }
    }
}

static void OPN2_FMGenerate(ym3438_t *chip)
{
    u32 slot = (chip->cycles + 19) % 24;
    /* Calculate phase */
    u16 phase = (chip->fm_mod[slot] + (chip->pg_phase[slot] >> 10)) & 0x3ff;
    u16 quarter;
    u16 level;
    s16 output;
    if (phase & 0x100)
    {
        quarter = (phase ^ 0xff) & 0xff;
    }
    else
    {
        quarter = phase & 0xff;
    }
    level = logsinrom[quarter];
    /* Apply envelope */
    level += chip->eg_out[slot] << 2;
    /* Transform */
    if (level > 0x1fff)
    {
        level = 0x1fff;
    }
    output = ((exprom[(level & 0xff) ^ 0xff] | 0x400) << 2) >> (level >> 8);
    if (phase & 0x200)
    {
        output = ((~output) ^ (chip->mode_test_21[4] << 13)) + 1;
    }
    else
    {
        output = output ^ (chip->mode_test_21[4] << 13);
    }
    output = SIGN_EXTEND(13, output);
    chip->fm_out[slot] = output;
}

static void OPN2_DoTimerA(ym3438_t *chip)
{
    u16 time;
    u8 load;
    load = chip->timer_a_overflow;
    if (chip->cycles == 2)
    {
        /* Lock load value */
        load |= (!chip->timer_a_load_lock && chip->timer_a_load);
        chip->timer_a_load_lock = chip->timer_a_load;
        if (chip->mode_csm)
        {
            /* CSM KeyOn */
            chip->mode_kon_csm = load;
        }
        else
        {
            chip->mode_kon_csm = 0;
        }
    }
    /* Load counter */
    if (chip->timer_a_load_latch)
    {
        time = chip->timer_a_reg;
    }
    else
    {
        time = chip->timer_a_cnt;
    }
    chip->timer_a_load_latch = load;
    /* Increase counter */
    if ((chip->cycles == 1 && chip->timer_a_load_lock) || chip->mode_test_21[2])
    {
        time++;
    }
    /* Set overflow flag */
    if (chip->timer_a_reset)
    {
        chip->timer_a_reset = 0;
        chip->timer_a_overflow_flag = 0;
    }
    else
    {
        chip->timer_a_overflow_flag |= chip->timer_a_overflow & chip->timer_a_enable;
    }
    chip->timer_a_overflow = (time >> 10);
    chip->timer_a_cnt = time & 0x3ff;
}

static void OPN2_DoTimerB(ym3438_t *chip)
{
    u16 time;
    u8 load;
    load = chip->timer_b_overflow;
    if (chip->cycles == 2)
    {
        /* Lock load value */
        load |= (!chip->timer_b_load_lock && chip->timer_b_load);
        chip->timer_b_load_lock = chip->timer_b_load;
    }
    /* Load counter */
    if (chip->timer_b_load_latch)
    {
        time = chip->timer_b_reg;
    }
    else
    {
        time = chip->timer_b_cnt;
    }
    chip->timer_b_load_latch = load;
    /* Increase counter */
    if (chip->cycles == 1)
    {
        chip->timer_b_subcnt++;
    }
    if ((chip->timer_b_subcnt == 0x10 && chip->timer_b_load_lock) || chip->mode_test_21[2])
    {
        time++;
    }
    chip->timer_b_subcnt &= 0x0f;
    /* Set overflow flag */
    if (chip->timer_b_reset)
    {
        chip->timer_b_reset = 0;
        chip->timer_b_overflow_flag = 0;
    }
    else
    {
        chip->timer_b_overflow_flag |= chip->timer_b_overflow & chip->timer_b_enable;
    }
    chip->timer_b_overflow = (time >> 8);
    chip->timer_b_cnt = time & 0xff;
}

static void OPN2_KeyOn(ym3438_t*chip)
{
    u32 slot = chip->cycles;
    u32 chan = chip->channel;
    /* Key On */
    chip->eg_kon_latch[slot] = chip->mode_kon[slot];
    chip->eg_kon_csm[slot] = 0;
    if (chip->channel == 2 && chip->mode_kon_csm)
    {
        /* CSM Key On */
        chip->eg_kon_latch[slot] = 1;
        chip->eg_kon_csm[slot] = 1;
    }
    if (chip->cycles == chip->mode_kon_channel)
    {
        /* OP1 */
        chip->mode_kon[chan] = chip->mode_kon_operator[0];
        /* OP2 */
        chip->mode_kon[chan + 12] = chip->mode_kon_operator[1];
        /* OP3 */
        chip->mode_kon[chan + 6] = chip->mode_kon_operator[2];
        /* OP4 */
        chip->mode_kon[chan + 18] = chip->mode_kon_operator[3];
    }
}

void OPN2_Reset(ym3438_t *chip)
{
    u32 i;
    xee::mem::Memset(chip, 0, sizeof(ym3438_t));
    for (i = 0; i < 24; i++)
    {
        chip->eg_out[i] = 0x3ff;
        chip->eg_level[i] = 0x3ff;
        chip->eg_state[i] = eg_num_release;
        chip->multi[i] = 1;
    }
    for (i = 0; i < 6; i++)
    {
        chip->pan_l[i] = 1;
        chip->pan_r[i] = 1;
    }
}

void OPN2_SetChipType(u32 type)
{
    chip_type = type;
}

void OPN2_Clock(ym3438_t *chip, s16 *buffer)
{
    u32 slot = chip->cycles;
    chip->lfo_inc = chip->mode_test_21[1];
    chip->pg_read >>= 1;
    chip->eg_read[1] >>= 1;
    chip->eg_cycle++;
    /* Lock envelope generator timer value */
    if (chip->cycles == 1 && chip->eg_quotient == 2)
    {
        if (chip->eg_cycle_stop)
        {
            chip->eg_shift_lock = 0;
        }
        else
        {
            chip->eg_shift_lock = chip->eg_shift + 1;
        }
        chip->eg_timer_low_lock = chip->eg_timer & 0x03;
    }
    /* Cycle specific functions */
    switch (chip->cycles)
    {
    case 0:
        chip->lfo_pm = chip->lfo_cnt >> 2;
        if (chip->lfo_cnt & 0x40)
        {
            chip->lfo_am = chip->lfo_cnt & 0x3f;
        }
        else
        {
            chip->lfo_am = chip->lfo_cnt ^ 0x3f;
        }
        chip->lfo_am <<= 1;
        break;
    case 1:
        chip->eg_quotient++;
        chip->eg_quotient %= 3;
        chip->eg_cycle = 0;
        chip->eg_cycle_stop = 1;
        chip->eg_shift = 0;
        chip->eg_timer_inc |= chip->eg_quotient >> 1;
        chip->eg_timer = chip->eg_timer + chip->eg_timer_inc;
        chip->eg_timer_inc = chip->eg_timer >> 12;
        chip->eg_timer &= 0xfff;
        break;
    case 2:
        chip->pg_read = chip->pg_phase[21] & 0x3ff;
        chip->eg_read[1] = chip->eg_out[0];
        break;
    case 13:
        chip->eg_cycle = 0;
        chip->eg_cycle_stop = 1;
        chip->eg_shift = 0;
        chip->eg_timer = chip->eg_timer + chip->eg_timer_inc;
        chip->eg_timer_inc = chip->eg_timer >> 12;
        chip->eg_timer &= 0xfff;
        break;
    case 23:
        chip->lfo_inc |= 1;
        break;
    }
    chip->eg_timer &= ~(chip->mode_test_21[5] << chip->eg_cycle);
    if (((chip->eg_timer >> chip->eg_cycle) | (chip->pin_test_in & chip->eg_custom_timer)) & chip->eg_cycle_stop)
    {
        chip->eg_shift = chip->eg_cycle;
        chip->eg_cycle_stop = 0;
    }

    OPN2_DoIO(chip);

    OPN2_DoTimerA(chip);
    OPN2_DoTimerB(chip);
    OPN2_KeyOn(chip);

    OPN2_ChOutput(chip);
    OPN2_ChGenerate(chip);

    OPN2_FMPrepare(chip);
    OPN2_FMGenerate(chip);

    OPN2_PhaseGenerate(chip);
    OPN2_PhaseCalcIncrement(chip);

    OPN2_EnvelopeADSR(chip);
    OPN2_EnvelopeGenerate(chip);
    OPN2_EnvelopeSSGEG(chip);
    OPN2_EnvelopePrepare(chip);

    /* Prepare fnum & block */
    if (chip->mode_ch3)
    {
        /* Channel 3 special mode */
        switch (slot)
        {
        case 1: /* OP1 */
            chip->pg_fnum = chip->fnum_3ch[1];
            chip->pg_block = chip->block_3ch[1];
            chip->pg_kcode = chip->kcode_3ch[1];
            break;
        case 7: /* OP3 */
            chip->pg_fnum = chip->fnum_3ch[0];
            chip->pg_block = chip->block_3ch[0];
            chip->pg_kcode = chip->kcode_3ch[0];
            break;
        case 13: /* OP2 */
            chip->pg_fnum = chip->fnum_3ch[2];
            chip->pg_block = chip->block_3ch[2];
            chip->pg_kcode = chip->kcode_3ch[2];
            break;
        case 19: /* OP4 */
        default:
            chip->pg_fnum = chip->fnum[(chip->channel + 1) % 6];
            chip->pg_block = chip->block[(chip->channel + 1) % 6];
            chip->pg_kcode = chip->kcode[(chip->channel + 1) % 6];
            break;
        }
    }
    else
    {
        chip->pg_fnum = chip->fnum[(chip->channel + 1) % 6];
        chip->pg_block = chip->block[(chip->channel + 1) % 6];
        chip->pg_kcode = chip->kcode[(chip->channel + 1) % 6];
    }

    OPN2_UpdateLFO(chip);
    OPN2_DoRegWrite(chip);
    chip->cycles = (chip->cycles + 1) % 24;
    chip->channel = chip->cycles % 6;

    buffer[0] = chip->mol;
    buffer[1] = chip->mor;

    if (chip->status_time)
        chip->status_time--;
}

void OPN2_Write(ym3438_t *chip, u32 port, u8 data)
{
    port &= 3;
    chip->write_data = ((port << 7) & 0x100) | data;
    if (port & 1)
    {
        /* Data */
        chip->write_d |= 1;
    }
    else
    {
        /* Address */
        chip->write_a |= 1;
    }
}

void OPN2_SetTestPin(ym3438_t *chip, u32 value)
{
    chip->pin_test_in = value & 1;
}

u32 OPN2_ReadTestPin(ym3438_t *chip)
{
    if (!chip->mode_test_2c[7])
    {
        return 0;
    }
    return chip->cycles == 23;
}

u32 OPN2_ReadIRQPin(ym3438_t *chip)
{
    return chip->timer_a_overflow_flag | chip->timer_b_overflow_flag;
}

u8 OPN2_Read(ym3438_t *chip, u32 port)
{
    if ((port & 3) == 0 || (chip_type & ym3438_mode_readmode))
    {
        if (chip->mode_test_21[6])
        {
            /* Read test data */
            u32 slot = (chip->cycles + 18) % 24;
            u16 testdata = ((chip->pg_read & 0x01) << 15)
                            | ((chip->eg_read[chip->mode_test_21[0]] & 0x01) << 14);
            if (chip->mode_test_2c[4])
            {
                testdata |= chip->ch_read & 0x1ff;
            }
            else
            {
                testdata |= chip->fm_out[slot] & 0x3fff;
            }
            if (chip->mode_test_21[7])
            {
                chip->status = testdata & 0xff;
            }
            else
            {
                chip->status = testdata >> 8;
            }
        }
        else
        {
            chip->status = (chip->busy << 7) | (chip->timer_b_overflow_flag << 1)
                 | chip->timer_a_overflow_flag;
        }
        if (chip_type & ym3438_mode_ym2612)
        {
            chip->status_time = 300000;
        }
        else
        {
            chip->status_time = 40000000;
        }
    }
    if (chip->status_time)
    {
        return chip->status;
    }
    return 0;
}
