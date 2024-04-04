/**
 * This file is part of the Video Game Systems (VIGAS) project.
 *
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#ifndef __GPGX_VGS_VDP_IRQ_HANDLER_Z80_H__
#define __GPGX_VGS_VDP_IRQ_HANDLER_Z80_H__

namespace gpgx::vgs {

//==============================================================================

//------------------------------------------------------------------------------

void z80_vdp_set_irq_line(unsigned int state);

} // namespace gpgx::vgs

#endif // #ifndef __GPGX_VGS_VDP_IRQ_HANDLER_Z80_H__

