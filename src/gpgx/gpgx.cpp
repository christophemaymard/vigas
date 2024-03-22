/**
 * This file is part of the Video Game Systems (VIGAS) project.
 * 
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#include "gpgx/gpgx.h"

#include <new>

#include "gpgx/g_psg.h"
#include "gpgx/g_ym2413.h"
#include "gpgx/g_ym2612.h"
#include "gpgx/sound/sn76489.h"
#include "gpgx/sound/ym2413/ym2413.h"
#include "gpgx/sound/ym2612/ym2612.h"

namespace gpgx {

//==============================================================================

//------------------------------------------------------------------------------

// Initialize the parts of the Genesis Plus GX port.
bool InitGpgx()
{
  g_psg = new (std::nothrow) gpgx::sound::Sn76489();
  g_ym2612 = new (std::nothrow) gpgx::sound::ym2612::Ym2612();
  g_ym2413 = new (std::nothrow) gpgx::sound::ym2413::Ym2413();

  if (!g_psg || !g_ym2612 || !g_ym2413) {
    DestroyGpgx();

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------

// Destroy all the parts of the Genesis Plus GX port.
void DestroyGpgx()
{
  if (g_ym2413) {
    delete g_ym2413;
    g_ym2413 = nullptr;
  }

  if (g_ym2612) {
    delete g_ym2612;
    g_ym2612 = nullptr;
  }

  if (g_psg) {
    delete g_psg;
    g_psg = nullptr;
  }
}

} // namespace gpgx
