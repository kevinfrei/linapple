// This file contains shim functions to get the thing compiling
// & running on arduino

#include "arduinoshim.h"
#include <sstream>
#include <string>
#include <cstdint>

namespace ard {
std::string
to_string(uintmax_t sz) {
    std::string res;
    do {
        res.push_back('0' + sz % 10);
        sz /= 10;
    } while (sz != 0);
    return std::string(res.rbegin(), res.rend());
}
}  // namespace ard
namespace SDL {

bool
PollEvent(Event *evt) {
    // TODO: Check the keyboard here?
    return false;
}

void
PushEvent(Event *evt) {
    // Stuff this in an event queue
    return;
}

void
WM_SetCaption(const char *title, const char *appTitle) {
    // I can set this on the display at the bottom/top
}

void
WM_ToggleFullScreen(Surface *) {
    // I don't think this doesn't anything
}

void
WM_GrabInput(uint32_t) {
    // Do nothing
}

// Not sure about the second arg type just yet...
void
WM_SetIcon(Surface *, char *) {
    // Do nothing
}

void
ShowCursor(uint32_t) {
    // Mouse? I think so, so do nothing
}

void
Quit() {
    // Go back to the normal Keyboard mode...
}

uint32_t
MapRGB(Surface::Format *fmt, uint8_t r, uint8_t g, uint8_t b) {
    return 0;
}

void
SetColors(Surface *, Color *c, uint32_t lo, uint32_t hi) {
    return;
}

void
FillRect(Surface *s, Rect *r, uint32_t color) {
}

// TODO: Not sure about the other 4 pixels
Surface *
CreateRGBSurface(SurfaceType type, uint32_t w, uint32_t h, uint8_t bpp, uint32_t n0, uint32_t n1, uint32_t n2,
                 uint32_t n3) {
    return nullptr;
}

void
BlitSurface(Surface *src, void *, Surface *dst, void *) {
}

Surface *
DisplayFormat(Surface *sfc) {
    return nullptr;
}

void
Flip(Surface *) {
}

Surface *
SetVideoMode(uint32_t w, uint32_t h, uint8_t bpp, SurfaceType st) {
    return nullptr;
}

char *
getenv(const char *) {
    return nullptr;
}

const char *
GetError() {
    return nullptr;
}

void
SetError(const char *, ...) {
}

void
EnableKeyRepeat(uint32_t delay, uint32_t interval) {
}

void
SetColorKey(Surface *, uint32_t srce, uint32_t mapRGBresult) {
}

uint32_t
Init(uint32_t what) {
    return 0;
}

uint8_t *
GetKeyState(char *) {
    return nullptr;
}

void
FreeSurface(Surface *) {
}

bool
MUSTLOCK(Surface *) {
    return false;
}

int
LockSurface(Surface *) {
    return 0;
}

void
UnlockSurface(Surface *) {
    return;
}

int
SoftStretch(Surface *src, Rect *srcrect, Surface *dst, Rect *dstrect) {
    return 0;
}

void
UpdateRect(Surface *scrn, int x, int y, int w, int h) {
}

void
free(void *ptr) {
    ::free(ptr);
}

void *
calloc(size_t num, size_t sz) {
    return ::calloc(num, sz);
}

void *
malloc(size_t sz) {
    return ::malloc(sz);
}

void *
realloc(void *ptr, size_t sz) {
    return ::realloc(ptr, sz);
}

void
OutOfMemory() {
}

Surface *
LoadBMP(const char *path) {
    return nullptr;
}

char *
strdup(const char *str) {
    std::size_t len = strlen(str) + 1;
    char *res = (char *)malloc(len);
    if (res) {
        memcpy(res, str, len);
    }
    return res;
}

char *
strrchr(const char *str, char chr) {
    return ::strrchr(str, chr);
}

size_t
strlen(const char *str) {
    return ::strlen(str);
}

void
GetClipRect(Surface *, Rect *) {
}
}  // namespace SDL

SDL::Surface *
IMG_ReadXPMFromArray(const char **) {
    return nullptr;
}

void *
MyFOpen(const char *fn, const char *perms) {
    return nullptr;
}
