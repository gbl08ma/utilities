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
#include "debugGUI.hpp"

int main()
{
  // Add-in entry point
  setInitStackPtr((int)GetStackPtr()); // set pointer for monitoring stack usage
  //Load settings
  LoadSettings();
  
  // Adjust UI and HW values according to settings
  // Set brightness if setting is set
  if (GetSetting(SETTING_STARTUP_BRIGHTNESS) != 250 && 0 <= GetSetting(SETTING_STARTUP_BRIGHTNESS) && GetSetting(SETTING_STARTUP_BRIGHTNESS) <= 249) {
    SetBacklightSubLevel_RAW(GetSetting(SETTING_STARTUP_BRIGHTNESS));
  }
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

