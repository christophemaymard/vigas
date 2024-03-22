/**
 * This file is part of the Video Game Systems (VIGAS) project.
 * 
 * Copyright 2024 Christophe Maymard <christophe.maymard@hotmail.com>
 */

#ifndef __GPGX_GPGX_H__
#define __GPGX_GPGX_H__

namespace gpgx {

//==============================================================================

//------------------------------------------------------------------------------

// Initialize the parts of the Genesis Plus GX port.
bool InitGpgx();

// Destroy all the parts of the Genesis Plus GX port.
void DestroyGpgx();

} // namespace gpgx

#endif // #ifndef __GPGX_GPGX_H__

