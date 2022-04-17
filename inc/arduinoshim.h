// This file contains shim functions to get the thing compiling
// & running on arduino

#ifndef __arduinoshim_h
#define __arduinoshim_h

#include "Arduino.h"

#include <string>
#include <cstdint>

namespace ard {
std::string
to_string(uintmax_t sz);
}  // namespace ard

namespace SDL {
enum SDL_EVENTS { QUIT, KEYDOWN, KEYUP, USEREVENT, VIDEORESIZE, ACTIVEEVENT };
typedef uint32_t SurfaceType;
constexpr SurfaceType SWSURFACE = 1;
constexpr SurfaceType HWPALETTE = 2;
constexpr SurfaceType RESIZABLE = 4;
constexpr uint32_t DISABLE = 0x1234dead;
constexpr uint32_t ENABLE = 0x4321beef;
constexpr uint32_t GRAB_ON = 0x18476284;
constexpr uint32_t GRAB_OFF = 0x18567295;

struct Surface {
    uint32_t h, w;
    struct Format {
        uint8_t BitsPerPixel;
        struct Palette {
            uint32_t ncolors;
            struct Colors {};
            Colors *colors;
        };
        Palette *palette;
    };
    Format *format;
};

struct Event {
    SDL_EVENTS type;
    struct Resize {
        uint32_t h, w;
    };
    Resize resize;
    struct Key {
        int keycode;
        int keymod;
        int scancode;
    };
    Key key;
    struct Active {
        uint8_t gain;
    };
    Active active;
    struct User {
      uint32_t code;
    };
    User user;
};
struct Rect {
    uint32_t x, y, w, h;
};
bool
PollEvent(Event *evt);
void
PushEvent(Event *evt);
inline void
Delay(uint32_t ms) {
    delay(ms);
}
void
WM_SetCaption(const char *title, const char *appTitle);
void
WM_ToggleFullScreen(Surface *);
void WM_GrabInput(uint32_t);
// Not sure about the second arg type just yet...
void WM_SetIcon(Surface *, char *);
void ShowCursor(uint32_t);
void
Quit();
uint32_t
MapRGB(Surface::Format *fmt, uint8_t r, uint8_t g, uint8_t b);
void
SetColors(Surface *, Surface::Format::Palette::Colors *c, uint32_t lo, uint32_t hi);
void
FillRect(Surface *s, Rect *r, uint32_t color);
// TODO: Not sure about the other 4 pixels
Surface *
CreateRGBSurface(SurfaceType type, uint32_t w, uint32_t h, uint8_t bpp, uint32_t n0, uint32_t n1, uint32_t n2,
                 uint32_t n3);
void
BlitSurface(Surface *src, void *, Surface *dst, void *);
Surface *
DisplayFormat(Surface *sfc);
void
Flip(Surface *);
Surface *
SetVideoMode(uint32_t w, uint32_t h, uint8_t bpp, SurfaceType st);

void SaveBMP(Surface *, const char *name);
void putenv(const char *);
const char *GetError();
constexpr uint32_t DEFAULT_REPEAT_DELAY=250;
constexpr uint32_t DEFAULT_REPEAT_INTERVAL=75;
void EnableKeyRepeat(uint32_t delay, uint32_t interval);

constexpr uint32_t SRCCOLORKEY = 0x184abcd;
void SetColorKey(Surface *, uint32_t srce, uint32_t mapRGBresult);
constexpr uint32_t INIT_EVERYTHING = 0x4284bead;
uint32_t Init(uint32_t what);
uint8_t *GetKeyState(char *);
}  // namespace SDL

using Uint8 = uint8_t;
using Uint32 = uint32_t;

constexpr uint16_t KMOD_LCTRL = 0x80;
constexpr uint16_t KMOD_RCTRL = 0x08;
constexpr uint16_t KMOD_CTRL = 0x88;
constexpr uint16_t KMOD_LALT = 0x40;
constexpr uint16_t KMOD_RALT = 0x04;
constexpr uint16_t KMOD_ALT = 0x44;
constexpr uint16_t KMOD_LSHIFT = 0x10;
constexpr uint16_t KMOD_RSHIFT = 0x01;
constexpr uint16_t KMOD_CAPS = 0x100;

constexpr uint16_t SDLK_LSHIFT = 0x60;
constexpr uint16_t SDLK_RSHIFT = 0x61;
constexpr uint16_t SDLK_LCTRL = 0x62;
constexpr uint16_t SDLK_RCTRL = 0x63;
constexpr uint16_t SDLK_LALT = 0x64;
constexpr uint16_t SDLK_RALT = 0x65;
constexpr uint16_t SDLK_RETURN = 0x66;
constexpr uint16_t SDLK_LEFT = 0x67;
constexpr uint16_t SDLK_UP = 0x68;
constexpr uint16_t SDLK_RIGHT = 0x69;
constexpr uint16_t SDLK_DOWN = 0x6a;
constexpr uint16_t SDLK_DELETE = 0x6b;
constexpr uint16_t SDLK_F1 = 0xF1;
constexpr uint16_t SDLK_F4 = 0xF4;
constexpr uint16_t SDLK_F12 = 0xFC;
constexpr uint16_t SDLK_0 = 0x30;
constexpr uint16_t SDLK_9 = 0x39;
constexpr uint16_t SDLK_KP_PLUS = 0x81;
constexpr uint16_t SDLK_KP_MINUS = 0x82;
constexpr uint16_t SDLK_KP_MULTIPLY = 0x83;
constexpr uint16_t SDLK_CAPSLOCK = 0x90;
constexpr uint16_t SDLK_SCROLLOCK = 0x91;
constexpr uint16_t SDLK_PAUSE = 0x92;

#define DECLSPEC
#define SDLCALL
//
constexpr uint32_t KMOD_SHIFT = 0x80;
#endif
