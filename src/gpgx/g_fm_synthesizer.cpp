/**
 * This file is part of the Video Game Systems (VIGAS) project.
 *
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#include "gpgx/g_fm_synthesizer.h"

namespace gpgx {

//==============================================================================

//------------------------------------------------------------------------------

// The current FM synthesizer (that can be YM3438, YM2612, YM2413 or "Null").
gpgx::audio::effect::IFmSynthesizer* g_fm_synthesizer = nullptr;

} // namespace gpgx

