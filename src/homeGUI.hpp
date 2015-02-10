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
void powerMenu(int* pane_keycache);
void lightMenu(int* pane_keycache);
void timeMenu(chronometer* chrono, int* pane_keycache);
void memsysMenu(int* pane_keycache);
void toolsMenu(int* pane_keycache);
void handleHomePane(int key, int* pane_keycache);
void eventsPane(int retkey, int* pane_keycache);
void memoryUsagePane(int retkey, int* pane_keycache);

#endif