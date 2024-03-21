/**
 * This file is part of the Video Game Systems (VIGAS) project.
 * 
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#include "gpgx/gpgx.h"

#include <new>

#include "gpgx/g_psg.h"
#include "gpgx/sound/sn76489.h"

namespace gpgx {

//==============================================================================

//------------------------------------------------------------------------------

// Initialize the parts of the Genesis Plus GX port.
bool InitGpgx()
{
  g_psg = new (std::nothrow) gpgx::sound::Sn76489();

  if (!g_psg) {
    return false;
  }

  return true;
}

} // namespace gpgx
