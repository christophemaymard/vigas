/***************************************************************************************
 *  Genesis Plus
 *
 *  Copyright (C) 1998-2003  Charles Mac Donald (original code)
 *  Copyright (C) 2007-2024  Eke-Eke (Genesis Plus GX)
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

#include "SDL.h"
#include "SDL_thread.h"

#include <stdio.h>

#include "xee/fnd/data_type.h"
#include "xee/mem/memory.h"

#include "osd.h"
#include "core/vdp/pixel.h"
#include "core/loadrom.h"
#include "core/audio_subsystem.h"
#include "core/boot_rom.h"
#include "core/core_config.h"
#include "core/framebuffer.h"
#include "core/io_reg.h"
#include "core/pico_current.h"
#include "core/region_code.h"
#include "core/rominfo.h"
#include "core/snd.h"
#include "core/system.h"
#include "core/system_bios.h"
#include "core/system_hw.h"
#include "core/system_model.h"
#include "core/viewport.h"
#include "core/ext.h" // For scd.
#include "core/genesis.h" // For gen_reset().
#include "core/vdp_ctrl.h"
#include "core/input_hw/input.h"
#include "core/cart_hw/sram.h"
#include "core/state.h"

#include "gpgx/cpu/z80/z80.h"

#include "gpgx/hid/controller_type.h"
#include "gpgx/hid/hid_system.h"
#include "gpgx/hid/input.h"
#include "gpgx/g_audio_renderer.h"
#include "gpgx/g_hid_system.h"
#include "gpgx/g_z80.h"

#define SOUND_FREQUENCY 48000
#define SOUND_SAMPLES_SIZE  2048

#define VIDEO_WIDTH  320
#define VIDEO_HEIGHT 240

int joynum = 0;

int log_error   = 0;
int debug_on    = 0;
int turbo_mode  = 0;
int use_sound   = 1;
int fullscreen  = 0; /* SDL_WINDOW_FULLSCREEN */

struct {
  SDL_Window* window;
  SDL_Surface* surf_screen;
  SDL_Surface* surf_bitmap;
  SDL_Rect srect;
  SDL_Rect drect;
  Uint32 frames_rendered;
} sdl_video;

/* sound */

struct {
  char* current_pos;
  char* buffer;
  int current_emulated_samples;
} sdl_sound;


static u8 brm_format[0x40] =
{
  0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x00,0x00,0x00,0x00,0x40,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x53,0x45,0x47,0x41,0x5f,0x43,0x44,0x5f,0x52,0x4f,0x4d,0x00,0x01,0x00,0x00,0x00,
  0x52,0x41,0x4d,0x5f,0x43,0x41,0x52,0x54,0x52,0x49,0x44,0x47,0x45,0x5f,0x5f,0x5f
};


static short soundframe[SOUND_SAMPLES_SIZE];

static void sdl_sound_callback(void *userdata, Uint8 *stream, int len)
{
  if(sdl_sound.current_emulated_samples < len) {
    xee::mem::Memset(stream, 0, len);
  }
  else {
    xee::mem::Memcpy(stream, sdl_sound.buffer, len);
    /* loop to compensate desync */
    do {
      sdl_sound.current_emulated_samples -= len;
    } while(sdl_sound.current_emulated_samples > 2 * len);
    xee::mem::Memcpy(sdl_sound.buffer,
           sdl_sound.current_pos - sdl_sound.current_emulated_samples,
           sdl_sound.current_emulated_samples);
    sdl_sound.current_pos = sdl_sound.buffer + sdl_sound.current_emulated_samples;
  }
}

static int sdl_sound_init()
{
  int n;
  SDL_AudioSpec as_desired;

  if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL Audio initialization failed", sdl_video.window);
    return 0;
  }

  as_desired.freq     = SOUND_FREQUENCY;
  as_desired.format   = AUDIO_S16SYS;
  as_desired.channels = 2;
  as_desired.samples  = SOUND_SAMPLES_SIZE;
  as_desired.callback = sdl_sound_callback;

  if(SDL_OpenAudio(&as_desired, NULL) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL Audio open failed", sdl_video.window);
    return 0;
  }

  sdl_sound.current_emulated_samples = 0;
  n = SOUND_SAMPLES_SIZE * 2 * sizeof(short) * 20;
  sdl_sound.buffer = (char*)malloc(n);
  if(!sdl_sound.buffer) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Can't allocate audio buffer", sdl_video.window);
    return 0;
  }
  xee::mem::Memset(sdl_sound.buffer, 0, n);
  sdl_sound.current_pos = sdl_sound.buffer;
  return 1;
}

static void sdl_sound_update(int enabled)
{
  int size = gpgx::g_audio_renderer->Update(soundframe) * 2;

  if (enabled)
  {
    int i;
    short *out;

    SDL_LockAudio();
    out = (short*)sdl_sound.current_pos;
    for(i = 0; i < size; i++)
    {
      *out++ = soundframe[i];
    }
    sdl_sound.current_pos = (char*)out;
    sdl_sound.current_emulated_samples += size * sizeof(short);
    SDL_UnlockAudio();
  }
}

static void sdl_sound_close()
{
  SDL_PauseAudio(1);
  SDL_CloseAudio();
  if (sdl_sound.buffer)
    free(sdl_sound.buffer);
}

/* video */

static int sdl_video_init()
{
#if defined(USE_8BPP_RENDERING)
  const unsigned long surface_format = SDL_PIXELFORMAT_RGB332;
#elif defined(USE_15BPP_RENDERING)
  const unsigned long surface_format = SDL_PIXELFORMAT_RGB555;
#elif defined(USE_16BPP_RENDERING)
  const unsigned long surface_format = SDL_PIXELFORMAT_RGB565;
#elif defined(USE_32BPP_RENDERING)
  const unsigned long surface_format = SDL_PIXELFORMAT_RGB888;
#endif

  if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL Video initialization failed", sdl_video.window);
    return 0;
  }
  sdl_video.window = SDL_CreateWindow("Genesis Plus GX", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, VIDEO_WIDTH, VIDEO_HEIGHT, fullscreen);
  sdl_video.surf_screen  = SDL_GetWindowSurface(sdl_video.window);
  sdl_video.surf_bitmap = SDL_CreateRGBSurfaceWithFormat(0, 720, 576, SDL_BITSPERPIXEL(surface_format), surface_format);
  sdl_video.frames_rendered = 0;
  SDL_ShowCursor(0);
  return 1;
}

static void sdl_video_update()
{
  if (system_hw == SYSTEM_MCD)
  {
    system_frame_scd(0);
  }
  else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
  {
    system_frame_gen(0);
  }
  else	
  {
    system_frame_sms(0);
  }

  /* viewport size changed */
  if(viewport.changed & 1)
  {
    viewport.changed &= ~1;

    /* source bitmap */
    sdl_video.srect.w = viewport.w+2*viewport.x;
    sdl_video.srect.h = viewport.h+2*viewport.y;
    sdl_video.srect.x = 0;
    sdl_video.srect.y = 0;
    if (sdl_video.srect.w > sdl_video.surf_screen->w)
    {
      sdl_video.srect.x = (sdl_video.srect.w - sdl_video.surf_screen->w) / 2;
      sdl_video.srect.w = sdl_video.surf_screen->w;
    }
    if (sdl_video.srect.h > sdl_video.surf_screen->h)
    {
      sdl_video.srect.y = (sdl_video.srect.h - sdl_video.surf_screen->h) / 2;
      sdl_video.srect.h = sdl_video.surf_screen->h;
    }

    /* destination bitmap */
    sdl_video.drect.w = sdl_video.srect.w;
    sdl_video.drect.h = sdl_video.srect.h;
    sdl_video.drect.x = (sdl_video.surf_screen->w - sdl_video.drect.w) / 2;
    sdl_video.drect.y = (sdl_video.surf_screen->h - sdl_video.drect.h) / 2;

    /* clear destination surface */
    SDL_FillRect(sdl_video.surf_screen, 0, 0);
  }

  SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.srect, sdl_video.surf_screen, &sdl_video.drect);
  SDL_UpdateWindowSurface(sdl_video.window);

  ++sdl_video.frames_rendered;
}

static void sdl_video_close()
{
  SDL_FreeSurface(sdl_video.surf_bitmap);
  SDL_DestroyWindow(sdl_video.window);
}

/* Timer Sync */

struct {
  SDL_sem* sem_sync;
  unsigned ticks;
} sdl_sync;

static Uint32 sdl_sync_timer_callback(Uint32 interval, void *param)
{
  SDL_SemPost(sdl_sync.sem_sync);
  sdl_sync.ticks++;
  if (sdl_sync.ticks == (vdp_pal ? 50 : 20))
  {
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT;
    userevent.code = vdp_pal ? (sdl_video.frames_rendered / 3) : sdl_video.frames_rendered;
    userevent.data1 = NULL;
    userevent.data2 = NULL;
    sdl_sync.ticks = sdl_video.frames_rendered = 0;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
  }
  return interval;
}

static int sdl_sync_init()
{
  if(SDL_InitSubSystem(SDL_INIT_TIMER|SDL_INIT_EVENTS) < 0)
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL Timer initialization failed", sdl_video.window);
    return 0;
  }

  sdl_sync.sem_sync = SDL_CreateSemaphore(0);
  sdl_sync.ticks = 0;
  return 1;
}

static void sdl_sync_close()
{
  if(sdl_sync.sem_sync)
    SDL_DestroySemaphore(sdl_sync.sem_sync);
}

static const u16 vc_table[4][2] =
{
  /* NTSC, PAL */
  {0xDA , 0xF2},  /* Mode 4 (192 lines) */
  {0xEA , 0x102}, /* Mode 5 (224 lines) */
  {0xDA , 0xF2},  /* Mode 4 (192 lines) */
  {0x106, 0x10A}  /* Mode 5 (240 lines) */
};

static u8 state_load_buf[STATE_SIZE];
static u8 state_save_buf[STATE_SIZE];

static int sdl_control_update(SDL_Keycode keystate)
{
    switch (keystate)
    {
      case SDLK_TAB:
      {
        system_reset();
        break;
      }

      case SDLK_F1:
      {
        if (SDL_ShowCursor(-1)) SDL_ShowCursor(0);
        else SDL_ShowCursor(1);
        break;
      }

      case SDLK_F2:
      {
        fullscreen = (fullscreen ? 0 : SDL_WINDOW_FULLSCREEN);
        SDL_SetWindowFullscreen(sdl_video.window, fullscreen);
        sdl_video.surf_screen  = SDL_GetWindowSurface(sdl_video.window);
        viewport.changed = 1;
        break;
      }

      case SDLK_F3:
      {
        if (core_config.bios == 0) core_config.bios = 3;
        else if (core_config.bios == 3) core_config.bios = 1;
        break;
      }

      case SDLK_F4:
      {
        if (!turbo_mode) use_sound ^= 1;
        break;
      }

      case SDLK_F5:
      {
        log_error ^= 1;
        break;
      }

      case SDLK_F6:
      {
        if (!use_sound)
        {
          turbo_mode ^=1;
          sdl_sync.ticks = 0;
        }
        break;
      }

      case SDLK_F7:
      {
        FILE *f = fopen("game.gp0","rb");
        if (f)
        {
          fread(&state_load_buf, STATE_SIZE, 1, f);
          state_load(state_load_buf);
          fclose(f);
        }
        break;
      }

      case SDLK_F8:
      {
        FILE *f = fopen("game.gp0","wb");
        if (f)
        {
          int len = state_save(state_save_buf);
          fwrite(&state_save_buf, len, 1, f);
          fclose(f);
        }
        break;
      }

      case SDLK_F9:
      {
        core_config.region_detect = (core_config.region_detect + 1) % 5;
        get_region(0);

        /* framerate has changed, reinitialize audio timings */
        audio_init(snd.sample_rate, 0);

        /* system with region BIOS should be reinitialized */
        if ((system_hw == SYSTEM_MCD) || ((system_hw & SYSTEM_SMS) && (core_config.bios & 1)))
        {
          system_init();
          system_reset();
        }
        else
        {
          /* reinitialize I/O region register */
          if (system_hw == SYSTEM_MD)
          {
            io_reg[0x00] = 0x20 | region_code | (core_config.bios & 1);
          }
          else
          {
            io_reg[0x00] = 0x80 | (region_code >> 1);
          }

          /* reinitialize VDP */
          if (vdp_pal)
          {
            status |= 1;
            lines_per_frame = 313;
          }
          else
          {
            status &= ~1;
            lines_per_frame = 262;
          }

          /* reinitialize VC max value */
          switch (viewport.h)
          {
            case 192:
              vc_max = vc_table[0][vdp_pal];
              break;
            case 224:
              vc_max = vc_table[1][vdp_pal];
              break;
            case 240:
              vc_max = vc_table[3][vdp_pal];
              break;
          }
        }
        break;
      }

      case SDLK_F10:
      {
        gen_reset(0);
        break;
      }

      case SDLK_F11:
      {
        core_config.overscan =  (core_config.overscan + 1) & 3;
        if ((system_hw == SYSTEM_GG) && !core_config.gg_extra)
        {
          viewport.x = (core_config.overscan & 2) ? 14 : -48;
        }
        else
        {
          viewport.x = (core_config.overscan & 2) * 7;
        }
        viewport.changed = 3;
        break;
      }

      case SDLK_F12:
      {
        joynum = (joynum + 1) % MAX_DEVICES;
        while (gpgx::g_hid_system->GetController(joynum)->GetType() == gpgx::hid::ControllerType::kNone)
        {
          joynum = (joynum + 1) % MAX_DEVICES;
        }
        break;
      }

      case SDLK_ESCAPE:
      {
        return 0;
      }

      default:
        break;
    }

   return 1;
}

int sdl_input_update(void)
{
  const u8 *keystate = SDL_GetKeyboardState(NULL);

  // Retrieve the controller.
  const auto controller = gpgx::g_hid_system->GetController(joynum);

  /* reset input */
  controller->ResetButtons();

  switch (controller->GetType())
  {
    case gpgx::hid::ControllerType::kLightGun:
    {
      /* get mouse coordinates (absolute values) */
      int x,y;
      int state = SDL_GetMouseState(&x,&y);

      /* X axis */
      input.analog[joynum][0] =  x - (sdl_video.surf_screen->w-viewport.w)/2;

      /* Y axis */
      input.analog[joynum][1] =  y - (sdl_video.surf_screen->h-viewport.h)/2;

      /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
      if(state & SDL_BUTTON_LMASK) controller->PressButton(gpgx::hid::Button::kA);
      if(state & SDL_BUTTON_RMASK) controller->PressButton(gpgx::hid::Button::kB);
      if(state & SDL_BUTTON_MMASK) controller->PressButton(gpgx::hid::Button::kC);
      if(keystate[SDL_SCANCODE_F])  controller->PressButton(gpgx::hid::Button::kStart);
      break;
    }

    case gpgx::hid::ControllerType::kPaddle:
    {
      /* get mouse (absolute values) */
      int x;
      int state = SDL_GetMouseState(&x, NULL);

      /* Range is [0;256], 128 being middle position */
      input.analog[joynum][0] = x * 256 /sdl_video.surf_screen->w;

      /* Button I -> 0 0 0 0 0 0 0 I*/
      if(state & SDL_BUTTON_LMASK) controller->PressButton(gpgx::hid::Button::kB);

      break;
    }

    case gpgx::hid::ControllerType::kSportsPad:
    {
      /* get mouse (relative values) */
      int x,y;
      int state = SDL_GetRelativeMouseState(&x,&y);

      /* Range is [0;256] */
      input.analog[joynum][0] = (unsigned char)(-x & 0xFF);
      input.analog[joynum][1] = (unsigned char)(-y & 0xFF);

      /* Buttons I & II -> 0 0 0 0 0 0 II I*/
      if(state & SDL_BUTTON_LMASK) controller->PressButton(gpgx::hid::Button::kB);
      if(state & SDL_BUTTON_RMASK) controller->PressButton(gpgx::hid::Button::kC);

      break;
    }

    case gpgx::hid::ControllerType::kMouse:
    {
      /* get mouse (relative values) */
      int x,y;
      int state = SDL_GetRelativeMouseState(&x,&y);

      /* Sega Mouse range is [-256;+256] */
      input.analog[joynum][0] = x * 2;
      input.analog[joynum][1] = y * 2;

      /* Vertical movement is upsidedown */
      if (!app_config.invert_mouse)
        input.analog[joynum][1] = 0 - input.analog[joynum][1];

      /* Start,Left,Right,Middle buttons -> 0 0 0 0 START MIDDLE RIGHT LEFT */
      if(state & SDL_BUTTON_LMASK) controller->PressButton(gpgx::hid::Button::kB);
      if(state & SDL_BUTTON_RMASK) controller->PressButton(gpgx::hid::Button::kC);
      if(state & SDL_BUTTON_MMASK) controller->PressButton(gpgx::hid::Button::kA);
      if(keystate[SDL_SCANCODE_F])  controller->PressButton(gpgx::hid::Button::kStart);

      break;
    }

    case gpgx::hid::ControllerType::kXe1Ap:
    {
      /* A,B,C,D,Select,START,E1,E2 buttons -> E1(?) E2(?) START SELECT(?) A B C D */
      if(keystate[SDL_SCANCODE_A])  controller->PressButton(gpgx::hid::Button::kStart);
      if(keystate[SDL_SCANCODE_S])  controller->PressButton(gpgx::hid::Button::kA);
      if(keystate[SDL_SCANCODE_D])  controller->PressButton(gpgx::hid::Button::kC);
      if(keystate[SDL_SCANCODE_F])  controller->PressButton(gpgx::hid::Button::kY);
      if(keystate[SDL_SCANCODE_Z])  controller->PressButton(gpgx::hid::Button::kB);
      if(keystate[SDL_SCANCODE_X])  controller->PressButton(gpgx::hid::Button::kX);
      if(keystate[SDL_SCANCODE_C])  controller->PressButton(gpgx::hid::Button::kMode);
      if(keystate[SDL_SCANCODE_V])  controller->PressButton(gpgx::hid::Button::kZ);

      /* Left Analog Stick (bidirectional) */
      if(keystate[SDL_SCANCODE_UP])     input.analog[joynum][1]-=2;
      else if(keystate[SDL_SCANCODE_DOWN])   input.analog[joynum][1]+=2;
      else input.analog[joynum][1] = 128;
      if(keystate[SDL_SCANCODE_LEFT])   input.analog[joynum][0]-=2;
      else if(keystate[SDL_SCANCODE_RIGHT])  input.analog[joynum][0]+=2;
      else input.analog[joynum][0] = 128;

      /* Right Analog Stick (unidirectional) */
      if(keystate[SDL_SCANCODE_KP_8])    input.analog[joynum+1][0]-=2;
      else if(keystate[SDL_SCANCODE_KP_2])   input.analog[joynum+1][0]+=2;
      else if(keystate[SDL_SCANCODE_KP_4])   input.analog[joynum+1][0]-=2;
      else if(keystate[SDL_SCANCODE_KP_6])  input.analog[joynum+1][0]+=2;
      else input.analog[joynum+1][0] = 128;

      /* Limiters */
      if (input.analog[joynum][0] > 0xFF) input.analog[joynum][0] = 0xFF;
      else if (input.analog[joynum][0] < 0) input.analog[joynum][0] = 0;
      if (input.analog[joynum][1] > 0xFF) input.analog[joynum][1] = 0xFF;
      else if (input.analog[joynum][1] < 0) input.analog[joynum][1] = 0;
      if (input.analog[joynum+1][0] > 0xFF) input.analog[joynum+1][0] = 0xFF;
      else if (input.analog[joynum+1][0] < 0) input.analog[joynum+1][0] = 0;
      if (input.analog[joynum+1][1] > 0xFF) input.analog[joynum+1][1] = 0xFF;
      else if (input.analog[joynum+1][1] < 0) input.analog[joynum+1][1] = 0;

      break;
    }

    case gpgx::hid::ControllerType::kPico:
    {
      /* get mouse (absolute values) */
      int x,y;
      int state = SDL_GetMouseState(&x,&y);

      // Retrieve the first controller.
      // (PICO tablet should be always connected to index 0)
      const auto first_controller = gpgx::g_hid_system->GetController(0);

      /* Calculate X Y axis values */
      input.analog[0][0] = 0x3c  + (x * (0x17c-0x03c+1)) / sdl_video.surf_screen->w;
      input.analog[0][1] = 0x1fc + (y * (0x2f7-0x1fc+1)) / sdl_video.surf_screen->h;

      /* Map mouse buttons to player #1 inputs */
      if(state & SDL_BUTTON_MMASK) pico_current = (pico_current + 1) & 7;
      if(state & SDL_BUTTON_RMASK) first_controller->PressButton(gpgx::hid::Button::kPicoRed);
      if(state & SDL_BUTTON_LMASK) first_controller->PressButton(gpgx::hid::Button::kPicoPen);

      break;
    }

    case gpgx::hid::ControllerType::kTerebi:
    {
      /* get mouse (absolute values) */
      int x,y;
      int state = SDL_GetMouseState(&x,&y);

      // Retrieve the first controller.
      // (Terebi Oekaki tablet should be always connected to index 0)
      const auto first_controller = gpgx::g_hid_system->GetController(0);

      /* Calculate X Y axis values */
      input.analog[0][0] = (x * 250) / sdl_video.surf_screen->w;
      input.analog[0][1] = (y * 250) / sdl_video.surf_screen->h;

      /* Map mouse buttons to player #1 inputs */
      if(state & SDL_BUTTON_RMASK) first_controller->PressButton(gpgx::hid::Button::kB);

      break;
    }

    case gpgx::hid::ControllerType::kGraphicBoard:
    {
      /* get mouse (absolute values) */
      int x,y;
      int state = SDL_GetMouseState(&x,&y);

      // Retrieve the first controller.
      // @todo  Check where the Graphic Board can be connected, differences between sdl_input_update(), input_init(), input_reset() and graphic_board_reset().
      const auto first_controller = gpgx::g_hid_system->GetController(0);

      /* Calculate X Y axis values */
      input.analog[0][0] = (x * 255) / sdl_video.surf_screen->w;
      input.analog[0][1] = (y * 255) / sdl_video.surf_screen->h;

      /* Map mouse buttons to player #1 inputs */
      if(state & SDL_BUTTON_LMASK) first_controller->PressButton(gpgx::hid::Button::kGraphicPen);
      if(state & SDL_BUTTON_RMASK) first_controller->PressButton(gpgx::hid::Button::kGraphicMenu);
      if(state & SDL_BUTTON_MMASK) first_controller->PressButton(gpgx::hid::Button::kGraphicDo);

      break;
    }

    case gpgx::hid::ControllerType::kActivator:
    {
      if(keystate[SDL_SCANCODE_G])  controller->PressButton(gpgx::hid::Button::kActivator7L);
      if(keystate[SDL_SCANCODE_H])  controller->PressButton(gpgx::hid::Button::kActivator7U);
      if(keystate[SDL_SCANCODE_J])  controller->PressButton(gpgx::hid::Button::kActivator8L);
      if(keystate[SDL_SCANCODE_K])  controller->PressButton(gpgx::hid::Button::kActivator8U);
    }

    default:
    {
      if(keystate[SDL_SCANCODE_A])  controller->PressButton(gpgx::hid::Button::kA);
      if(keystate[SDL_SCANCODE_S])  controller->PressButton(gpgx::hid::Button::kB);
      if(keystate[SDL_SCANCODE_D])  controller->PressButton(gpgx::hid::Button::kC);
      if(keystate[SDL_SCANCODE_F])  controller->PressButton(gpgx::hid::Button::kStart);
      if(keystate[SDL_SCANCODE_Z])  controller->PressButton(gpgx::hid::Button::kX);
      if(keystate[SDL_SCANCODE_X])  controller->PressButton(gpgx::hid::Button::kY);
      if(keystate[SDL_SCANCODE_C])  controller->PressButton(gpgx::hid::Button::kZ);
      if(keystate[SDL_SCANCODE_V])  controller->PressButton(gpgx::hid::Button::kMode);

      if(keystate[SDL_SCANCODE_UP]) controller->PressButton(gpgx::hid::Button::kUp);
      else
      if(keystate[SDL_SCANCODE_DOWN]) controller->PressButton(gpgx::hid::Button::kDown);
      if(keystate[SDL_SCANCODE_LEFT]) controller->PressButton(gpgx::hid::Button::kLeft);
      else
      if(keystate[SDL_SCANCODE_RIGHT]) controller->PressButton(gpgx::hid::Button::kRight);

      break;
    }
  }
  return 1;
}


int main (int argc, char **argv)
{
  FILE *fp;
  int running = 1;

  /* Print help if no game specified */
  if(argc < 2)
  {
    char caption[256];
    sprintf(caption, "Genesis Plus GX\\SDL\nusage: %s gamename\n", argv[0]);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Information", caption, sdl_video.window);
    return 1;
  }

  // 
  gpgx::g_z80 = new gpgx::cpu::z80::Z80();

  // Initialize the HID system.
  gpgx::g_hid_system = new gpgx::hid::HIDSystem();
  gpgx::g_hid_system->Initialize();

  /* set default config */
  error_init();
  set_config_defaults();

  /* mark all BIOS as unloaded */
  system_bios = 0;

  /* Genesis BOOT ROM support (2KB max) */
  xee::mem::Memset(boot_rom, 0xFF, 0x800);
  fp = fopen(MD_BIOS, "rb");
  if (fp != NULL)
  {
    int i;

    /* read BOOT ROM */
    fread(boot_rom, 1, 0x800, fp);
    fclose(fp);

    /* check BOOT ROM */
    if (!xee::mem::Memcmp((char *)(boot_rom + 0x120),"GENESIS OS", 10))
    {
      /* mark Genesis BIOS as loaded */
      system_bios = SYSTEM_MD;
    }

    /* Byteswap ROM */
    for (i=0; i<0x800; i+=2)
    {
      u8 temp = boot_rom[i];
      boot_rom[i] = boot_rom[i+1];
      boot_rom[i+1] = temp;
    }
  }

  /* initialize SDL */
  if(SDL_Init(0) < 0)
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL initialization failed", sdl_video.window);
    return 1;
  }
  sdl_video_init();
  if (use_sound) sdl_sound_init();
  sdl_sync_init();

  /* initialize Genesis virtual system */
  SDL_LockSurface(sdl_video.surf_bitmap);
  xee::mem::Memset(&framebuffer, 0, sizeof(framebuffer));
  framebuffer.width        = 720;
  framebuffer.height       = 576;
  framebuffer.pitch        = framebuffer.width * sizeof(PIXEL_OUT_T);
  framebuffer.data         = (u8*)sdl_video.surf_bitmap->pixels;
  SDL_UnlockSurface(sdl_video.surf_bitmap);
  viewport.changed = 3;

  /* Load game file */
  if(!load_rom(argv[1]))
  {
    char caption[256];
    sprintf(caption, "Error loading file `%s'.", argv[1]);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", caption, sdl_video.window);
    return 1;
  }

  /* initialize system hardware */
  audio_init(SOUND_FREQUENCY, 0);
  system_init();

  /* Mega CD specific */
  if (system_hw == SYSTEM_MCD)
  {
    /* load internal backup RAM */
    fp = fopen("./scd.brm", "rb");
    if (fp!=NULL)
    {
      fread(scd.bram, 0x2000, 1, fp);
      fclose(fp);
    }

    /* check if internal backup RAM is formatted */
    if (xee::mem::Memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20))
    {
      /* clear internal backup RAM */
      xee::mem::Memset(scd.bram, 0x00, 0x200);

      /* Internal Backup RAM size fields */
      brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = 0x00;
      brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (sizeof(scd.bram) / 64) - 3;

      /* format internal backup RAM */
      xee::mem::Memcpy(scd.bram + 0x2000 - 0x40, brm_format, 0x40);
    }

    /* load cartridge backup RAM */
    if (scd.cartridge.id)
    {
      fp = fopen("./cart.brm", "rb");
      if (fp!=NULL)
      {
        fread(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
        fclose(fp);
      }

      /* check if cartridge backup RAM is formatted */
      if (xee::mem::Memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20))
      {
        /* clear cartridge backup RAM */
        xee::mem::Memset(scd.cartridge.area, 0x00, scd.cartridge.mask + 1);

        /* Cartridge Backup RAM size fields */
        brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = (((scd.cartridge.mask + 1) / 64) - 3) >> 8;
        brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (((scd.cartridge.mask + 1) / 64) - 3) & 0xff;

        /* format cartridge backup RAM */
        xee::mem::Memcpy(scd.cartridge.area + scd.cartridge.mask + 1 - sizeof(brm_format), brm_format, sizeof(brm_format));
      }
    }
  }

  if (sram.on)
  {
    /* load SRAM */
    fp = fopen("./game.srm", "rb");
    if (fp!=NULL)
    {
      fread(sram.sram,0x10000,1, fp);
      fclose(fp);
    }
  }

  /* reset system hardware */
  system_reset();

  if(use_sound) SDL_PauseAudio(0);

  /* 3 frames = 50 ms (60hz) or 60 ms (50hz) */
  if(sdl_sync.sem_sync)
    SDL_AddTimer(vdp_pal ? 60 : 50, sdl_sync_timer_callback, NULL);

  /* emulation loop */
  while(running)
  {
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
      switch(event.type)
      {
        case SDL_USEREVENT:
        {
          char caption[100];
          sprintf(caption,"Genesis Plus GX - %d fps - %s", event.user.code, (rominfo.international[0] != 0x20) ? rominfo.international : rominfo.domestic);
          SDL_SetWindowTitle(sdl_video.window, caption);
          break;
        }

        case SDL_QUIT:
        {
          running = 0;
          break;
        }

        case SDL_KEYDOWN:
        {
          running = sdl_control_update(event.key.keysym.sym);
          break;
        }
      }
    }

    sdl_video_update();
    sdl_sound_update(use_sound);

    if(!turbo_mode && sdl_sync.sem_sync && sdl_video.frames_rendered % 3 == 0)
    {
      SDL_SemWait(sdl_sync.sem_sync);
    }
  }

  if (system_hw == SYSTEM_MCD)
  {
    /* save internal backup RAM (if formatted) */
    if (!xee::mem::Memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20))
    {
      fp = fopen("./scd.brm", "wb");
      if (fp!=NULL)
      {
        fwrite(scd.bram, 0x2000, 1, fp);
        fclose(fp);
      }
    }

    /* save cartridge backup RAM (if formatted) */
    if (scd.cartridge.id)
    {
      if (!xee::mem::Memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20))
      {
        fp = fopen("./cart.brm", "wb");
        if (fp!=NULL)
        {
          fwrite(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
          fclose(fp);
        }
      }
    }
  }

  if (sram.on)
  {
    /* save SRAM */
    fp = fopen("./game.srm", "wb");
    if (fp!=NULL)
    {
      fwrite(sram.sram,0x10000,1, fp);
      fclose(fp);
    }
  }

  audio_shutdown();
  error_shutdown();

  sdl_video_close();
  sdl_sound_close();
  sdl_sync_close();
  SDL_Quit();

  // 
  if (gpgx::g_z80) {
    delete gpgx::g_z80;
    gpgx::g_z80 = nullptr;
  }

  return 0;
}
