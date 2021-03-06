#ifndef __TIMEGUI_H
#define __TIMEGUI_H

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

void drawLongDate(int textY, int format, int colorfg, int colorbg, int miniminiinvert=0);
void drawAnalogClock(int cx, int cy, int radius, int colorbg, int colorfg);
void drawAnalogChronometer(int cx, int cy, int radius, int colorbg, int colorfg, int d, int h, int m, int sPlusMs);
void RTCunadjustedWizard(int helpMessage, int ignoreAdjusted = 0);
void setTimeScreen(int canExit = 1);
void setDateScreen(int canExit = 1);
void currentTimeToBasicVar();
void drawHomeClock(int format, int theme=getSetting(SETTING_THEME));
void setTimezone();
#endif