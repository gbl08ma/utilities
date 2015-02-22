#ifndef __MEMSYSGUI_H
#define __MEMSYSGUI_H

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
#include "fileProvider.hpp"

#define TOTAL_SMEM 16801792 //as seen on the TEST MODE, on the emulator, OS 1.02, and on the TEST MODE of a real fx-CG 20, OS 1.04.
#define TOTAL_PASSWORD 0x1FFFF
#define DRAW_MEMUSAGE_GRAPHS

typedef struct
{
  int active; //whether the add-in is set to show in menu (.g3a) or hidden (.h3a)
  char filename[MAX_FILENAME_SIZE]; //filename, not proper for use with Bfile.
  char name[MAX_NAME_SIZE]; //friendly name
} AddIn;

void memoryCapacityViewer(int isPaneOffset=0);
int GetAddins(AddIn addins[]);
void addinManager();
int addinManagerSub(Menu* menu);
void changeFKeyColor();
void systemInfo();
void userInfo();

#endif 
