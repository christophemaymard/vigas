/***************************************************************************************
 *  Genesis Plus GX
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

#ifndef __GPGX_HID_CONTROLLER_H__
#define __GPGX_HID_CONTROLLER_H__

#include "xee/fnd/data_type.h"

#include "gpgx/hid/controller_type.h"
#include "gpgx/hid/input.h" // For Button.

namespace gpgx::hid {

//==============================================================================

//------------------------------------------------------------------------------

class Controller
{
public:
  Controller(ControllerType type);

  /// Returns the type of this controller.
  /// 
  /// @return The type of this controller.
  ControllerType GetType() const;

  /// Sets the state of the specified button as pressed.
  /// 
  /// @param  button  The button to change the state.
  void PressButton(Button button);

  /// Indicates whether the specified button is pressed.
  /// 
  /// @param  button  The button to check.
  /// @return true if the button is pressed, otherwise false.
  bool IsButtonPressed(Button button) const;

  /// Returns the button states.
  /// 
  /// @return The button states.
  u16 GetButtons() const;

  /// Reset the button states.
  void ResetButtons();

private:
  ControllerType m_type; /// The type of this controller.
  u16 m_buttons; // The button states.
};

} // namespace gpgx::hid

#endif // #ifndef __GPGX_HID_CONTROLLER_H__

