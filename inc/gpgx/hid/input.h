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

#ifndef __GPGX_HID_INPUT_H__
#define __GPGX_HID_INPUT_H__

#include "xee/fnd/data_type.h"

namespace gpgx::hid {

//==============================================================================

//------------------------------------------------------------------------------

enum class Button
{
  kMode        , /// Button Mode.
  kX           , /// Button X.
  kY           , /// Button Y.
  kZ           , /// Button Z.
  kStart       , /// Button Start.
  kA           , /// Button A.
  kC           , /// Button C.
  kB           , /// Button B.
  kRight       , /// Button Right.
  kLeft        , /// Button Left.
  kDown        , /// Button Down.
  kUp          , /// Button Up.

  kButton2     , /// (Master System specific).
  kButton1     , /// (Master System specific).

  kMouseCenter , /// (Mega Mouse specific).
  kMouseRight  , /// (Mega Mouse specific).
  kMouseLeft   , /// (Mega Mouse specific).

  kPicoPen     , /// (Pico hardware specific).
  kPicoRed     , /// (Pico hardware specific).

  kXeE1        , /// (XE-1AP specific).
  kXeE2        , /// (XE-1AP specific).
  kXeStart     , /// (XE-1AP specific).
  kXeSelect    , /// (XE-1AP specific).
  kXeA         , /// (XE-1AP specific).
  kXeB         , /// (XE-1AP specific).
  kXeA2        , /// (XE-1AP specific).
  kXeB2        , /// (XE-1AP specific).
  kXeC         , /// (XE-1AP specific).
  kXeD         , /// (XE-1AP specific).

  kActivator8U , /// (Activator specific).
  kActivator8L , /// (Activator specific).
  kActivator7U , /// (Activator specific).
  kActivator7L , /// (Activator specific).
  kActivator6U , /// (Activator specific).
  kActivator6L , /// (Activator specific).
  kActivator5U , /// (Activator specific).
  kActivator5L , /// (Activator specific).
  kActivator4U , /// (Activator specific).
  kActivator4L , /// (Activator specific).
  kActivator3U , /// (Activator specific).
  kActivator3L , /// (Activator specific).
  kActivator2U , /// (Activator specific).
  kActivator2L , /// (Activator specific).
  kActivator1U , /// (Activator specific).
  kActivator1L , /// (Activator specific).

  kGraphicPen  , /// (Graphic Board specific).
  kGraphicDo   , /// (Graphic Board specific).
  kGraphicMenu , /// (Graphic Board specific).
};

//------------------------------------------------------------------------------

struct ButtonSet
{
  static constexpr u16 kMode        = 0x0800; /// Button Mode.
  static constexpr u16 kX           = 0x0400; /// Button X.
  static constexpr u16 kY           = 0x0200; /// Button Y.
  static constexpr u16 kZ           = 0x0100; /// Button Z.
  static constexpr u16 kStart       = 0x0080; /// Button Start.
  static constexpr u16 kA           = 0x0040; /// Button A.
  static constexpr u16 kC           = 0x0020; /// Button C.
  static constexpr u16 kB           = 0x0010; /// Button B.
  static constexpr u16 kRight       = 0x0008; /// Button Right.
  static constexpr u16 kLeft        = 0x0004; /// Button Left.
  static constexpr u16 kDown        = 0x0002; /// Button Down.
  static constexpr u16 kUp          = 0x0001; /// Button Up.

  static constexpr u16 kButton2     = kC;     /// (inherited from button C)(Master System specific).
  static constexpr u16 kButton1     = kB;     /// (inherited from button B)(Master System specific).

  static constexpr u16 kMouseCenter = kA;     /// (inherited from button A)(Mega Mouse specific).
  static constexpr u16 kMouseRight  = kC;     /// (inherited from button C)(Mega Mouse specific).
  static constexpr u16 kMouseLeft   = kB;     /// (inherited from button B)(Mega Mouse specific).

  static constexpr u16 kPicoPen     = kStart; /// (inherited from button Start)(Pico hardware specific).
  static constexpr u16 kPicoRed     = kB;     /// (inherited from button B)(Pico hardware specific).

  static constexpr u16 kXeE1        = 0x2000; /// (XE-1AP specific).
  static constexpr u16 kXeE2        = 0x1000; /// (XE-1AP specific).
  static constexpr u16 kXeStart     = kMode;  /// (inherited from button Mode)(XE-1AP specific).
  static constexpr u16 kXeSelect    = kX;     /// (inherited from button X)(XE-1AP specific).
  static constexpr u16 kXeA         = kY;     /// (inherited from button Y)(XE-1AP specific).
  static constexpr u16 kXeB         = kZ;     /// (inherited from button Z)(XE-1AP specific).
  static constexpr u16 kXeA2        = kStart; /// (inherited from button Start)(XE-1AP specific).
  static constexpr u16 kXeB2        = kA;     /// (inherited from button A)(XE-1AP specific).
  static constexpr u16 kXeC         = kC;     /// (inherited from button C)(XE-1AP specific).
  static constexpr u16 kXeD         = kB;     /// (inherited from button B)(XE-1AP specific).

  static constexpr u16 kActivator8U = 0x8000; /// (Activator specific).
  static constexpr u16 kActivator8L = 0x4000; /// (Activator specific).
  static constexpr u16 kActivator7U = 0x2000; /// (Activator specific).
  static constexpr u16 kActivator7L = 0x1000; /// (Activator specific).
  static constexpr u16 kActivator6U = kMode;  /// (inherited from button Mode)(Activator specific).
  static constexpr u16 kActivator6L = kX;     /// (inherited from button X)(Activator specific).
  static constexpr u16 kActivator5U = kY;     /// (inherited from button Y)(Activator specific).
  static constexpr u16 kActivator5L = kZ;     /// (inherited from button Z)(Activator specific).
  static constexpr u16 kActivator4U = kStart; /// (inherited from button Start)(Activator specific).
  static constexpr u16 kActivator4L = kA;     /// (inherited from button A)(Activator specific).
  static constexpr u16 kActivator3U = kC;     /// (inherited from button C)(Activator specific).
  static constexpr u16 kActivator3L = kB;     /// (inherited from button B)(Activator specific).
  static constexpr u16 kActivator2U = kRight; /// (inherited from button Right)(Activator specific).
  static constexpr u16 kActivator2L = kLeft;  /// (inherited from button Left)(Activator specific).
  static constexpr u16 kActivator1U = kDown;  /// (inherited from button Down)(Activator specific).
  static constexpr u16 kActivator1L = kUp;    /// (inherited from button Up)(Activator specific).

  static constexpr u16 kGraphicPen  = kLeft;  /// (inherited from button Left)(Graphic Board specific).
  static constexpr u16 kGraphicDo   = kDown;  /// (inherited from button Down)(Graphic Board specific).
  static constexpr u16 kGraphicMenu = kUp;    /// (inherited from button Up)(Graphic Board specific).
};

} // namespace gpgx::hid

#endif // #ifndef __GPGX_HID_INPUT_H__

