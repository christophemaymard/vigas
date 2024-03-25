/**
 * File: ym2413.c - software implementation of YM2413
 *                  FM sound generator type OPLL
 *
 * Copyright (C) 2002 Jarek Burczynski
 *
 * Version 1.0
 */

#ifndef __GPGX_IC_YM2413_YM2413_H__
#define __GPGX_IC_YM2413_YM2413_H__

#include "xee/fnd/data_type.h"

#include "gpgx/audio/effect/fm_synthesizer_base.h"

#include "gpgx/ic/ym2413/opll_slot.h"
#include "gpgx/ic/ym2413/opll_ch.h"

namespace gpgx::ic::ym2413 {

//==============================================================================

//------------------------------------------------------------------------------

class Ym2413 : public gpgx::audio::effect::FmSynthesizerBase
{
private:
  static constexpr s32 kSinBits = 10;
  static constexpr s32 kSinLen = 1 << kSinBits;
  static constexpr s32 kTlResLen = 256; // 8 bits addressing (real chip).

  // kTlTabLen is calculated as:
  // 11 - sinus amplitude bits     (Y axis)
  // 2  - sinus sign bit           (Y axis)
  // kTlResLen - sinus resolution (X axis)
  static constexpr s32 kTlTabLen = 11 * 2 * kTlResLen;
  static constexpr s32 kRateSteps = 16;
  static constexpr s32 kLfoAmTabElements = 210;

  static const u32 kKeyScaleLevelTable[8 * 16];
  static const u32 kSustainLevelTable[16];
  static const unsigned char kEgInc[14 * kRateSteps];
  static const unsigned char kEgMul[17 * kRateSteps];
  static const unsigned char kEgRateSelect[16 + 64 + 16];
  static const unsigned char kEgRateShift[16 + 64 + 16];
  static const u8 kMulTab[16];
  static const u8 kLfoAmTable[kLfoAmTabElements];
  static const s8 kLfoPmTable[8 * 8];
  static unsigned char kTable[19][8];

public:
  Ym2413();

  void YM2413Init();
  void YM2413ResetChip();

  void YM2413Update(int* buffer, int length);

  // YM2413 I/O interface.
  void YM2413Write(unsigned int a, unsigned int v);

  unsigned int YM2413Read();

  // Implementation of gpgx::audio::effect::IFmSynthesizer.

  // Synchronize FM chip with CPU and reset FM chip.
  void SyncAndReset(unsigned int cycles);

  void Write(unsigned int cycles, unsigned int address, unsigned int data);
  unsigned int Read(unsigned int cycles, unsigned int address);

private:
  void advance_lfo();
  void advance();
  signed int op_calc(u32 phase, unsigned int env, signed int pm, unsigned int wave_tab);
  signed int op_calc1(u32 phase, unsigned int env, signed int pm, unsigned int wave_tab);
  void chan_calc(OPLL_CH* CH);
  void rhythm_calc(OPLL_CH* CH, unsigned int noise);
  int init_tables();
  void OPLL_initalize();
  void KEY_ON(OPLL_SLOT* SLOT, u32 key_set);
  void KEY_OFF(OPLL_SLOT* SLOT, u32 key_clr);
  void CALC_FCSLOT(OPLL_CH* CH, OPLL_SLOT* SLOT);
  void set_mul(int slot, int v);
  void set_ksl_tl(int chan, int v);
  void set_ksl_wave_fb(int chan, int v);
  void set_ar_dr(int slot, int v);
  void set_sl_rr(int slot, int v);
  void load_instrument(u32 chan, u32 slot, u8* inst);
  void update_instrument_zero(u8 r);
  void OPLLWriteReg(int r, int v);
  void ClearContext();

  // Implementation of gpgx::audio::effect::FmSynthesizerBase.

  void UpdateSampleBuffer(int* buffer, int length);

  int SaveChipContext(unsigned char* state);
  int LoadChipContext(unsigned char* state);

private:
  // 
  signed int m_tl_tab[kTlTabLen];

  // sin waveform table in 'decibel' scale
  // two waveforms on OPLL type chips
  unsigned int m_sin_tab[kSinLen * 2];

  signed int m_output[2];
  u32 m_LFO_AM;
  s32 m_LFO_PM;

  // Chip state.

  OPLL_CH m_P_CH[9];        // OPLL chips have 9 channels.
  u8 m_instvol_r[9];        // instrument/volume (or volume/volume in percussive mode).

  u32 m_eg_cnt;             // global envelope generator counter.
  u32 m_eg_timer;           // global envelope generator counter works at frequency = chipclock/72
  u32 m_eg_timer_add;       // step of eg_timer.
  u32 m_eg_timer_overflow;  // envelope generator timer overlfows every 1 sample (on real chip).

  u8 m_rhythm;              // Rhythm mode.

  // LFO

  u32 m_lfo_am_cnt;
  u32 m_lfo_am_inc;
  u32 m_lfo_pm_cnt;
  u32 m_lfo_pm_inc;

  u32 m_noise_rng;      // 23 bit noise shift register.
  u32 m_noise_p;        // current noise 'phase'.
  u32 m_noise_f;        // current noise period.

  // instrument settings.
  // 
  // 0-user instrument
  // 1-15 - fixed instruments
  // 16 -bass drum settings
  // 17,18 - other percussion instruments
  u8 m_inst_tab[19][8];

  u32 m_fn_tab[1024]; // fnumber->increment counter.

  u8 m_address; // address register.
  u8 m_status;  // status flag.

  f64 m_clock;  // master clock  (Hz).
  int m_rate;   // sampling rate (Hz).
};

} // namespace gpgx::ic::ym2413

#endif // #ifndef __GPGX_IC_YM2413_YM2413_H__

