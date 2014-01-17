#ifndef __HOMEGUI_H
#define __HOMEGUI_H
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

void showHome(chronometer* chrono);
inline void powerMenu();
inline void lightMenu();
inline void timeMenu(chronometer* chrono);
inline void toolsMenu();
void eventsPane();

#endif