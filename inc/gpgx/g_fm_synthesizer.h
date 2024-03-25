/**
 * This file is part of the Video Game Systems (VIGAS) project.
 *
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#ifndef __GPGX_G_FM_SYNTHESIZER_H__
#define __GPGX_G_FM_SYNTHESIZER_H__

#include "gpgx/audio/effect/fm_synthesizer.h"

namespace gpgx {

//==============================================================================

//------------------------------------------------------------------------------

// The current FM synthesizer (that can be YM3438, YM2612, YM2413 or "Null").
extern gpgx::audio::effect::IFmSynthesizer* g_fm_synthesizer;

} // namespace gpgx

#endif // #ifndef __GPGX_G_FM_SYNTHESIZER_H__

