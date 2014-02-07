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

#include "debugGUI.hpp"
#include "menuGUI.hpp"
#include "settingsProvider.hpp"
#include "linkProvider.hpp"
#include "graphicsProvider.hpp"

static int debugMode = 0; // when 1, debug mode is on
int GetDebugMode() {
  return debugMode;
}
void SetDebugMode(int val) {
  debugMode = val;
}

static int initStackPtr;
void debugMessage(char* text1, char* text2, int value) {
  int key;
  MsgBoxPush(4);
  PrintXY(3,2,(char*)text1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  char buffer1[10] = "";
  itoa(value, (unsigned char*)buffer1);
  char buffer2[15] = "";
  strcpy(buffer2, text2);
  strcat(buffer2, buffer1);
  PrintXY(3,3,(char*)buffer2, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  GetKey(&key);
  MsgBoxPop();
}
void setInitStackPtr(int val) {
  initStackPtr = val;
}
void showRAMused() {
  int usedStack = (int)GetStackPtr();
  int ramused = initStackPtr - usedStack;
  debugMessage((char*)"  RAM", (char*)"  b:", ramused);
  debugMessage((char*)"  MB", (char*)"  c:", getNumberOfMsgBoxPushed());
}

/*void showRAMusedStatus() {
  int usedStack = (int)GetStackPtr();
  int ramused = initStackPtr - usedStack;
  
  char buffer1[10] = "";
  char buffer2[25] = "";
  itoa(ramused, (unsigned char*)buffer1);
  strcpy(buffer2, "RAM used: ");
  strcat(buffer2, buffer1);
  
  EnableStatusArea(0);    // necessary, if the status area should be used by system functions!
  DefineStatusAreaFlags(DSA_SETDEFAULT, 0, 0, 0); // necessary, if the status area should be used by system functions!
  DefineStatusAreaFlags(3, SAF_BATTERY|SAF_TEXT|SAF_GLYPH, 0, 0);
  
  DefineStatusMessage((char*)buffer2, 1, 0, 0);
  DisplayStatusArea();
}*/

/*void mycallback() {
  debugMessage((char*)"  My menu", (char*)"  callback", 0);
}*/

void masterControl() {
  MenuItem menuitems[10];
  strcpy(menuitems[0].text, "SS()");;
  strcpy(menuitems[1].text, "LS()");
  strcpy(menuitems[2].text, "Lk Off");
  strcpy(menuitems[3].text, "Lk On");
  strcpy(menuitems[4].text, "Dbg Off");
  strcpy(menuitems[5].text, "Dbg On");
  strcpy(menuitems[6].text, "FR Off");
  strcpy(menuitems[7].text, "FR On");
  strcpy(menuitems[8].text, "Rst");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=9;
  menu.scrollout=1;
  menu.showtitle=1;
  strcpy(menu.title, "UTILS CTRL");
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1: SaveSettings(); break;
        case 2: LoadSettings(); break;
        case 3: 
          SetSetting(SETTING_ENABLE_LOCK, 0, 0); // do not autosave, to allow full control of the operator
          break;
        case 4:
          SetSetting(SETTING_ENABLE_LOCK, 1, 0); // do not autosave, to allow full control of the operator
          break;
        case 5: SetDebugMode(0); break;
        case 6: SetDebugMode(1); break;
        case 7:
          SetSetting(SETTING_IS_FIRST_RUN, 0, 0); // do not autosave, to allow full control of the operator
          break;
        case 8:
          SetSetting(SETTING_IS_FIRST_RUN, 1, 0); // do not autosave, to allow full control of the operator
          break;
        case 9:
          //CallbackAtQuitMainFunction( mycallback );
          Restart();
          break;
      }
    }
  }
}