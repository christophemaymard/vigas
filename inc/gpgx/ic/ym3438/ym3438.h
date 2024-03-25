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

#ifndef __GPGX_IC_YM3438_YM3438_H__
#define __GPGX_IC_YM3438_YM3438_H__

#include "xee/fnd/data_type.h"

#include "gpgx/audio/effect/fm_synthesizer_base.h"

#include "gpgx/ic/ym3438/opn2_context.h"

namespace gpgx::ic::ym3438 {

//==============================================================================

//------------------------------------------------------------------------------

class Ym3438 : public gpgx::audio::effect::FmSynthesizerBase
{
private:
  static const u16 kLogSinTable[256];
  static const u16 kExpTable[256];
  static const u32 kFnNote[16];
  static const u32 kEgStephi[4][4];
  static const u8 kEgAmShift[4];
  static const u32 kPgDetune[8];
  static const u32 kPgLfoSh1[8][8];
  static const u32 kPgLfoSh2[8][8];
  static const u32 kOpOffset[12];
  static const u32 kChOffset[6];
  static const u32 kLfoCycles[8];
  static const u32 kFmAlgorithm[4][6][8];

  static constexpr s32 kUpdateClock = 24;
  static constexpr s32 kUpdateSampleAmp = 11;
public:
  Ym3438();

  void OPN2_Reset();
  void OPN2_SetChipType(u32 type);
  void OPN2_Clock(s16* buffer);
  void OPN2_Write(u32 port, u8 data);
  void OPN2_SetTestPin(u32 value);
  u32 OPN2_ReadTestPin();
  u32 OPN2_ReadIRQPin();
  u8 OPN2_Read(u32 port);

  void Init();

  // Implementation of gpgx::audio::effect::IFmSynthesizer.

  // Synchronize FM chip with CPU and reset FM chip.
  void SyncAndReset(unsigned int cycles);

  void Write(unsigned int cycles, unsigned int address, unsigned int data);
  unsigned int Read(unsigned int cycles, unsigned int address);

private:
  void OPN2_DoIO();
  void OPN2_DoRegWrite();
  void OPN2_PhaseCalcIncrement();
  void OPN2_PhaseGenerate();
  void OPN2_EnvelopeSSGEG();
  void OPN2_EnvelopeADSR();
  void OPN2_EnvelopePrepare();
  void OPN2_EnvelopeGenerate();
  void OPN2_UpdateLFO();
  void OPN2_FMPrepare();
  void OPN2_ChGenerate();
  void OPN2_ChOutput();
  void OPN2_FMGenerate();
  void OPN2_DoTimerA();
  void OPN2_DoTimerB();
  void OPN2_KeyOn();

  // Implementation of gpgx::audio::effect::FmSynthesizerBase.

  void UpdateSampleBuffer(int* buffer, int length);

  int SaveChipContext(unsigned char* state);
  int LoadChipContext(unsigned char* state);

private:
  u32 m_chip_type;

  OPN2Context m_opn2_ctx; // OPN2 context.

  short m_accm[kUpdateClock][2];
  int m_sample[2];
  int m_cycles;
};

} // namespace gpgx::ic::ym3438

#endif // #ifndef __GPGX_IC_YM3438_YM3438_H__

