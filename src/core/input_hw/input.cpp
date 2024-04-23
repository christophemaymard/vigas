/***************************************************************************************
 *  Genesis Plus
 *  Input peripherals support
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

#include "core/input_hw/input.h"

#include "core/core_config.h"
#include "core/rominfo.h"
#include "core/romtype.h"
#include "core/system_hw.h"
#include "core/system_model.h"
#include "core/ext.h" // For cart.
#include "core/cart_hw/special_hw_md.h"
#include "core/cart_hw/special_hw_sms.h"

#include "core/input_hw/gamepad.h"
#include "core/input_hw/lightgun.h"
#include "core/input_hw/mouse.h"
#include "core/input_hw/activator.h"
#include "core/input_hw/xe_1ap.h"
#include "core/input_hw/teamplayer.h"
#include "core/input_hw/paddle.h"
#include "core/input_hw/sportspad.h"
#include "core/input_hw/terebi_oekaki.h"
#include "core/input_hw/graphic_board.h"

#include "gpgx/hid/controller_type.h"
#include "gpgx/hid/device_type.h"
#include "gpgx/g_hid_system.h"

t_input input = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
  0,
  0
};


void input_init(void)
{
  int i;
  int player = 0;

  for (i=0; i<MAX_DEVICES; i++)
  {
    input.pad[i] = 0;
  }

  gpgx::g_hid_system->DisconnectAllControllers();

  /* PICO tablet */
  if (system_hw == SYSTEM_PICO)
  {
    gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kPico);
    return;
  }

  /* Terebi Oekaki tablet */
  if (cart.special & HW_TEREBI_OEKAKI)
  {
    gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kTerebi);
    return;
  }

  // Default gamepad type (2-buttons, 3-buttons or 6-buttons).
  gpgx::hid::ControllerType padtype = gpgx::hid::ControllerType::kNone;

  if (romtype & SYSTEM_MD)
  {
    /* 3-buttons or 6-buttons */
    padtype = (rominfo.peripherals & 2) ? gpgx::hid::ControllerType::kPad6B : gpgx::hid::ControllerType::kPad3B;
  }
  else
  {
    /* 2-buttons */
    padtype = gpgx::hid::ControllerType::kPad2B;
  }

  switch (gpgx::g_hid_system->GetDevice(0)->GetType())
  {
    case gpgx::hid::DeviceType::kGamepad:
    {
      gpgx::g_hid_system->ConnectController(0, padtype);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kMouse:
    {
      gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kMouse);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kActivator:
    {
      gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kActivator);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kXe1Ap:
    {
      gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kXe1Ap);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kWayPlay:
    {
      for (i=0; i< 4; i++)
      {
        if (player < MAX_DEVICES)
        {
          /* only allow 3-buttons or 6-buttons control pad */
          gpgx::g_hid_system->ConnectController(i, (padtype == gpgx::hid::ControllerType::kPad2B) ? gpgx::hid::ControllerType::kPad3B : padtype);
          player++;
        }
      }
      break;
    }

    case gpgx::hid::DeviceType::kTeamPlayer:
    {
      for (i=0; i<4; i++)
      {
        if (player < MAX_DEVICES)
        {
          /* only allow 3-buttons or 6-buttons control pad */
          gpgx::g_hid_system->ConnectController(i, (padtype == gpgx::hid::ControllerType::kPad2B) ? gpgx::hid::ControllerType::kPad3B : padtype);
          player++;
        }
      }
      teamplayer_init(0);
      break;
    }

    case gpgx::hid::DeviceType::kMasterTap:
    {
      for (i=0; i<4; i++)
      {
        if (player < MAX_DEVICES)
        {
          gpgx::g_hid_system->ConnectController(i, gpgx::hid::ControllerType::kPad2B);
          player++;
        }
      }
      break;
    }

    case gpgx::hid::DeviceType::kLightPhaser:
    {
      gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kLightGun);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kPaddle:
    {
      gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kPaddle);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kSportsPad:
    {
      gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kSportsPad);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kGraphicBoard:
    {
      gpgx::g_hid_system->ConnectController(0, gpgx::hid::ControllerType::kGraphicBoard);
      player++;
      break;
    }
  }

  if (player == MAX_DEVICES)
  {
    return;
  }

  switch (gpgx::g_hid_system->GetDevice(1)->GetType())
  {
    case gpgx::hid::DeviceType::kGamepad:
    {
      gpgx::g_hid_system->ConnectController(4, padtype);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kMouse:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kMouse);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kActivator:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kActivator);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kXe1Ap:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kXe1Ap);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kMenacer:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kLightGun);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kJustifier:
    {
      for (i=4; i<6; i++)
      {
        if (player < MAX_DEVICES)
        {
          gpgx::g_hid_system->ConnectController(i, gpgx::hid::ControllerType::kLightGun);
          player++;
        }
      }
      break;
    }

    case gpgx::hid::DeviceType::kTeamPlayer:
    {
      for (i=4; i<8; i++)
      {
        if (player < MAX_DEVICES)
        {
          /* only allow 3-buttons or 6-buttons control pad */
          gpgx::g_hid_system->ConnectController(i, (padtype == gpgx::hid::ControllerType::kPad2B) ? gpgx::hid::ControllerType::kPad3B : padtype);
          player++;
        }
      }
      teamplayer_init(1);
      break;
    }

    case gpgx::hid::DeviceType::kMasterTap:
    {
      for (i=4; i<8; i++)
      {
        if (player < MAX_DEVICES)
        {
          gpgx::g_hid_system->ConnectController(i, gpgx::hid::ControllerType::kPad2B);
          player++;
        }
      }
      break;
    }

    case gpgx::hid::DeviceType::kLightPhaser:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kLightGun);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kPaddle:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kPaddle);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kSportsPad:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kSportsPad);
      player++;
      break;
    }

    case gpgx::hid::DeviceType::kGraphicBoard:
    {
      gpgx::g_hid_system->ConnectController(4, gpgx::hid::ControllerType::kGraphicBoard);
      player++;
      break;
    }
  }

  /* J-CART */
  if (cart.special & HW_J_CART)
  {
    /* two additional gamepads */
    for (i=5; i<7; i++)
    {
      if (player < MAX_DEVICES)
      {
        /* only allow 3-buttons or 6-buttons control pad */
        gpgx::g_hid_system->ConnectController(i, (padtype == gpgx::hid::ControllerType::kPad2B) ? gpgx::hid::ControllerType::kPad3B : padtype);
        player ++;
      }
    }
  }
}

void input_reset(void)
{
  /* Reset input devices */
  int i;
  for (i=0; i<MAX_DEVICES; i++)
  {
    switch (gpgx::g_hid_system->GetController(i)->GetType())
    {
      case gpgx::hid::ControllerType::kPad2B:
      case gpgx::hid::ControllerType::kPad3B:
      case gpgx::hid::ControllerType::kPad6B:
      {
        gamepad_reset(i);
        break;
      }

      case gpgx::hid::ControllerType::kLightGun:
      {
        lightgun_reset(i);
        break;
      }

      case gpgx::hid::ControllerType::kMouse:
      {
        mouse_reset(i);
        break;
      }

      case gpgx::hid::ControllerType::kActivator:
      {
        activator_reset(i >> 2);
        break;
      }

      case gpgx::hid::ControllerType::kXe1Ap:
      {
        xe_1ap_reset(i);
        break;
      }

      case gpgx::hid::ControllerType::kPaddle:
      {
        paddle_reset(i);
        break;
      }

      case gpgx::hid::ControllerType::kSportsPad:
      {
        sportspad_reset(i);
        break;
      }

      case gpgx::hid::ControllerType::kTerebi:
      {
        terebi_oekaki_reset();
        break;
      }

      case gpgx::hid::ControllerType::kGraphicBoard:
      {
        graphic_board_reset(i);
        break;
      }

      default:
      {
        break;
      }
    }
  }

  /* Team Player */
  for (i=0; i<2; i++)
  {
    if (gpgx::g_hid_system->GetDevice(i)->GetType() == gpgx::hid::DeviceType::kTeamPlayer)
    {
      teamplayer_reset(i);
    }
  }
}

void input_refresh(void)
{
  int i;
  for (i=0; i<MAX_DEVICES; i++)
  {
    switch (gpgx::g_hid_system->GetController(i)->GetType())
    {
      case gpgx::hid::ControllerType::kPad6B:
      {
        gamepad_refresh(i);
        break;
      }

      case gpgx::hid::ControllerType::kLightGun:
      {
        lightgun_refresh(i);
        break;
      }
    }
  }
}

void input_end_frame(unsigned int cycles)
{
  int i;
  for (i=0; i<MAX_DEVICES; i++)
  {
    switch (gpgx::g_hid_system->GetController(i)->GetType())
    {
      case gpgx::hid::ControllerType::kPad3B:
      case gpgx::hid::ControllerType::kPad6B:
      {
        gamepad_end_frame(i, cycles);
        break;
      }
    }
  }
}