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

#define SETTINGSFILE_VERSION 5 // NOTE: update this when changing the amount or meaning of settings!
static int setting_self_fileversion = SETTINGSFILE_VERSION; // this is a special setting
static int setting_timeformat = 0; // 0 = 24-hour HH:MM:SS ; 1 = 12-hour HH:MM:SS AM/PM
static int setting_longdateformat = 0;
/* 0 = "Weekday, Month 12"&mini 4-digits year below
   1 = "Weekday, 12th Month"&mini 4-digits year below
   2 = "Weekday, 12th Month 2121" (year not mini)
   3 = "Weekday, 12th Month" (no year)
   4 = "Month 12"&mini 4-digits year below
   5 = "Month 12, 2012" (year not mini)
   6 = "Month 12" (no year)
   7 = "12th Month"&mini 4-digits year below
   8 = "12th Month 2012" (year not mini)
   9 = "12th Month" (no year)
*/
static int setting_dateformat = 0;
/* 0 = DD/MM/YYYY
   1 = MM/DD/YYYY
   2 = YYYY/MM/DD
*/
static int setting_theme = 0; //whether the home screen (and later, possibly other things too) should use a dark/inverted theme. 0: white theme; 1: dark theme
static int setting_display_statusbar = 1; //whether the status bar should be displayed
static int setting_startup_brightness = 250; //screen brightness level to force on add-in startup, 0~249. 250 for no forcing //NOTE: was unsigned int before... why?
static int setting_show_advanced = 0; //whether to show advanced things like CPU clock selection
static int setting_display_fkeys = 1;
static int setting_password_privacy = 1; // 0: show last input character when entering passwords; 1: do not show last char NOTE: this had a different name before and the meaning of the value is now different!
static int setting_show_calendar_events_count = 1; // had a different name before, value meaning is the same
static int setting_is_first_run = 1; //should be 1 for the first time the add-in is run, then it's set to zero and never changed
static int setting_enable_lock = 1; //whether lock functionality is available or not. Should always be on and not changeable on the settings menu, except when a specific code is entered on master-control in order to disable it to make people who don't want the lock, because they accidentally lock their calculators, happier (I'm thinking of you Catarina...)
static int setting_lock_autooff = 0; //whether to turn off automatically after locking the calc
static int setting_lock_on_exe = 0; //when enabled, calculator is locked when EXE is pressed on the home screen (i.e. legacy support for people used to the old lock add-in).
static int setting_unlock_runmat = 0; //whether to jump to Run-Mat when calculator is unlocked. 2 when user should be asked.
static int setting_clock_type = 0;
/* what clock to show on home screen.
   0 = digital time and long date - "traditional", like versions up to 1.0
   1 = just digital time
   2 = just digital time and short date
   3 = just long date
   4 = just short date
   5 = analog clock, nothing else
   6 = analog clock with digital time
   7 = analog clock with d. time and long date
   8 = analog clock with d. time and short date
   9 = analog clock with long date
   10 = analog clock with short date
   11 = show nothing at all
*/
static int setting_clock_seconds = 1; // whether to show seconds in clock
static int setting_home_panes = 1; // whether to enable panes in home screen
static int setting_default_calendar_view = 1; // default calendar view. 0 = week, 1 = month

// Routines for accessing and setting settings
// NOTE: directly accessing setting_* variables is now strictly forbidden!

int GetSetting(int setting) {
  switch(setting) {
    case SETTING_SELF_FILEVERSION:
      return setting_self_fileversion;
    case SETTING_TIMEFORMAT:
      return setting_timeformat;
    case SETTING_LONGDATEFORMAT:
      return setting_longdateformat;
    case SETTING_DATEFORMAT:
      return setting_dateformat;
    case SETTING_THEME:
      return setting_theme;
    case SETTING_DISPLAY_STATUSBAR:
      return setting_display_statusbar;
    case SETTING_STARTUP_BRIGHTNESS:
      return setting_startup_brightness;
    case SETTING_SHOW_ADVANCED:
      return setting_show_advanced;
    case SETTING_DISPLAY_FKEYS:
      return setting_display_fkeys;
    case SETTING_PASSWORD_PRIVACY:
      return setting_password_privacy;
    case SETTING_SHOW_CALENDAR_EVENTS_COUNT:
      return setting_show_calendar_events_count;
    case SETTING_IS_FIRST_RUN:
      return setting_is_first_run;
    case SETTING_ENABLE_LOCK:
      return setting_enable_lock;
    case SETTING_LOCK_AUTOOFF:
      return setting_lock_autooff;
    case SETTING_LOCK_ON_EXE:
      return setting_lock_on_exe;
    case SETTING_UNLOCK_RUNMAT:
      return setting_unlock_runmat;
    case SETTING_CLOCK_TYPE:
      return setting_clock_type;
    case SETTING_CLOCK_SECONDS:
      return setting_clock_seconds;
    case SETTING_HOME_PANES:
      return setting_home_panes;
    case SETTING_DEFAULT_CALENDAR_VIEW:
      return setting_default_calendar_view;
    default:
      return 0;
  }
}

void SetSetting(int setting, int value, int autosave) {
  switch(setting) {
    case SETTING_SELF_FILEVERSION:
      // do nothing. this "setting" is not meant to be changed at runtime.
      break;
    case SETTING_TIMEFORMAT:
      setting_timeformat = value;
      break;
    case SETTING_LONGDATEFORMAT:
      setting_longdateformat = value;
      break;
    case SETTING_DATEFORMAT:
      setting_dateformat = value;
      break;
    case SETTING_THEME:
      setting_theme = value;
      break;
    case SETTING_DISPLAY_STATUSBAR:
      setting_display_statusbar = value;
      // This automatically sets the statusbar to show or hide throughout all the UI:
      if (setting_display_statusbar) {
        EnableStatusArea(0);
        DefineStatusAreaFlags(DSA_SETDEFAULT, 0, 0, 0);
        DefineStatusAreaFlags(3, SAF_BATTERY | SAF_TEXT | SAF_GLYPH | SAF_ALPHA_SHIFT, 0, 0);
      } else {
        EnableStatusArea(3);
      }
      break;
    case SETTING_STARTUP_BRIGHTNESS:
      setting_startup_brightness = value;
      break;
    case SETTING_SHOW_ADVANCED:
      setting_show_advanced = value;
      break;
    case SETTING_DISPLAY_FKEYS:
      setting_display_fkeys = value;
      break;
    case SETTING_PASSWORD_PRIVACY:
      setting_password_privacy = value;
      break;
    case SETTING_SHOW_CALENDAR_EVENTS_COUNT:
      setting_show_calendar_events_count = value;
      break;
    case SETTING_IS_FIRST_RUN:
      setting_is_first_run = value;
      break;
    case SETTING_ENABLE_LOCK:
      setting_enable_lock = value;
      break;
    case SETTING_LOCK_AUTOOFF:
      setting_lock_autooff = value;
      break;
    case SETTING_LOCK_ON_EXE:
      setting_lock_on_exe = value;
      break;
    case SETTING_UNLOCK_RUNMAT:
      setting_unlock_runmat = value;
      break;
    case SETTING_CLOCK_TYPE:
      setting_clock_type = value;
      break;
    case SETTING_CLOCK_SECONDS:
      setting_clock_seconds = value;
      break;
    case SETTING_HOME_PANES:
      setting_home_panes = value;
      break;
    case SETTING_DEFAULT_CALENDAR_VIEW:
      setting_default_calendar_view = value;
      break;
    default:
      break;
  }
  if (autosave) SaveSettings();
}

// Routines for saving and loading settings
int LoadSettings() { // returns 0 on success, 1 if settings were reset (first run / incompatibility)
  int size;
  MCSGetDlen2(DIRNAME, SETTINGSFILE, &size);
  if (size == 0) return 1;

  unsigned char buffer[NUMBER_OF_SETTINGS+2];
  MCSGetData1(0, NUMBER_OF_SETTINGS+1, buffer); // +1 because NUMBER_OF_SETTINGS does not include the file version
  // detect if the current settings file version is compatible with the one we're expecting.
  // file version is always the first byte, except for the settings files of versions 1.0 and earlier.
  // for these, the filesize happened to be equal or smaller than 16. The following is a dirty hack to handle such files.
  // (later, it was found that the file size doesn't relate directly to the amount of settings, and that's why this code is newly written)
  if (size <= 16) return 1; // reset settings
  // looks like the file is a "modern" one, so check the version byte.
  if (buffer[0] != SETTINGSFILE_VERSION) return 1; //if incompatible, reset settings
  
  int curupd = 1; //setting count starts at one (zero is for the file version, which we already took care of)
  // this assumes same setting IDs across files. Also, setting IDs must be consecutive
  // still, if a setting has no correspondence, SetSetting will just ignore the command...
  while(curupd <= NUMBER_OF_SETTINGS) {
    SetSetting(curupd, buffer[curupd], 0); // do not save every time, as we're loading right now...
    curupd++;
  }
  return 0;
}

void SaveSettings() {
  int createResult = MCS_CreateDirectory( DIRNAME );

  if(createResult != 0) // Check if directory exists
  { // directory already exists, so delete the exiting file that may be there
    MCSDelVar2(DIRNAME, SETTINGSFILE);
  }
  unsigned char buffer[NUMBER_OF_SETTINGS+2];
  int curupd = 0; //setting count starts at one, but we want to retrieve the file version too...
  while(curupd <= NUMBER_OF_SETTINGS+1) { //+1 because here we want to save the file version too
    buffer[curupd] = GetSetting(curupd);
    curupd++;
  }
  MCSPutVar2(DIRNAME, SETTINGSFILE, NUMBER_OF_SETTINGS+1, buffer); //+1 on purpose again, for the same reason
}
