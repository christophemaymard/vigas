/***************************************************************************************
 *  Genesis Plus GX
 *  HID system.
 * 
 *  Copyright (C) 1998-2003  Charles Mac Donald (original code)
 *  Copyright (C) 2007-2016  Eke-Eke (Genesis Plus GX)
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************/

#ifndef __GPGX_HID_HID_SYSTEM_H__
#define __GPGX_HID_HID_SYSTEM_H__

#include "gpgx/hid/device.h"
#include "gpgx/hid/device_type.h"

namespace gpgx::hid {

//==============================================================================

//------------------------------------------------------------------------------

/// HID system.
class HIDSystem
{
public:
  static constexpr u32 kDeviceCount = 2; /// Max number of devices.

  HIDSystem();

  void Initialize();

  /// Connects a device to a port.
  /// 
  /// @param  port  The port to connect the device to (0 or 1).
  /// @param  type  The type of the device to connect.
  void ConnectDevice(u32 port, DeviceType type);

  /// Retrieves the device connected to the specified port.
  /// 
  /// @param  port  The port of the device to retrieve (0 or 1).
  /// @return The instance of the connected device.
  Device* GetDevice(u32 port) const;

private:
  Device* m_devices[kDeviceCount]; /// The connected devices.
};

} // namespace gpgx::hid

#endif // #ifndef __GPGX_HID_HID_SYSTEM_H__

