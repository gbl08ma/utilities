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

#include "settingsProvider.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "constantsProvider.hpp"
#include "timeGUI.hpp"
#include "menuGUI.hpp"
#include "homeGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"
#include "firstRunGUI.hpp"
#include "setjmp.h"
#include "debugGUI.hpp"
jmp_buf utilities_return;
int main()
{
  // Add-in entry point
  setInitStackPtr((int)GetStackPtr()); // set pointer for monitoring stack usage
  
  if(setjmp(utilities_return)) {
    // we are returning from RunMat after the calc was locked.
    // turn off blinking cursor and clear strange keys remapping
    Cursor_SetFlashOff();
    Bkey_ClrAllFlags();
    // according to Simon, the following two syscalls shouldn't be called unless an add-in is "run from RAM".
    // he says in the INSIGHT source code that the built-in apps remap the value of fkeys. This appears to be true.
    // INSIGHT only remaps when running from RAM. Apparently the OS clears the key mapping when starting a new add-in, which makes sense
    // since the OS isn't here to help us, we must clear manually.
    // still, his comment about there being a "host" and an add-in being run from RAM doesn't seem to apply here.
    const unsigned int default_fkeys[] = { 0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0 };
    Set_FKeys1( 0, (unsigned int*)default_fkeys );
    Set_FKeys2( 0 );
  }
  //Load settings
  LoadSettings();
  
  // Adjust UI and HW values according to settings
  // Set brightness if setting is set
  setBrightnessToStartupSetting();
  Bdisp_EnableColor(1);
  
  if(GetSetting(SETTING_IS_FIRST_RUN)) {
    firstRunWizard();
    RTCunadjustedWizard(0);
  } else {
    RTCunadjustedWizard(1);
  }
  // load chronos
  chronometer* chrono;
  chrono = (chronometer*)alloca(NUMBER_OF_CHRONO*sizeof(chronometer));
  loadChronoArray(chrono, NUMBER_OF_CHRONO);
  
  setChronoArrayPtr(chrono); //needed by the checkChronoComplete function
  
  // NOTE: the following debug timer is disabled because it makes many things become much slower, and causes system errors on Bfile. only enable if needed.
  //int ramUsedDbgTimer = Timer_Install(0, showRAMusedStatus, 50);
  //if (ramUsedDbgTimer > 0) { Timer_Start(ramUsedDbgTimer); }
  
  while(1) {
    showHome(chrono);
  }
}

