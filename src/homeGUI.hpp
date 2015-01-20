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
inline void powerMenu(int* pane_keycache);
inline void lightMenu(int* pane_keycache);
inline void timeMenu(chronometer* chrono, int* pane_keycache);
inline void memsysMenu(int* pane_keycache);
inline void toolsMenu(int* pane_keycache);
void eventsPane(int* pane_keycache);

#endif