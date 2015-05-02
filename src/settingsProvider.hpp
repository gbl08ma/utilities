#ifndef __SETTINGSPROVIDER_H
#define __SETTINGSPROVIDER_H

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

#include "constantsProvider.hpp"

// SETTINGS engine
/*App settings.*/
// the default value of the settings goes on settingsProvider.cpp

#define SETTINGSFILE_VERSION 14 // NOTE: increase this when changing the amount or meaning of settings!

#define SETTING_SELF_FILEVERSION 0 // this is a special field used by the settings system to determine file compatibility
#define SETTING_TIMEFORMAT 1 // 0 = 24-hour HH:MM:SS ; 1 = 12-hour HH:MM:SS AM/PM
#define SETTING_LONGDATEFORMAT 2
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

#define SETTING_DATEFORMAT 3 // 0 = DD/MM/YYYY, 1 = MM/DD/YYYY, 2 = YYYY/MM/DD
#define SETTING_THEME 4  // whether the home screen should use a dark/inverted theme. 0: white theme; 1: dark theme
#define SETTING_DISPLAY_STATUSBAR 5 // whether the status bar should be displayed
#define SETTING_STARTUP_BRIGHTNESS 6 // screen brightness level to force on add-in startup, 0~249. 250 for no forcing
#define SETTING_SHOW_ADVANCED 7  // whether to show advanced things like CPU clock selection
#define SETTING_DISPLAY_FKEYS 8 // 0: show last input character when entering passwords; 1: do not show last char
#define SETTING_PASSWORD_PRIVACY 9 // had a different name before, value meaning is the same
#define SETTING_SHOW_CALENDAR_EVENTS_COUNT 10 // 1 if this is the first time the add-in is run, then it's set to zero and never changed, unless the user cancels update checking, in which case it's set to 2. So: if 1 is first run, if any other value is not first run.
#define SETTING_IS_FIRST_RUN 11 // 1 if this is the first time the add-in is run, then it's set to zero and never changed, unless the user cancels update checking, in which case it's set to 2. So: if 1 is first run, if any other value is not first run.
#define SETTING_ENABLE_LOCK 12  // whether lock functionality is available or not. Should always be on and not changeable on the settings menu, except when special instructions are given to disable the function
#define SETTING_LOCK_AUTOOFF 13 // whether to turn off automatically after locking the calc
#define SETTING_LOCK_ON_EXE 14 // when enabled, calculator is locked when EXE is pressed on the home screen (i.e. legacy support for people used to the old lock add-in).
#define SETTING_UNLOCK_RUNMAT 15 // whether to jump to Run-Mat when calculator is unlocked. 2 when user should be asked.
#define SETTING_CLOCK_TYPE 16
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
#define SETTING_CLOCK_SECONDS 17 // whether to show seconds in clock
#define SETTING_DEFAULT_CALENDAR_VIEW 18 // default calendar view. 0 = week, 1 = month
#define SETTING_WEEK_START_DAY 19 // 0 = sunday, 1 = monday
#define SETTING_SHOW_CALENDAR_BUSY_MAP 20 
#define SETTING_CHRONO_NOTIFICATION_TYPE 21 // notification for when a downwards chrono finishes. 0: no notification; 1: pop-up with screen flashing; 2: pop-up without screen flashing; 3: message on home screen without pop-up
#define SETTING_FILE_MANAGER_SORT 22 // file list sort in file manager. 0: no sort (list as they appear), 1: A-Z, 2: Z-A, 3: size (ascending), 4: size (descending), 5: type A-Z, 6: type Z-A
#define SETTING_FILE_MANAGER_SEARCH 23 // settings for search in the file manager; the value of this is never used as a whole, but its bits are set individually for each search option.
#define SETTING_TIMEZONE 24 // clock offset from UTC in intervals of 15 min. Used in the TOTP authenticator. The value 0 represents UTC-12:45. 51 is UTC+00:00.

/* home pane screens and shortcuts:
   0 is disabled
   1 is today events pane
*/
#define SETTING_HOME_PANE_TOP 25
#define SETTING_HOME_PANE_RIGHT 26
#define SETTING_HOME_PANE_BOTTOM 27
#define SETTING_HOME_PANE_LEFT 28
#define SETTING_LOCK_USERNAME 29 // 0 = do not show owner info on lock screen, 1 = show (default)

#define NUMBER_OF_SETTINGS 29 // NOTE update this when adding settings. DOES NOT include the SETTING_SELF_FILEVERSION
/*End of settings*/ 

int getSetting(int setting);
void setSetting(int setting, int value, int autosave);
int loadSettings();
void saveSettings();

#endif