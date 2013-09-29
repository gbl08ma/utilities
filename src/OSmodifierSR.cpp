#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
#include <fxcg/rtc.h>
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "OSmodifierSR.hpp" // OS modifier code - stay resident after add-in is closed
#include "settingsProvider.hpp"

char buffer1[20] __attribute__((section(".rsmem.data")));
//int OSmod_SBcolor1 __attribute__((section(".rsmem.data"))) = 7; // first statusbar color
//int OSmod_SBcolor2 __attribute__((section(".rsmem.data"))) = 7; // second statusbar color
// we can't put the SB color settings on the area of RSMEM that's overwritten when this add-in launches
// the setting would be lost every time an add-in with support for RSMEM was launched
// so, put them right before.
#define OSMOD_SBCOLOR1 (unsigned char*)0xFD803198
#define OSMOD_SBCOLOR2 (unsigned char*)0xFD803199

void strcpy_rs(char dest[], const char source[]) { // in RS memory
   int i = 0;
   while (1) {
      dest[i] = source[i];
      if (dest[i] == '\0') break;
      i++;
} }

void OSmodifierTimerHandler() { // in RS memory
  // Handle statusbar colors mod. I'm not yet sure this isn't changeable through the setup.
  // anyway, we can't use the setup here as we can't use syscalls
  unsigned char*sbcolor1 = (unsigned char*) 0x8804F48C;
  //*sbcolor1 = OSmod_SBcolor1;
  *sbcolor1 = *OSMOD_SBCOLOR1;
  unsigned char*sbcolor2 = (unsigned char*) 0x8804F48D;
  //*sbcolor2 = OSmod_SBcolor2;
  *sbcolor2 = *OSMOD_SBCOLOR2;
  
  unsigned char*smsg = (unsigned char*) 0x8804F48E;
  strcpy_rs((char*)smsg, buffer1);
}

void setOSmodifier() {
  // setups and starts OS modifier timer, or uninstalls and stops if the Prizmed Framework is disabled
  if(!GetSetting(SETTING_ENABLE_PRIZMED)) {
    // Prizmed framework is not enabled or was just disabled.
    // Stop and uninstall timer 3 (let's hope it's ours) and return
    Timer_Stop(3);
    Timer_Deinstall(3);
    return;
  }
  // install timer on slot 3 - this appears to be unused by the system, but it's a system timer
  // so it doesn't get uninstalled when leaving the add-in.
  int osmodt = Timer_Install(3, OSmodifierTimerHandler, 1000);
  if (osmodt > 0) { Timer_Start(osmodt); }
  else {
    // timer install failed. This may be because our timer is already running (or the OS decided to use it).
    // but we must reinstall the timer: imagine that due to a change in the code,
    // the timer handler now lives at a different position in RS memory...
    // so, stop the timer and install it again.
    Timer_Stop(3);
    Timer_Deinstall(3);
    osmodt = Timer_Install(3, OSmodifierTimerHandler, 1000);
    if (osmodt > 0) { Timer_Start(osmodt); } // if it still fails now, there's nothing we can do
  }
}

void setOSmodStatusBar(int color1, int color2) {
  *OSMOD_SBCOLOR1 = color1;
  *OSMOD_SBCOLOR2 = color2;
}

void getOSmodStatusBar(int* color1, int* color2) {
  *color1 = *OSMOD_SBCOLOR1;
  *color2 = *OSMOD_SBCOLOR2;
}