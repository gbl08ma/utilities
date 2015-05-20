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
#include "constantsProvider.hpp"

static unsigned char settings[NUMBER_OF_SETTINGS+1];

// Routines for accessing and setting settings
// NOTE: directly accessing the settings array is strictly forbidden!

unsigned char getSetting(const int setting) {
  return settings[setting];
}

void setSetting(const int setting, const unsigned char value, const int autosave) {
  settings[setting] = value;
  if(setting == SETTING_DISPLAY_STATUSBAR) {
    if(settings[setting]) {
      EnableStatusArea(0);
      DefineStatusAreaFlags(3, SAF_BATTERY | SAF_TEXT | SAF_GLYPH | SAF_ALPHA_SHIFT, 0, 0);
    } else {
      EnableStatusArea(3);
    }
  }
  if (autosave) saveSettings();
}

// Routines for saving and loading settings
int loadSettings() { // returns 0 on success, 1 if settings were reset (first run / incompatibility)
  int size;
  MCSGetDlen2(DIRNAME, SETTINGSFILE, &size);
  if (size <= 16) goto resetSettings; // no settings file or incompatible, reset settings.

  MCSGetData1(0, NUMBER_OF_SETTINGS+1, settings); // +1 because NUMBER_OF_SETTINGS does not include the file version
  // detect if the current settings file version is compatible with the one we're expecting.
  // file version is always the first byte, except for the settings files of versions 1.0 and earlier.
  // for these, the filesize happened to be equal or smaller than 16 (checked above).
  // if we got here, the file is a "modern" one, so check the version byte.
  if (settings[0] != SETTINGSFILE_VERSION) goto resetSettings; //if incompatible, reset settings
  return 0;

  resetSettings:
  for(int i = 1; i <= NUMBER_OF_SETTINGS; i++) settings[i] = 0;
  // the defaults for some settings are not zero:
  settings[SETTING_DISPLAY_STATUSBAR] = 1;
  settings[SETTING_STARTUP_BRIGHTNESS] = 250;
  settings[SETTING_DISPLAY_FKEYS] = 1;
  settings[SETTING_PASSWORD_PRIVACY] = 1;
  settings[SETTING_SHOW_CALENDAR_EVENTS_COUNT] = 1;
  settings[SETTING_ENABLE_LOCK] = 1;
  settings[SETTING_CLOCK_SECONDS] = 1;
  settings[SETTING_DEFAULT_CALENDAR_VIEW] = 1;
  settings[SETTING_SHOW_CALENDAR_BUSY_MAP] = 1;
  settings[SETTING_CHRONO_NOTIFICATION_TYPE] = 1;
  settings[SETTING_FILE_MANAGER_SORT] = 1;
  settings[SETTING_FILE_MANAGER_SEARCH] = 1;
  settings[SETTING_TIMEZONE] = 51;
  return 1;
}

void saveSettings() {
  if(MCS_CreateDirectory(DIRNAME)) // Check if directory exists
  { // directory already exists, so delete the exiting file that may be there
    MCSDelVar2(DIRNAME, SETTINGSFILE);
  }
  settings[SETTING_SELF_FILEVERSION] = SETTINGSFILE_VERSION;
  MCSPutVar2(DIRNAME, SETTINGSFILE, NUMBER_OF_SETTINGS+1, settings); //+1 on purpose again, for the same reason
}
