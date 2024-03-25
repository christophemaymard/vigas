/**
 * This file is part of the Video Game Systems (VIGAS) project.
 * 
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#include "gpgx/gpgx.h"

#include <new> // For std::nothrow.

#include "gpgx/g_psg.h"
#include "gpgx/ic/sn76489/sn76489.h"

namespace gpgx {

//==============================================================================

//------------------------------------------------------------------------------

// Initialize the parts of the Genesis Plus GX port.
bool InitGpgx()
{
  g_psg = new (std::nothrow) gpgx::ic::sn76489::Sn76489();

  if (!g_psg) {
    DestroyGpgx();

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------

// Destroy all the parts of the Genesis Plus GX port.
void DestroyGpgx()
{
  if (g_psg) {
    delete g_psg;
    g_psg = nullptr;
  }
}

} // namespace gpgx
