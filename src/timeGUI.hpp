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

void drawLongDate(int textY, int format, int colorfg, int colorbg, int miniminiinvert);
void drawAnalogClockFace(int cx, int cy, int radius, int colorbg, int colorfg);
void drawAnalogClockSecondNeedle(int s, int cx, int cy, double radius, int colorfg);
void drawAnalogClockMinuteNeedle(int m, int s, int cx, int cy, double radius, int colorfg);
void drawAnalogClockHourNeedle(int h, int m, int s, int cx, int cy, double radius, int colorfg);
void drawAnalogClock(int cx, int cy, int radius, int colorbg, int colorfg);
void RTCunadjustedWizard(int helpMessage);
void setTimeGUI(int canExit = 1);
void setDateGUI(int canExit = 1);
void currentTimeToBasicVar();
void drawHomeClock(int format, int fgcolor, int bgcolor);
#endif