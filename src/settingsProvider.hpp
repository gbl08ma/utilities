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
#define SETTING_TIMEFORMAT 1
#define SETTING_LONGDATEFORMAT 2
#define SETTING_DATEFORMAT 3
#define SETTING_THEME 4
#define SETTING_DISPLAY_STATUSBAR 5
#define SETTING_STARTUP_BRIGHTNESS 6
#define SETTING_SHOW_ADVANCED 7
#define SETTING_DISPLAY_FKEYS 8
#define SETTING_PASSWORD_PRIVACY 9
#define SETTING_SHOW_CALENDAR_EVENTS_COUNT 10
#define SETTING_IS_FIRST_RUN 11
#define SETTING_ENABLE_LOCK 12
#define SETTING_LOCK_AUTOOFF 13
#define SETTING_LOCK_ON_EXE 14
#define SETTING_UNLOCK_RUNMAT 15

#define NUMBER_OF_SETTINGS 15 //NOTE update this when adding settings
/*End of settings*/ 

int GetSetting(int setting);
void SetSetting(int setting, int value, int autosave);
int LoadSettings();
void SaveSettings();

#endif