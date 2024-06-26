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

#include "gpgx/hid/hid_system.h"

#include "gpgx/hid/controller.h"
#include "gpgx/hid/controller_type.h"
#include "gpgx/hid/device.h"
#include "gpgx/hid/device_type.h"

namespace gpgx::hid {

//==============================================================================
// HIDSystem

//------------------------------------------------------------------------------

HIDSystem::HIDSystem()
{
  u32 idx = 0;

  for (idx = 0; idx < kDeviceCount; idx++) {
    m_devices[idx] = nullptr;
  }

  for (idx = 0; idx < kControllerCount; idx++) {
    m_controllers[idx] = nullptr;
  }
}

//------------------------------------------------------------------------------

void HIDSystem::Initialize()
{
  for (u32 idx = 0; idx < kDeviceCount; idx++) {
    if (m_devices[idx]) {
      delete m_devices[idx];
      m_devices[idx] = nullptr;
    }

    m_devices[idx] = new Device(DeviceType::kNone);
  }

  DisconnectAllControllers();
}

//------------------------------------------------------------------------------

void HIDSystem::ConnectDevice(u32 port, DeviceType type)
{
  if (port >= kDeviceCount) {
    return;
  }

  if (m_devices[port]) {
    delete m_devices[port];
    m_devices[port] = nullptr;
  }

  m_devices[port] = new Device(type);
}

//------------------------------------------------------------------------------

Device* HIDSystem::GetDevice(u32 port) const
{
  return (port >= kDeviceCount) ? nullptr : m_devices[port];
}

//------------------------------------------------------------------------------

void HIDSystem::ConnectController(u32 index, ControllerType type)
{
  if (index >= kControllerCount) {
    return;
  }

  if (m_controllers[index]) {
    delete m_controllers[index];
    m_controllers[index] = nullptr;
  }

  m_controllers[index] = new Controller(type);
}

//------------------------------------------------------------------------------

Controller* HIDSystem::GetController(u32 index) const
{
  return (index >= kControllerCount) ? nullptr : m_controllers[index];
}

//------------------------------------------------------------------------------

void HIDSystem::DisconnectAllControllers()
{
  for (u32 idx = 0; idx < kControllerCount; idx++) {
    if (m_controllers[idx]) {
      delete m_controllers[idx];
      m_controllers[idx] = nullptr;
    }

    m_controllers[idx] = new Controller(ControllerType::kNone);
  }
}

} // namespace gpgx::hid

