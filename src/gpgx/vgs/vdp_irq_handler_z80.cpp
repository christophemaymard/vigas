/**
 * This file is part of the Video Game Systems (VIGAS) project.
 * 
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#include "gpgx/g_z80.h"

namespace gpgx::vgs {

//==============================================================================

//------------------------------------------------------------------------------

void z80_vdp_set_irq_line(unsigned int state)
{
  gpgx::g_z80->SetIRQLine(state);
}

} // namespace gpgx::vgs

