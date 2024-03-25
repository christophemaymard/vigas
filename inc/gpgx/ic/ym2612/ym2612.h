/**
 * software implementation of Yamaha FM sound generator (YM2612/YM3438)
 *
 * Original code (MAME fm.c)
 *
 * Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
 * Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
 *
 * Version 1.4 (final beta)
 *
 * Additional code & fixes by Eke-Eke for Genesis Plus GX
 */

#ifndef __GPGX_IC_YM2612_YM2612_H__
#define __GPGX_IC_YM2612_YM2612_H__

#include "xee/fnd/data_type.h"

#include "gpgx/audio/effect/fm_synthesizer_base.h"

#include "gpgx/ic/ym2612/fm_ch.h"
#include "gpgx/ic/ym2612/fm_opn.h"
#include "gpgx/ic/ym2612/fm_slot.h"

namespace gpgx::ic::ym2612 {

//==============================================================================

//------------------------------------------------------------------------------

class Ym2612 : public gpgx::audio::effect::FmSynthesizerBase
{
public:
  // YM2612 internal clock = input clock / 6 = (master clock / 7) / 6.
  static constexpr s32 kYm2612ClockRatio = 7 * 6;

private:
  // Operator unit.

  static constexpr s32 kSinBits = 10;
  static constexpr s32 kSinLen = 1 << kSinBits;
  static constexpr s32 kSinMask = kSinLen - 1;
  
  static constexpr s32 kTlResLen = 256; // 8 bits addressing (real chip)

  //  TL_TAB_LEN is calculated as:
  //  13 - sinus amplitude bits     (Y axis)
  //  2  - sinus sign bit           (Y axis)
  //  TL_RES_LEN - sinus resolution (X axis)
  static constexpr s32 kTlTabLen = 13 * 2 * kTlResLen;

  // Sustain level table.
  static const u32 kSustainLevelTable[16];

  static constexpr s32 kRateSteps = 8;

  static const u8 kEgInc[19 * kRateSteps];

  // Envelope Generator rates (32 + 64 rates + 32 RKS).
  static const u8 kEgRateSelect[32 + 64 + 32];

  // Envelope Generator counter shifts (32 + 64 rates + 32 RKS).
  static const u8 kEgRateShift[32 + 64 + 32];

  // YM2151 and YM2612 phase increment data (in 10.10 fixed point format).
  static const u8 kDtTab[4 * 32];

  // OPN key frequency number -> key code follow table.
  static const u8 kOpnFreqKeyTable[16];

  // LFO speed parameters.
  static const u32 kLfoSamplesPerStep[8];

  static const u8 kLfoAmsDepthShift[4];

  static const u8 kLfoPmOutput[7 * 8][8];

public:
  Ym2612();

  void YM2612Init();
  void YM2612Config(int type);
  void YM2612ResetChip();

  void YM2612Update(int* buffer, int length);
  void YM2612Write(unsigned int a, unsigned int v);
  unsigned int YM2612Read();

  // Implementation of gpgx::audio::effect::IFmSynthesizer.

  // Synchronize FM chip with CPU and reset FM chip.
  void SyncAndReset(unsigned int cycles);

  void Write(unsigned int cycles, unsigned int address, unsigned int data);
  unsigned int Read(unsigned int cycles, unsigned int address);

private:
  void FM_KEYON(FM_CH* CH, int s);
  void FM_KEYOFF(FM_CH* CH, int s);
  void FM_KEYON_CSM(FM_CH* CH, int s);
  void FM_KEYOFF_CSM(FM_CH* CH, int s);
  void CSMKeyControll(FM_CH* CH);
  void INTERNAL_TIMER_A();
  void INTERNAL_TIMER_B(int step);
  void set_timers(int v);
  void setup_connection(FM_CH* CH, int ch);
  void set_det_mul(FM_CH* CH, FM_SLOT* SLOT, int v);
  void set_tl(FM_SLOT* SLOT, int v);
  void set_ar_ksr(FM_CH* CH, FM_SLOT* SLOT, int v);
  void set_dr(FM_SLOT* SLOT, int v);
  void set_sr(FM_SLOT* SLOT, int v);
  void set_sl_rr(FM_SLOT* SLOT, int v);
  void advance_lfo();
  void advance_eg_channels(FM_CH* CH, unsigned int eg_cnt);
  void update_ssg_eg_channels(FM_CH* CH);
  void update_phase_lfo_slot(FM_SLOT* SLOT, u32 pm, u8 kc, u32 fc);
  void update_phase_lfo_channel(FM_CH* CH);
  void refresh_fc_eg_slot(FM_SLOT* SLOT, unsigned int fc, unsigned int kc);
  void refresh_fc_eg_chan(FM_CH* CH);
  signed int op_calc(u32 phase, unsigned int env, unsigned int pm, unsigned int opmask);
  signed int op_calc1(u32 phase, unsigned int env, unsigned int pm, unsigned int opmask);
  void chan_calc(FM_CH* CH, int num);
  void OPNWriteMode(int r, int v);
  void OPNWriteReg(int r, int v);
  void reset_channels(FM_CH* CH, int num);
  void init_tables();

  // Implementation of gpgx::audio::effect::FmSynthesizerBase.

  void UpdateSampleBuffer(int* buffer, int length);

  int SaveChipContext(unsigned char* state);
  int LoadChipContext(unsigned char* state);

private:
  signed int m_tl_tab[kTlTabLen];

  // Sin waveform table in 'decibel' scale.
  unsigned int m_sin_tab[kSinLen];

  // All 128 LFO PM waveforms.
  // 128 combinations of 7 bits meaningful (of F-NUMBER), 8 LFO depths, 
  // 32 LFO output levels per one depth.
  s32 m_lfo_pm_table[128 * 8 * 32];

  // Current chip state.

  s32  m_m2;        // Phase Modulation input for operators 2.
  s32  m_c1;        // Phase Modulation input for operators 3.
  s32  m_c2;        // Phase Modulation input for operators 4.
  s32  m_mem;       // One sample delay memory.
  s32  m_out_fm[6]; // Outputs of working channels.

  // Chip type.

  u32 m_op_mask[8][4];  // Operator output bitmasking (DAC quantization).
  int m_chip_type;

  // Context.

  FM_CH m_CH[6];  // Channel state.
  u8 m_dacen;     // DAC mode.
  s32 m_dacout;   // DAC output.
  FM_OPN m_OPN;   // OPN state.
};

} // namespace gpgx::ic::ym2612

#endif // #ifndef __GPGX_IC_YM2612_YM2612_H__

