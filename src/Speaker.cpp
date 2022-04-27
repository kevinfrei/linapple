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

/* Description: Speaker emulation
 *
 * Author: Various
 */

/* Remake for SDL Audio for Linux (or other SDL-compliant OSes) by beom beotiger --bb */

#include "stdafx.h"
// for _ASSERT ion. (here _ASSERT means Unix(tm) assert) --bb
#include <assert.h>

// Notes:
//
// [OLD: 23.191 Apple CLKs == 44100Hz (CLOCK_6502/44100)]
// 23 Apple CLKS per PC sample (played back at 44.1KHz)
//
//
// The speaker's wave output drives how much 6502 emulation is done in real-time, eg:
// If the speaker's wave buffer is running out of sample-data, then more 6502 cycles
// need to be executed to top-up the wave buffer.
// This is in contrast to the AY8910 voices, which can simply generate more data if
// their buffers are running low.


// number of channels and buffer size for Apple][ Speakers
static const unsigned short g_nSPKR_NumChannels = 1;

// Globals (SOUND_WAVE)
// GPH This "INIT" value is the actual sample data written to the speaker when
// it gets whapped with a BIT $C030.  Note the output is always digital.
// The Apple II had no notion of volume or of looping waves, only the single
// "click" of the speaker. We set this to a value well under the maximum to
// provide headroom for mixing Mockingboard sound without causing peak clipping.
const short SPKR_DATA_INIT = (short) 0x2000;  // data written to speakers buffer

// Application-wide globals:
unsigned int soundtype = SOUND_WAVE; //default
double g_fClksPerSpkrSample;    // Setup in SetClksPerSpkrSample()

// Globals
static UINT64 g_nSpkrQuietCycleCount = 0;
static bool g_bSpkrToggleFlag = false;

static bool g_bSpkrAvailable = false;
static bool g_bSpkrRecentlyActive = false;

// Forward refs:
// static ULONG Spkr_SubmitWaveBuffer(short *pSpeakerBuffer, ULONG nNumSamples);

static void Spkr_SetActive(bool bActive);
void Spkr_FlipSpeaker();

// Let us leave benchmark for the near future --bb ^_^
#if 0
static void DisplayBenchmarkResults () {

  unsigned int totaltime = GetTickCount() - extbench;
  VideoRedrawScreen();
  char buffer[64];
  sp_rintf(buffer,
           TEXT("This benchmark took %u.%02u seconds."),
           (unsigned)(totaltime / 1000),
           (unsigned)((totaltime / 10) % 100));
  p_rintf("This benchmark took %u.%02u seconds.",
     (unsigned)(totaltime / 1000), (unsigned)((totaltime / 10) % 100));

}
#endif
/*
static void SetClksPerSpkrSample() {
  // 23.191 clks for 44.1Khz (when 6502 CLK=1.0Mhz)
  //  g_fClksPerSpkrSample = g_fCurrentCLK6502 / (double)SPKR_SAMPLE_RATE;

  // Use integer value: Better for MJ Mahon's RT.SYNTH.DSK (integer multiples of 1.023MHz Clk)
  // . 23 clks @ 1.023MHz    SPKR_SAMPLE_RATE = 44100Hz!?
  g_fClksPerSpkrSample = (double) (unsigned int)(g_fCurrentCLK6502 / (double) SPKR_SAMPLE_RATE);
}
*/
//=============================================================================

void SpkrInitialize() {
  g_bSpkrAvailable = Spkr_DSInit();
}

// NB. Called when /g_fCurrentCLK6502/ changes
void SpkrReinitialize() {
}

void SpkrReset() {
  Spkr_SetActive(false);
  Spkr_Demute();
}

#if 0
bool SpkrSetEmulationType (unsigned int newtype) {
  if (soundtype != SOUND_NONE)
    SpkrDestroy();
  soundtype = newtype;
  if (soundtype != SOUND_NONE)
    SpkrInitialize();
  if (soundtype != newtype)
    switch (newtype) {  // some fault occured
      case SOUND_WAVE:
        fp_rintf(stderr, "Unable to initialize a waveform output device.\n");
        return 0;
    }
  return 1;
}
#endif

// Called by emulation code when Speaker I/O reg (0xC030) is accessed
unsigned char SpkrToggle(unsigned short, unsigned short, unsigned char, unsigned char, ULONG nCyclesLeft) {
  g_bSpkrToggleFlag = true;

  if (!g_bFullSpeed) {
    Spkr_SetActive(true);
  }
  Spkr_FlipSpeaker();
  needsprecision = cumulativecycles;  // ?

  return MemReadFloatingBus(nCyclesLeft); // reading from $C030..$C03F retrurns unpredictable value?
}

void SpkrUpdate(unsigned int totalcycles) {
  if (!g_bSpkrToggleFlag) {
    if (!g_nSpkrQuietCycleCount) {
      g_nSpkrQuietCycleCount = g_nCumulativeCycles;
    } else if (g_nCumulativeCycles - g_nSpkrQuietCycleCount > (UINT64)g_fCurrentCLK6502 / 5)
    {
      // After 0.2 sec of Apple time, deactivate spkr voice
      // . This allows emulator to auto-switch to full-speed g_nAppMode for fast disk access
      Spkr_SetActive(false);
    }
  } else {
    g_nSpkrQuietCycleCount = 0;
    g_bSpkrToggleFlag = false;
  }
}


static void Spkr_SetActive(bool bActive) {
  // yes, I know the right way is:   g_bSpkrRecentlyActive = bActive;, but... ^_^ --bb
  if (bActive) {
    // Called by SpkrToggle() or SpkrReset()
    g_bSpkrRecentlyActive = true;
  } else {
    // Called by SpkrUpdate() after 0.2s of speaker inactivity
    g_bSpkrRecentlyActive = false;
  }
}

bool Spkr_IsActive() {
  return g_bSpkrRecentlyActive;
}

// How to deal with volume in SDL Audio may be need to go to SDL Mixer?
unsigned int SpkrGetVolume() {
  return 0;
}

void SpkrSetVolume(unsigned int dwVolume, unsigned int dwVolumeMax) {
}

unsigned int SpkrGetSnapshot(SS_IO_Speaker *pSS) {
  // pSS->g_nSpkrLastCycle = g_nSpkrLastCycle;
  return 0;
}

unsigned int SpkrSetSnapshot(SS_IO_Speaker *pSS) {
  // g_nSpkrLastCycle = pSS->g_nSpkrLastCycle;
  return 0;
}

// TODO: Get the right pin
constexpr uint8_t SPEAKER_PIN=2;

static bool low = true;
static bool muted = false;

bool Spkr_DSInit(){
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
  muted = false;
  return true;
}

void Spkr_DSUninit() {
  digitalWrite(SPEAKER_PIN, LOW);
}

void SpkrDestroy() {}

void Spkr_FlipSpeaker(){
  if (!muted) {
    digitalWrite(SPEAKER_PIN, low ? HIGH : LOW);
  }
  low = !low;
}

// Mute - set volume to MINIMUM,  Demute - set volume to NORMAL STATE? -bb
void Spkr_Mute() {
  muted = true;
}

void Spkr_Demute() {
  muted = false;
}
