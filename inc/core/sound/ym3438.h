/*
 * Copyright (C) 2017-2021 Alexey Khokholov (Nuke.YKT)
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
 * version: 1.0.9
 */

#ifndef YM3438_H
#define YM3438_H

#include "xee/fnd/data_type.h"

enum {
    ym3438_mode_ym2612 = 0x01,      /* Enables YM2612 emulation (MD1, MD2 VA2) */
    ym3438_mode_readmode = 0x02     /* Enables status read on any port (TeraDrive, MD1 VA7, MD2, etc) */
};

typedef struct
{
    u32 cycles;
    u32 channel;
    s16 mol, mor;
    /* IO */
    u16 write_data;
    u8 write_a;
    u8 write_d;
    u8 write_a_en;
    u8 write_d_en;
    u8 write_busy;
    u8 write_busy_cnt;
    u8 write_fm_address;
    u8 write_fm_data;
    u16 write_fm_mode_a;
    u16 address;
    u8 data;
    u8 pin_test_in;
    u8 pin_irq;
    u8 busy;
    /* LFO */
    u8 lfo_en;
    u8 lfo_freq;
    u8 lfo_pm;
    u8 lfo_am;
    u8 lfo_cnt;
    u8 lfo_inc;
    u8 lfo_quotient;
    /* Phase generator */
    u16 pg_fnum;
    u8 pg_block;
    u8 pg_kcode;
    u32 pg_inc[24];
    u32 pg_phase[24];
    u8 pg_reset[24];
    u32 pg_read;
    /* Envelope generator */
    u8 eg_cycle;
    u8 eg_cycle_stop;
    u8 eg_shift;
    u8 eg_shift_lock;
    u8 eg_timer_low_lock;
    u16 eg_timer;
    u8 eg_timer_inc;
    u16 eg_quotient;
    u8 eg_custom_timer;
    u8 eg_rate;
    u8 eg_ksv;
    u8 eg_inc;
    u8 eg_ratemax;
    u8 eg_sl[2];
    u8 eg_lfo_am;
    u8 eg_tl[2];
    u8 eg_state[24];
    u16 eg_level[24];
    u16 eg_out[24];
    u8 eg_kon[24];
    u8 eg_kon_csm[24];
    u8 eg_kon_latch[24];
    u8 eg_csm_mode[24];
    u8 eg_ssg_enable[24];
    u8 eg_ssg_pgrst_latch[24];
    u8 eg_ssg_repeat_latch[24];
    u8 eg_ssg_hold_up_latch[24];
    u8 eg_ssg_dir[24];
    u8 eg_ssg_inv[24];
    u32 eg_read[2];
    u8 eg_read_inc;
    /* FM */
    s16 fm_op1[6][2];
    s16 fm_op2[6];
    s16 fm_out[24];
    u16 fm_mod[24];
    /* Channel */
    s16 ch_acc[6];
    s16 ch_out[6];
    s16 ch_lock;
    u8 ch_lock_l;
    u8 ch_lock_r;
    s16 ch_read;
    /* Timer */
    u16 timer_a_cnt;
    u16 timer_a_reg;
    u8 timer_a_load_lock;
    u8 timer_a_load;
    u8 timer_a_enable;
    u8 timer_a_reset;
    u8 timer_a_load_latch;
    u8 timer_a_overflow_flag;
    u8 timer_a_overflow;

    u16 timer_b_cnt;
    u8 timer_b_subcnt;
    u16 timer_b_reg;
    u8 timer_b_load_lock;
    u8 timer_b_load;
    u8 timer_b_enable;
    u8 timer_b_reset;
    u8 timer_b_load_latch;
    u8 timer_b_overflow_flag;
    u8 timer_b_overflow;

    /* Register set */
    u8 mode_test_21[8];
    u8 mode_test_2c[8];
    u8 mode_ch3;
    u8 mode_kon_channel;
    u8 mode_kon_operator[4];
    u8 mode_kon[24];
    u8 mode_csm;
    u8 mode_kon_csm;
    u8 dacen;
    s16 dacdata;

    u8 ks[24];
    u8 ar[24];
    u8 sr[24];
    u8 dt[24];
    u8 multi[24];
    u8 sl[24];
    u8 rr[24];
    u8 dr[24];
    u8 am[24];
    u8 tl[24];
    u8 ssg_eg[24];

    u16 fnum[6];
    u8 block[6];
    u8 kcode[6];
    u16 fnum_3ch[6];
    u8 block_3ch[6];
    u8 kcode_3ch[6];
    u8 reg_a4;
    u8 reg_ac;
    u8 connect[6];
    u8 fb[6];
    u8 pan_l[6], pan_r[6];
    u8 ams[6];
    u8 pms[6];
    u8 status;
    u32 status_time;
} ym3438_t;

void OPN2_Reset(ym3438_t *chip);
void OPN2_SetChipType(u32 type);
void OPN2_Clock(ym3438_t *chip, s16 *buffer);
void OPN2_Write(ym3438_t *chip, u32 port, u8 data);
void OPN2_SetTestPin(ym3438_t *chip, u32 value);
u32 OPN2_ReadTestPin(ym3438_t *chip);
u32 OPN2_ReadIRQPin(ym3438_t *chip);
u8 OPN2_Read(ym3438_t *chip, u32 port);

#endif
