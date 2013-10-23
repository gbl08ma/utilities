#ifndef __TOOLSGUI_H
#define __TOOLSGUI_H

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

#include "menuGUI.hpp"

#define TOTAL_SMEM 16801792 //as seen on the TEST MODE, on the emulator, OS 1.02, and on the TEST MODE of a real fx-CG 20, OS 1.04.
#define DRAW_MEMUSAGE_GRAPHS

typedef struct
{
  int active; //whether the add-in is set to show in menu (.g3a) or hidden (.h3a)
  char filename[128]; //filename, not proper for use with Bfile.
  char name[50]; //friendly name
} AddIn;

void memoryCapacityViewer();
int GetAddins(AddIn addins[]);
void addinManager();
int addinManagerSub(Menu* menu);
void changeFKeyColor();
void systemInfo();

#endif 
