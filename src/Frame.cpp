/*
AppleWin : An Apple //e emulator for Windows

Copyright (C) 1994-1996, Michael O'Brien
Copyright (C) 1999-2001, Oliver Schmidt
Copyright (C) 2002-2005, Tom Charlesworth
Copyright (C) 2006-2007, Tom Charlesworth, Michael Pohoreski

AppleWin is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppleWin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppleWin; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Description: Frame
 *
 * Author: Various
 */

/* Adaptation for SDL and POSIX (l) by beom beotiger, Nov-Dec 2007 */

/* And KREZ */

// #include <iostream>

#include "arduinoshim.h"

#include "stdafx.h"
#include "asset.h"

#define ENABLE_MENU 0

SDL::Surface *apple_icon;
SDL::Surface *screen;  // our main screen
// rects for screen stretch if needed
SDL::Rect origRect;
SDL::Rect newRect;

#define  VIEWPORTCX  560
#if ENABLE_MENU
#define  VIEWPORTCY  400
#else
#define  VIEWPORTCY  384
#endif
#define  BUTTONX     (VIEWPORTCX+(VIEWPORTX<<1))
#define  BUTTONY     0
#define  BUTTONCX    45
#define  BUTTONCY    45
#define  FSVIEWPORTX (640-BUTTONCX-VIEWPORTX-VIEWPORTCX)
#define  FSVIEWPORTY ((480-VIEWPORTCY)>>1)
#define  FSBUTTONX   (640-BUTTONCX)
#define  FSBUTTONY   (((480-VIEWPORTCY)>>1)-1)
#define  BUTTONS     8

static bool g_bAppActive = false;

static int buttondown = -1;

bool fullscreen = 0;
bool g_WindowResized;  // if we do not have a normal window size

static bool usingcursor = 0;

void DrawStatusArea(int drawflags);

void ProcessButtonClick(int button, int mod); // handle control buttons(F1-..F12) events

void ResetMachineState();

void SetFullScreenMode();

void SetNormalMode();

void SetUsingCursor(bool);

bool g_bScrollLock_FullSpeed = false;  // no in full speed!

void DrawFrameWindow()
{
  VideoRealizePalette(/*dc*/);

  // DRAW THE STATUS AREA
  DrawStatusArea(DRAW_BACKGROUND | DRAW_LEDS);

  // DRAW THE CONTENTS OF THE EMULATED SCREEN
  if (g_nAppMode == MODE_LOGO) {
    VideoDisplayLogo();
  }
  /* else if (g_nAppMode == MODE_DEBUG) {
    DebugDisplay(1);
  }*/
   else {
    VideoRedrawScreen(); // normal state - running emulator?
  }
}

void DrawStatusArea(int drawflags)
{
  if (font_sfc == NULL) {
    if (!fonts_initialization()) {
      ErrDumpln("Font file was not loaded.");
      return; // if we don't have a font, we just can do none
    }
  }

  SDL::Rect srect;
  Uint32 mybluez = SDL::MapRGB(screen->format, 10, 10, 255);  // bluez color, know that?
  SDL::SetColors(g_hStatusSurface, screen->format->palette->colors, 0, 256);

  if (drawflags & DRAW_BACKGROUND) {
    g_iStatusCycle = SHOW_CYCLES;  // start cycle for panel showing
  }
  if (drawflags & DRAW_LEDS) {
    srect.x = 4;
    srect.y = 22;
    srect.w = STATUS_PANEL_W - 8;
    srect.h = STATUS_PANEL_H - 25;
    SDL::FillRect(g_hStatusSurface, &srect, mybluez);  // clear

    char leds[2] = "\x64";
    #define LEDS  1
    int iDrive1Status = DISK_STATUS_OFF;
    int iDrive2Status = DISK_STATUS_OFF;
    int iHDDStatus = DISK_STATUS_OFF;

    DiskGetLightStatus(&iDrive1Status, &iDrive2Status);
    iHDDStatus = HD_GetStatus();

    leds[0] = LEDS + iDrive1Status;
    font_print(8, 23, leds, g_hStatusSurface, 4, 2.7);

    leds[0] = LEDS + iDrive2Status;
    font_print(40, 23, leds, g_hStatusSurface, 4, 2.7);

    leds[0] = LEDS + iHDDStatus;
    font_print(71, 23, leds, g_hStatusSurface, 4, 2.7);

    if (iDrive1Status | iDrive2Status | iHDDStatus) {
      g_iStatusCycle = SHOW_CYCLES;
    } // show status panel
  }
}

void FrameShowHelpScreen(int sx, int sy) // sx, sy - sizes of current window (screen)
{
  const int MAX_LINES = 25;
  const char *HelpStrings[MAX_LINES] = {"Welcome to LinApple - Apple][ emulator for Linux!",
                                        "Conf file is applino.conf in current directory by default",
                                        "       F1 - Show help screen",
                                        "  Ctrl+F2 - Cold reboot (Power off and back on)",
                                        " Shift+F2 - Reload configuration file and cold reboot",
                                        " Ctrl+F10 - Hot Reset (Control+Reset)",
                                        "      F12 - Quit",
                                        "",
                                        "    F3/F4 - Load floppy disk 1/2 (Slot 6, Drive 1/2)",
                                        "       F5 - Swap floppy disks",
                                        " Shift+F3/F4 - Attach hard drive 1/2 (Slot 7, Drive 1/2)",
                                        "",
                                        "       F6 - Toggle fullscreen mode",
                                        " Shift+F6 - Toggle character set (keyboard rocker switch)",
                                        "       F7 - Toggle debugging view",
                                        "       F8 - Take screenshot",
                                        " Shift+F8 - Save runtime changes to configuration file",
                                        "       F9 - Cycle through various video modes",
                                        " Shift+F9 - Budget video, for smoother music/audio",
                                        "  F10/F11 - Load/save snapshot file",
                                        "",
                                        "       Pause - Pause/resume emulator",
                                        " Scroll Lock - Toggle full speed",
                                        "  Numpad +/-/* - Increase/Decrease/Normal speed"};

  SDL::Surface *my_screen; // for background
  SDL::Surface *tempSurface = NULL; // temporary surface

  if (font_sfc == NULL) {
    if (!fonts_initialization()) {
      ErrDumpln("Font file was not loaded.");
      return; // If we don't have a font, we just can do none
    }
  }
  if (!g_WindowResized) {
    if (g_nAppMode == MODE_LOGO) {
      tempSurface = g_hLogoBitmap; // Use logobitmap
    } else {
      tempSurface = g_hDeviceBitmap;
    }
  } else {
    tempSurface = g_origscreen;
  }

  if (tempSurface == NULL) {
    tempSurface = screen;
  } // Use screen, if none available
  my_screen = SDL::CreateRGBSurface(SDL::SWSURFACE, tempSurface->w, tempSurface->h, tempSurface->format->BitsPerPixel, 0,
                                   0, 0, 0);
  if (tempSurface->format->palette && my_screen->format->palette) {
    SDL::SetColors(my_screen, tempSurface->format->palette->colors, 0, tempSurface->format->palette->ncolors);
  }

  surface_fader(my_screen, 0.2F, 0.2F, 0.2F, -1, 0);  // fade it out to 20% of normal
  SDL::BlitSurface(tempSurface, NULL, my_screen, NULL);

  SDL::BlitSurface(my_screen, NULL, screen, NULL);    // show background

  double facx = double(g_ScreenWidth) / double(SCREEN_WIDTH);
  double facy = double(g_ScreenHeight) / double(SCREEN_HEIGHT);

  font_print_centered(sx / 2, int(5 * facy), (char *) HelpStrings[0], screen, 1.5 * facx, 1.3 * facy);
  font_print_centered(sx / 2, int(20 * facy), (char *) HelpStrings[1], screen, 1.3 * facx, 1.2 * facy);
  font_print_centered(sx / 2, int(30 * facy), (char *) HelpStrings[2], screen, 1.2 * facx, 1.0 * facy);

  int Help_TopX = int(45 * facy);
  for (int i = 3; i < MAX_LINES; i++) {
    if (HelpStrings[i])
      font_print(4, Help_TopX + (i - 3) * 15 * facy, (char *) HelpStrings[i], screen, 1.5 * facx,
               1.5 * facy); // show keys
  }

  // show frames
  rectangle(screen, 0, Help_TopX - 5, g_ScreenWidth - 1, int(335 * facy), SDL::MapRGB(screen->format, 255, 255, 255));
  rectangle(screen, 1, Help_TopX - 4, g_ScreenWidth, int(335 * facy), SDL::MapRGB(screen->format, 255, 255, 255));
  rectangle(screen, 1, 1, g_ScreenWidth - 2, (Help_TopX - 8), SDL::MapRGB(screen->format, 255, 255, 0));

  tempSurface = SDL::DisplayFormat(assets->icon);
  SDL::Rect logo, scrr;
  logo.x = logo.y = 0;
  logo.w = tempSurface->w;
  logo.h = tempSurface->h;
  scrr.x = int(460 * facx);
  scrr.y = int(270 * facy);
  scrr.w = scrr.h = int(100 * facy);
  SDL::SoftStretchOr(tempSurface, &logo, screen, &scrr);

  SDL::Flip(screen); // Show the screen
  SDL::Delay(1000); // Wait 1 second to be not too fast

  // Wait for keypress
  SDL::Event event;

  event.type = SDL::QUIT;
  while (event.type != SDL::KEYDOWN) { // Wait for ESC-key pressed
    delayMicroseconds(100);
    SDL::PollEvent(&event);
  }

  DrawFrameWindow(); // Restore screen
}

void FrameQuickState(int num, int mod)
{
  // quick load or save state with number num, if Shift is pressed, state is being saved, otherwise - being loaded
  std::string fpath = SPrintf(g_sSaveStateDir, "/SaveState", num, ".aws", num); // prepare file name
  Snapshot_SetFilename(fpath.c_str());  // set it as a working name
  if (mod & KMOD_SHIFT) {
    Snapshot_SaveState();
  } else {
    Snapshot_LoadState();
  }
}

void FrameDispatchMessage(SDL::Event *e) {// process given SDL event
  int mysym = e->key.keycode; // keycode
  int mymod = e->key.keymod; // some special keys flags
  int myscancode = e->key.scancode; // some special keys flags

  switch (e->type) {//type of SDL event
    case SDL::VIDEORESIZE:
      ErrDumpln("OLD DIMENSIONS: ", g_ScreenWidth, "x", g_ScreenHeight);
      g_ScreenWidth = e->resize.w;
      g_ScreenHeight = (e->resize.h / 96) * 96;
      if (g_ScreenHeight < 192) {
        g_ScreenHeight = 192;
      }
      // Resize the screen
      screen = SDL::SetVideoMode(e->resize.w, e->resize.h, SCREEN_BPP, SDL::SWSURFACE | SDL::HWPALETTE | SDL::RESIZABLE);
      if (screen == NULL) {
        SDL::Quit();
        return;
      } else {
        // Define if we have resized window
        g_WindowResized = (g_ScreenWidth != SCREEN_WIDTH) | (g_ScreenHeight != SCREEN_HEIGHT);
        ErrDumpln("Screen size is ", g_ScreenWidth,"x", g_ScreenHeight);
        if (g_WindowResized) {
          // create rects for screen stretching
          origRect.x = origRect.y = newRect.x = newRect.y = 0;
          origRect.w = SCREEN_WIDTH;
          origRect.h = SCREEN_HEIGHT;
          newRect.w = g_ScreenWidth;
          newRect.h = g_ScreenHeight;
          if ((g_nAppMode != MODE_LOGO) && (g_nAppMode != MODE_DEBUG)) {
            VideoRedrawScreen();
          }
        }
      }
      break;

    case SDL::ACTIVEEVENT:
      g_bAppActive = e->active.gain; // if gain==1, app is active
      break;

    case SDL::KEYDOWN:
      if (mysym >= SDLK_0 && mysym <= SDLK_9 && mymod & KMOD_LCTRL) {
        FrameQuickState(mysym - SDLK_0, mymod);
        break;
      }

      if ((mysym >= SDLK_F1) && (mysym <= SDLK_F12) && (buttondown == -1)) {
        SetUsingCursor(0);
        buttondown = mysym - SDLK_F1;  // special function keys processing
      } else if (mysym == SDLK_KP_PLUS) { // Gray + - speed up the emulator!
        g_dwSpeed = g_dwSpeed + 2;
        if (g_dwSpeed > SPEED_MAX) {
          g_dwSpeed = SPEED_MAX;
        } // no Maximum trespassing!
        ErrDumpln("Now speed=", (int) g_dwSpeed);
        SetCurrentCLK6502();
      } else if (mysym == SDLK_KP_MINUS) { // Gray + - speed up the emulator!
        if (g_dwSpeed > SPEED_MIN) {
          g_dwSpeed = g_dwSpeed - 1;
        }// dw is unsigned value!
        ErrDumpln("Now speed=", (int) g_dwSpeed);
        SetCurrentCLK6502();
      } else if (mysym == SDLK_KP_MULTIPLY) { // Gray * - normal speed!
        g_dwSpeed = 10;// dw is unsigned value!
        ErrDumpln("Now speed=", (int) g_dwSpeed);
        SetCurrentCLK6502();
      } else if (mysym == SDLK_CAPSLOCK) {
        KeybToggleCapsLock();
      } else if (mysym == SDLK_PAUSE) {
        SetUsingCursor(0); // release cursor?
        switch (g_nAppMode) {
          case MODE_RUNNING: // go in pause
            g_nAppMode = MODE_PAUSED;
            // SoundCore_SetFade(FADE_OUT); // fade out sound?**************
            break;
          case MODE_PAUSED: // go to the normal mode?
            g_nAppMode = MODE_RUNNING;
            // SoundCore_SetFade(FADE_IN);  // fade in sound?***************
            break;
          case MODE_STEPPING:
            // DebuggerInputConsoleChar(DEBUG_EXIT_KEY);
            break;
          case MODE_LOGO:
          case MODE_DEBUG:
          default:
            break;
        }
        DrawStatusArea(DRAW_TITLE);
        if ((g_nAppMode != MODE_LOGO) && (g_nAppMode != MODE_DEBUG)) {
          VideoRedrawScreen();
        }
        g_bResetTiming = true;
      } else if (mysym == SDLK_SCROLLOCK) {
        g_bScrollLock_FullSpeed = !g_bScrollLock_FullSpeed; // turn on/off full speed?
      } else if ((g_nAppMode == MODE_RUNNING) || (g_nAppMode == MODE_LOGO) || (g_nAppMode == MODE_STEPPING)) {
        // g_bDebuggerEatKey = false;
        // Note about Alt Gr (Right-Alt):
        // . WM_KEYDOWN[Left-Control], then:
        // . WM_KEYDOWN[Right-Alt]
        if (mymod & KMOD_RCTRL)     // GPH: Update trim?
        {
          // JoyUpdateTrimViaKey(mysym);
        } else {
          // Regular joystick movement
          if (/*(!JoyProcessKey(mysym, extended, true, autorep)) && */(g_nAppMode != MODE_LOGO)) {
            KeybQueueKeypress(mysym, NOT_ASCII);
          }
        }
      } else if (g_nAppMode == MODE_DEBUG) {
        if (((mymod & (KMOD_LSHIFT|KMOD_RSHIFT|KMOD_CAPS))>0)&&
            ((mysym>='a')&&(mysym<='z')))
        {
          // convert to upper case when any shift key was pressed
          mysym += 'A'-'a';
        }
        else
        {
          KeybUpdateCtrlShiftStatus();
          mysym = KeybDecodeKey(mysym);
        }
        // DebuggerProcessKey(mysym);
      }
      break;

    case SDL::KEYUP:
      if ((mysym >= SDLK_F1) && (mysym <= SDLK_F12) && (buttondown == mysym - SDLK_F1)) {
        buttondown = -1;
        ProcessButtonClick(mysym - SDLK_F1, mymod); // process function keys - special events
      } else if (mysym == SDLK_CAPSLOCK) {
        // GPH Fix caps lock toggle behavior.
        // (http://sdl.beuc.net/sdl.wiki/SDL_KeyboardEvent)
        KeybToggleCapsLock();
      } else {  // Need to know what "extended" means, and what's so special about SDLK_UP?
        if (myscancode) { // GPH: Checking scan codes tells us if a key was REALLY released.
          // JoyProcessKey(mysym, (mysym >= SDLK_UP && mysym <= SDLK_LEFT), false, 0);
        }
      }
      break;

    case SDL::USEREVENT:
      if (e->user.code == 1) { // should do restart?
        ProcessButtonClick(BTN_RUN, KMOD_LCTRL);
      }
      break;

    default:
    // TODO: Make sure we don't need to do anything else
    break;
  }
}

bool PSP_SaveStateSelectImage(bool saveit)
{
  static size_t fileIndex = 0;    // file index will be remembered for current dir
  static int backdx = 0;  // reserve
  static int dirdx = 0;  // reserve for dirs

  std::string filename;      // given filename
  std::string fullPath;  // full path for it
  bool isDirectory = true;      // if given filename is a directory?

  fileIndex = backdx;
  fullPath = g_sSaveStateDir;  // global var for disk selecting directory

  while (isDirectory) {
    if (!ChooseAnImage(g_ScreenWidth, g_ScreenHeight, fullPath, saveit,
                       filename, isDirectory, fileIndex)) {
      DrawFrameWindow();
      return false;  // if ESC was pressed, just leave
    }
    if (isDirectory) {
      if (filename == "..") {
        const auto last_sep_pos = fullPath.find_last_of(FILE_SEPARATOR);
        if (last_sep_pos == std::string::npos) {
          fullPath = fullPath.substr(0, last_sep_pos);
        }
        if (fullPath == "") {
          fullPath = "/";  //we don't want fullPath to be empty
        }
        fileIndex = dirdx;  // restore
      } else {
        if (fullPath != "/") {
          fullPath += "/" + filename;
        } else {
          fullPath = "/" + filename;
        }
        dirdx = fileIndex; // store it
        fileIndex = 0;  // start with beginning of dir
      }
    }
  }
  strcpy(g_sSaveStateDir, fullPath.c_str());
  RegSaveString(TEXT("Preferences"), REGVALUE_PREF_SAVESTATE_DIR, 1, g_sSaveStateDir); // Save it

  backdx = fileIndex; // Store cursor position

  fullPath += "/" + filename;

  Snapshot_SetFilename(fullPath.c_str()); // Set name for snapshot
  RegSaveString(TEXT("Preferences"), REGVALUE_SAVESTATE_FILENAME, 1, fullPath.c_str()); // Save it
  DrawFrameWindow();
  return true;
}

void FrameSaveBMP(void) {
#if 0
  // I don't care about this capability on Arduino

  // Save current screen as a .bmp file in current directory
  struct stat bufp;
  static int i = 1;  // index
  char bmpName[20];  // file name

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
  snp_rintf(bmpName, 20, "applino%7d.bmp", i);
  while (!stat(bmpName, &bufp)) { // Find first absent file
    i++;
    snp_rintf(bmpName, 20, "applino%7d.bmp", i);
  }
#pragma GCC diagnostic pop

  SDL::SaveBMP(screen, bmpName);  // Save file using SDL inner function
  p_rintf("File %s saved!\n", bmpName);
  i++;
#endif
}

void ProcessButtonClick(int button, int mod)
{
  // button - number of button pressed (starting with 0, which means F1
  // mod - what modifiers been set (like CTRL, ALT etc.)
  SDL::Event qe;  // for Quitting and Reset

  // SoundCore_SetFade(FADE_OUT); // sound/music off?

  switch (button) {
    case BTN_HELP:
      FrameShowHelpScreen(screen->w, screen->h);
      break;

    case BTN_RUN:  // F2 - Run that thing! Or Shift+2 ReloadConfig and run it anyway!
      if ((mod & (KMOD_LCTRL)) == (KMOD_LCTRL) || (mod & (KMOD_RCTRL)) == (KMOD_RCTRL)) {
        if (g_nAppMode == MODE_LOGO) {
          DiskBoot();
        } else if (g_nAppMode == MODE_RUNNING) {
          ResetMachineState();
        }
        if ((g_nAppMode == MODE_DEBUG) || (g_nAppMode == MODE_STEPPING)) {
          // DebugEnd();
        }
        g_nAppMode = MODE_RUNNING;
        DrawStatusArea(DRAW_TITLE);
        VideoRedrawScreen();
        g_bResetTiming = true;
      } else if (mod & KMOD_SHIFT) {
        restart = 1;  // Keep up flag of restarting
        qe.type = SDL::QUIT;
        SDL::PushEvent(&qe); // Push quit event
      }
      break;

    case BTN_DRIVE1:
    case BTN_DRIVE2:
      if (mod & KMOD_CTRL) {
        if (mod & KMOD_SHIFT) {
          printf("HDD  Eject Drive #%d\n", (button - BTN_DRIVE1) + 1);
          HD_Eject(button - BTN_DRIVE1);
        } else {
          printf("Disk Eject Drive #%d\n", (button - BTN_DRIVE1) + 1);
          DiskEject(button - BTN_DRIVE1);
        }
        break;
      }

      if (mod & KMOD_SHIFT) {
          HD_Select(button - BTN_DRIVE1);
      } else {
          DiskSelect(button - BTN_DRIVE1);
      }
      break;

    case BTN_DRIVESWAP:  // F5 - swap disk drives
      DiskDriveSwap();
      break;

    case BTN_FULLSCR:  // F6 - Fullscreen on/off
      if (mod & KMOD_SHIFT) {
         // only IIe and enhanced have a keyboard rocker switch (and only non-US keyboards)
         if ((g_KeyboardLanguage != English_US)&&
             ((g_Apple2Type == A2TYPE_APPLE2E)||(g_Apple2Type == A2TYPE_APPLE2EEHANCED)))
         {
           g_KeyboardRockerSwitch = !g_KeyboardRockerSwitch;
           ErrDumpln("Toggling keyboard rocker switch. Selected character set: ", (g_KeyboardRockerSwitch) ? "local" : "standard/US");
         }
      }
      else
      {
        if (fullscreen) {
          fullscreen = 0;
          SetNormalMode();
        } else {
          fullscreen = 1;
          SetFullScreenMode();
        }
      }
      break;

    case BTN_DEBUG:  // F7 - debug mode - not implemented yet? Please, see README about it. --bb
      if (g_nAppMode != MODE_DEBUG)
      {
        // DebugBegin();
        SetUsingCursor(0);
      }
      else
      if (g_nAppMode == MODE_DEBUG)
      {
        g_nAppMode = MODE_RUNNING;
      }
      break;

    case BTN_SETUP:  // setup is in conf file - applino.conf.
      // may be it should be implemented using SDL??? 0_0 --bb
      // Now Shift-F8 save settings changed run-tme in applino.conf
      // F8 - save current screen as a .bmp file
      // Currently these setting are just next:
      if (mod & KMOD_SHIFT) {
        RegSaveValue(TEXT("Configuration"), TEXT("Video Emulation"), 1, g_videotype);
        RegSaveValue(TEXT("Configuration"), TEXT("Emulation Speed"), 1, g_dwSpeed);
        RegSaveValue(TEXT("Configuration"), TEXT("Fullscreen"), 1, fullscreen);
      } else {
        FrameSaveBMP();
      }
      break;


    // Buttons handlers for F9 - F12
    case BTN_CYCLE: // F9 - CYCLE through allowed video modes
      if (mod & KMOD_SHIFT) {
        // GPH Added budget video for updating only every 12 60Hz frames.
        // This is because, on computers without a fast GPU, the drawing of the screen
        // can actually affect the audio.
        SetBudgetVideo(!GetBudgetVideo());
      } else {
        g_videotype++;  // Cycle through available video modes
        if (g_videotype >= VT_NUM_MODES) {
          g_videotype = 0;
        }
        VideoReinitialize();
        if (g_nAppMode != MODE_LOGO)
        {
          if (g_nAppMode == MODE_DEBUG)
          {
            // unsigned int debugVideoMode;
            if (false) { // DebugGetVideoMode(&debugVideoMode)) {
              VideoRefreshScreen();
            }
          }
          else
          {
            VideoRefreshScreen();
          }
        }
      }
      break;
    case BTN_QUIT:  // F10 - exit from emulator?
      qe.type = SDL::QUIT;
      SDL::PushEvent(&qe); // Push quit event
      break;  //
    case BTN_SAVEST: // Save state (F11)
      if (mod & KMOD_ALT) { // quick save
        Snapshot_SaveState();
      } else if (PSP_SaveStateSelectImage(true)) {
        Snapshot_SaveState();
      }
      break;
    case BTN_LOADST: // Load state (F12) or Hot Reset (Ctrl+F12)
      if (mod & KMOD_CTRL) {
        if (!IS_APPLE2) {
          MemResetPaging();
        }

        DiskReset();
        KeybReset();
        if (!IS_APPLE2) {
          VideoResetState();
        }  // Switch Alternate char set off
        CpuReset();
      } else if (mod & KMOD_ALT) { // quick load state
        Snapshot_LoadState();
      } else if (PSP_SaveStateSelectImage(false)) {
        Snapshot_LoadState();
      }
      break;
  }

  // if ((g_nAppMode != MODE_DEBUG) && (g_nAppMode != MODE_PAUSED)) {
  //   SoundCore_SetFade(FADE_IN);
  // }
}

void ResetMachineState() {
  DiskReset();    // Set floppymotoron=0
  g_bFullSpeed = 0;  // Might've hit reset in middle of InternalCpuExecute() - so beep may get (partially) muted

  MemReset();
  DiskBoot();
  VideoResetState();
  // sg_SSC.CommReset();
  PrintReset();
  SpkrReset();
}

static bool bIamFullScreened;  // for correct fullscreen switching

void SetFullScreenMode() {
  if (!bIamFullScreened) {
    bIamFullScreened = true;
    SDL::WM_ToggleFullScreen(screen);
    if (g_nAppMode != MODE_DEBUG)
      SDL::ShowCursor(SDL::DISABLE);
  }
}

void SetNormalMode()
{
  if (bIamFullScreened) {
    bIamFullScreened = 0;
    SDL::WM_ToggleFullScreen(screen);// we should go back anyway!? ^_^  --bb
    if (!usingcursor) {
      SDL::ShowCursor(SDL::ENABLE);
    } // show mouse cursor if not use it
  }
  else
  if (g_nAppMode == MODE_DEBUG)
  {
    SDL::ShowCursor(SDL::ENABLE);
    SDL::WM_GrabInput(SDL::GRAB_OFF);
  }
}

void SetUsingCursor(bool newvalue) {
  usingcursor = newvalue;
  if (usingcursor) { // Hide mouse cursor and grab input (mouse and keyboard)
    SDL::ShowCursor(SDL::DISABLE);
    SDL::WM_GrabInput(SDL::GRAB_ON);
  } else { // On the contrary - show mouse cursor and ungrab input
    if ((!bIamFullScreened)||(g_nAppMode == MODE_DEBUG)) {
      SDL::ShowCursor(SDL::ENABLE);
    }  // Show cursor if not in fullscreen mode
    SDL::WM_GrabInput(SDL::GRAB_OFF);
  }
}

int FrameCreateWindow()
{
  // Init SDL and create window screen
  bIamFullScreened = false; // At startup not in fullscreen mode
  screen = SDL::SetVideoMode(g_ScreenWidth, g_ScreenHeight, SCREEN_BPP, SDL::SWSURFACE | SDL::HWPALETTE);
  if (screen == NULL) {
    ErrDumpln("Could not set SDL video mode:", SDL::GetError());
    SDL::Quit();
    return 1;
  }

  // define if we have resized window
  g_WindowResized = (g_ScreenWidth != SCREEN_WIDTH) | (g_ScreenHeight != SCREEN_HEIGHT);
  ErrDumpln("Screen size is ", g_ScreenWidth, "x", g_ScreenHeight);
  if (g_WindowResized) {
    // create rects for screen stretching
    origRect.x = origRect.y = newRect.x = newRect.y = 0;
    origRect.w = SCREEN_WIDTH;
    origRect.h = SCREEN_HEIGHT;
    newRect.w = g_ScreenWidth;
    newRect.h = g_ScreenHeight;
  }
  SDL::EnableKeyRepeat(SDL::DEFAULT_REPEAT_DELAY, SDL::DEFAULT_REPEAT_INTERVAL);
  return 0;
}

void SetIcon()
{
  /* Black is the transparency colour.
     Part of the logo seems to use it !? */
  Uint32 colorkey = SDL::MapRGB(assets->icon->format, 0, 0, 0);
  SDL::SetColorKey(assets->icon, SDL::SRCCOLORKEY, colorkey);

  /* No need to pass a mask given the above. */
  SDL::WM_SetIcon(assets->icon, NULL);
}

int InitSDL()
{
  // initialize SDL subsystems, return 0 if all OK, else return 1
  if (SDL::Init(SDL::INIT_EVERYTHING) != 0) {
    ErrDumpln("Could not initialize SDL: ", SDL::GetError());
    return 1;
  }

  // SDL ref: Icon should be set *before* the first call to SDL::SetVideoMode.
  SetIcon();
  return 0;
}

void FrameRefreshStatus(int drawflags)
{
  DrawStatusArea(drawflags);
}
