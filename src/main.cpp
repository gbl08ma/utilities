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
#include "versionProvider.hpp"
#include "timeGUI.hpp"
#include "menuGUI.hpp"
#include "homeGUI.hpp"
#include "chronoProvider.hpp"
#include "firstRunGUI.hpp"
#include "aboutGUI.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "setjmp.h"
#include "debugGUI.hpp"
jmp_buf utilities_return;
int main()
{
  // Add-in entry point
  
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
    Bdisp_EnableColor(1);
    // workaround to enable color: we must use GetKey once, otherwise color might not be enabled
    int key;
    Keyboard_PutKeycode( -1, -1, KEY_CTRL_EXIT);
    GetKey(&key);
  }
  // disable Catalog function throughout the add-in, as we don't know how to make use of it:
  Bkey_SetAllFlags(0x80);
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
    if(getBuildIsExpired()) buildExpiredMessage();
  }
  // load chronos
  chronometer* chrono;
  chrono = (chronometer*)alloca(NUMBER_OF_CHRONO*sizeof(chronometer));
  loadChronoArray(chrono, NUMBER_OF_CHRONO);
  
  setChronoArrayPtr(chrono); //needed by the checkChronoComplete function
  
  // NOTE: the following debug timer is disabled because it makes many things become much slower, and causes system errors on Bfile. only enable if needed.
  //int ramUsedDbgTimer = Timer_Install(0, showRAMusedStatus, 50);
  //if (ramUsedDbgTimer > 0) { Timer_Start(ramUsedDbgTimer); }

#ifdef LOG_BATTERY_VOLTAGE
  // log battery voltage on every start-up
  char smemfile[50];
  strcpy(smemfile, "\\\\fls0\\battlog.txt");
  unsigned short pFile[sizeof(smemfile)*2]; // Make buffer
  Bfile_StrToName_ncpy(pFile, (unsigned char*)smemfile, strlen(smemfile)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  int size = 1, justcreated=0;
  if(hFile < 0) // Check if it opened
  {
    justcreated=1;
    //error, file doesn't exist. create it
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres < 0) // Did it create?
    {
      return 2; //error: file doesn't exist and yet can't be created.
    }
    hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
    // Check if it opened now that we created it:
    if(hFile < 0) return 3;
  }
  //file exists (even if it didn't exist before) and is open. write new contents
  char newentry[50];
  itoa(currentUnixTime()/1000, (unsigned char*)newentry);
  strcat(newentry, (char*)",");
  unsigned char voltbuffer[20] = "";
  itoa(GetMainBatteryVoltage(1), voltbuffer);
  strcat(newentry, (char*)voltbuffer);
  strcat(newentry, (char*)"\r\n");

  int oldsize = Bfile_GetFileSize_OS(hFile);
  if(!justcreated) Bfile_SeekFile_OS(hFile, oldsize); //move cursor to end
  Bfile_WriteFile_OS(hFile, newentry, strlen(newentry));
  Bfile_CloseFile_OS(hFile);
#endif
  
  while(1) {
    showHome(chrono);
  }
}

