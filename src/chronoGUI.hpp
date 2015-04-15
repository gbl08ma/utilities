#ifndef __CHRONOGUI_H
#define __CHRONOGUI_H

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

#include "chronoProvider.hpp"
#include "menuGUI.hpp"

void formatChronoString(chronometer* tchrono, int num, char* string, int isChronoView);
void stopAndUninstallStubTimer();
void chronoScreen();
void startSelectedChronos(Menu* menu, chronometer* tchrono, int count);
void stopSelectedChronos(Menu* menu, chronometer* tchrono, int count);
void clearSelectedChronos(Menu* menu, chronometer* tchrono, int count);
void setChronoScreen(Menu* menu, chronometer* tchrono, int menuSelOnly);
void setPresetChrono(Menu* menu, chronometer* tchrono);
int getLastCompleteChrono();
void checkChronoComplete(chronometer* chronoarray);
void viewChrono(Menu* menu, chronometer* chrnarr);

#endif
