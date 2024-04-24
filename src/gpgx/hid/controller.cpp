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

#include "gpgx/hid/controller.h"

#include "gpgx/hid/input.h"

namespace gpgx::hid {

//==============================================================================
// Controller

//------------------------------------------------------------------------------

Controller::Controller(ControllerType type) :
  m_type(type),
  m_buttons(0)
{
}

//------------------------------------------------------------------------------

ControllerType Controller::GetType() const
{
  return m_type;
}

//------------------------------------------------------------------------------

void Controller::PressButton(Button button)
{
  switch (button) {
    case Button::kMode:
      m_buttons |= ButtonSet::kMode;
      break;
    case Button::kX:
      m_buttons |= ButtonSet::kX;
      break;
    case Button::kY:
      m_buttons |= ButtonSet::kY;
      break;
    case Button::kZ:
      m_buttons |= ButtonSet::kZ;
      break;
    case Button::kStart:
      m_buttons |= ButtonSet::kStart;
      break;
    case Button::kA:
      m_buttons |= ButtonSet::kA;
      break;
    case Button::kC:
      m_buttons |= ButtonSet::kC;
      break;
    case Button::kB:
      m_buttons |= ButtonSet::kB;
      break;
    case Button::kRight:
      m_buttons |= ButtonSet::kRight;
      break;
    case Button::kLeft:
      m_buttons |= ButtonSet::kLeft;
      break;
    case Button::kDown:
      m_buttons |= ButtonSet::kDown;
      break;
    case Button::kUp:
      m_buttons |= ButtonSet::kUp;
      break;
    case Button::kButton2:
      m_buttons |= ButtonSet::kButton2;
      break;
    case Button::kButton1:
      m_buttons |= ButtonSet::kButton1;
      break;
    case Button::kMouseCenter:
      m_buttons |= ButtonSet::kMouseCenter;
      break;
    case Button::kMouseRight:
      m_buttons |= ButtonSet::kMouseRight;
      break;
    case Button::kMouseLeft:
      m_buttons |= ButtonSet::kMouseLeft;
      break;
    case Button::kPicoPen:
      m_buttons |= ButtonSet::kPicoPen;
      break;
    case Button::kPicoRed:
      m_buttons |= ButtonSet::kPicoRed;
      break;
    case Button::kXeE1:
      m_buttons |= ButtonSet::kXeE1;
      break;
    case Button::kXeE2:
      m_buttons |= ButtonSet::kXeE2;
      break;
    case Button::kXeStart:
      m_buttons |= ButtonSet::kXeStart;
      break;
    case Button::kXeSelect:
      m_buttons |= ButtonSet::kXeSelect;
      break;
    case Button::kXeA:
      m_buttons |= ButtonSet::kXeA;
      break;
    case Button::kXeB:
      m_buttons |= ButtonSet::kXeB;
      break;
    case Button::kXeA2:
      m_buttons |= ButtonSet::kXeA2;
      break;
    case Button::kXeB2:
      m_buttons |= ButtonSet::kXeB2;
      break;
    case Button::kXeC:
      m_buttons |= ButtonSet::kXeC;
      break;
    case Button::kXeD:
      m_buttons |= ButtonSet::kXeD;
      break;
    case Button::kActivator8U:
      m_buttons |= ButtonSet::kActivator8U;
      break;
    case Button::kActivator8L:
      m_buttons |= ButtonSet::kActivator8L;
      break;
    case Button::kActivator7U:
      m_buttons |= ButtonSet::kActivator7U;
      break;
    case Button::kActivator7L:
      m_buttons |= ButtonSet::kActivator7L;
      break;
    case Button::kActivator6U:
      m_buttons |= ButtonSet::kActivator6U;
      break;
    case Button::kActivator6L:
      m_buttons |= ButtonSet::kActivator6L;
      break;
    case Button::kActivator5U:
      m_buttons |= ButtonSet::kActivator5U;
      break;
    case Button::kActivator5L:
      m_buttons |= ButtonSet::kActivator5L;
      break;
    case Button::kActivator4U:
      m_buttons |= ButtonSet::kActivator4U;
      break;
    case Button::kActivator4L:
      m_buttons |= ButtonSet::kActivator4L;
      break;
    case Button::kActivator3U:
      m_buttons |= ButtonSet::kActivator3U;
      break;
    case Button::kActivator3L:
      m_buttons |= ButtonSet::kActivator3L;
      break;
    case Button::kActivator2U:
      m_buttons |= ButtonSet::kActivator2U;
      break;
    case Button::kActivator2L:
      m_buttons |= ButtonSet::kActivator2L;
      break;
    case Button::kActivator1U:
      m_buttons |= ButtonSet::kActivator1U;
      break;
    case Button::kActivator1L:
      m_buttons |= ButtonSet::kActivator1L;
      break;
    case Button::kGraphicPen:
      m_buttons |= ButtonSet::kGraphicPen;
      break;
    case Button::kGraphicDo:
      m_buttons |= ButtonSet::kGraphicDo;
      break;
    case Button::kGraphicMenu:
      m_buttons |= ButtonSet::kGraphicMenu;
      break;
    default:
      // Nothing to do.
      break;
  }
}

//------------------------------------------------------------------------------

bool Controller::IsButtonPressed(Button button) const
{
  switch (button) {
    case Button::kMode:
      return m_buttons & ButtonSet::kMode;
    case Button::kX:
      return m_buttons & ButtonSet::kX;
    case Button::kY:
      return m_buttons & ButtonSet::kY;
    case Button::kZ:
      return m_buttons & ButtonSet::kZ;
    case Button::kStart:
      return m_buttons & ButtonSet::kStart;
    case Button::kA:
      return m_buttons & ButtonSet::kA;
    case Button::kC:
      return m_buttons & ButtonSet::kC;
    case Button::kB:
      return m_buttons & ButtonSet::kB;
    case Button::kRight:
      return m_buttons & ButtonSet::kRight;
    case Button::kLeft:
      return m_buttons & ButtonSet::kLeft;
    case Button::kDown:
      return m_buttons & ButtonSet::kDown;
    case Button::kUp:
      return m_buttons & ButtonSet::kUp;
    case Button::kButton2:
      return m_buttons & ButtonSet::kButton2;
    case Button::kButton1:
      return m_buttons & ButtonSet::kButton1;
    case Button::kMouseCenter:
      return m_buttons & ButtonSet::kMouseCenter;
    case Button::kMouseRight:
      return m_buttons & ButtonSet::kMouseRight;
    case Button::kMouseLeft:
      return m_buttons & ButtonSet::kMouseLeft;
    case Button::kPicoPen:
      return m_buttons & ButtonSet::kPicoPen;
    case Button::kPicoRed:
      return m_buttons & ButtonSet::kPicoRed;
    case Button::kXeE1:
      return m_buttons & ButtonSet::kXeE1;
    case Button::kXeE2:
      return m_buttons & ButtonSet::kXeE2;
    case Button::kXeStart:
      return m_buttons & ButtonSet::kXeStart;
    case Button::kXeSelect:
      return m_buttons & ButtonSet::kXeSelect;
    case Button::kXeA:
      return m_buttons & ButtonSet::kXeA;
    case Button::kXeB:
      return m_buttons & ButtonSet::kXeB;
    case Button::kXeA2:
      return m_buttons & ButtonSet::kXeA2;
    case Button::kXeB2:
      return m_buttons & ButtonSet::kXeB2;
    case Button::kXeC:
      return m_buttons & ButtonSet::kXeC;
    case Button::kXeD:
      return m_buttons & ButtonSet::kXeD;
    case Button::kActivator8U:
      return m_buttons & ButtonSet::kActivator8U;
    case Button::kActivator8L:
      return m_buttons & ButtonSet::kActivator8L;
    case Button::kActivator7U:
      return m_buttons & ButtonSet::kActivator7U;
    case Button::kActivator7L:
      return m_buttons & ButtonSet::kActivator7L;
    case Button::kActivator6U:
      return m_buttons & ButtonSet::kActivator6U;
    case Button::kActivator6L:
      return m_buttons & ButtonSet::kActivator6L;
    case Button::kActivator5U:
      return m_buttons & ButtonSet::kActivator5U;
    case Button::kActivator5L:
      return m_buttons & ButtonSet::kActivator5L;
    case Button::kActivator4U:
      return m_buttons & ButtonSet::kActivator4U;
    case Button::kActivator4L:
      return m_buttons & ButtonSet::kActivator4L;
    case Button::kActivator3U:
      return m_buttons & ButtonSet::kActivator3U;
    case Button::kActivator3L:
      return m_buttons & ButtonSet::kActivator3L;
    case Button::kActivator2U:
      return m_buttons & ButtonSet::kActivator2U;
    case Button::kActivator2L:
      return m_buttons & ButtonSet::kActivator2L;
    case Button::kActivator1U:
      return m_buttons & ButtonSet::kActivator1U;
    case Button::kActivator1L:
      return m_buttons & ButtonSet::kActivator1L;
    case Button::kGraphicPen:
      return m_buttons & ButtonSet::kGraphicPen;
    case Button::kGraphicDo:
      return m_buttons & ButtonSet::kGraphicDo;
    case Button::kGraphicMenu:
      return m_buttons & ButtonSet::kGraphicMenu;
    default:
      return false;
  }
}

//------------------------------------------------------------------------------

u16 Controller::GetButtons() const
{
  return m_buttons;
}

//------------------------------------------------------------------------------

void Controller::ResetButtons()
{
  m_buttons = 0;
}
} // namespace gpgx::hid

