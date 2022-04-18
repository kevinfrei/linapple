#include <math.h>
#include <stdlib.h>
#include <string.h>
// #include <time.h>
#include <inttypes.h>
#include <stdint.h>

#ifndef _WIN32
#  include "wincompat.h"
#else
#  include <windows.h>
#endif

// #include <SDL.h>
#include "Arduino.h"
#include "arduinoshim.h"

#include "Common.h"
#include "Structs.h"

#include "AppleWin.h"

#include "CPU.h"

#include "Disk.h"
#include "DiskChoose.h"
#include "DiskImage.h"

#include "Frame.h"
#include "Harddisk.h"
#include "Clock.h"
#include "Keyboard.h"
#include "Log.h"
#include "Memory.h"
#include "ParallelPrinter.h"
#include "Registry.h"

#include "Riff.h"
#include "SaveState.h"
#include "Speaker.h"

#include "stretch.h"

#include "Timer.h"

#include "Video.h"

// This is to faciliate porting to SDL2
#include "shim.h"
