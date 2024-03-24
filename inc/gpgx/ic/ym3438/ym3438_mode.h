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

#ifndef __GPGX_IC_YM3438_YM3438_MODE_H__
#define __GPGX_IC_YM3438_YM3438_MODE_H__

namespace gpgx::ic::ym3438 {

//==============================================================================

//------------------------------------------------------------------------------

enum Ym3438Mode
{
  ym3438_mode_ym2612 = 0x01,    // Enables YM2612 emulation (MD1, MD2 VA2).
  ym3438_mode_readmode = 0x02,  // Enables status read on any port (TeraDrive, MD1 VA7, MD2, etc).
};

} // namespace gpgx::ic::ym3438

#endif // #ifndef __GPGX_IC_YM3438_YM3438_MODE_H__

