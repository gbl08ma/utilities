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
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"

#ifdef ENABLE_DEBUG
static int debugMode = 0; // when 1, debug mode is on
int GetDebugMode() {
  return debugMode;
}
void SetDebugMode(int val) {
  debugMode = val;
}

void debugMessage(char* text1, char* text2, int value) {
  int key;
  MsgBoxPush(4);
  PrintXY(3,2,(char*)text1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  char buffer1[10];
  itoa(value, (unsigned char*)buffer1);
  char buffer2[15];
  strcpy(buffer2, text2);
  strcat(buffer2, buffer1);
  PrintXY(3,3,(char*)buffer2, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mGetKey(&key);
  MsgBoxPop();
}
void showRAMused() {
  int ramused = 0x881E0000 - (int)GetStackPtr();
  debugMessage((char*)"  RAM", (char*)"  b:", ramused);
  debugMessage((char*)"  MB", (char*)"  c:", getNumberOfMsgBoxPushed());
}

#endif

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
  menuitems[0].text = (char*)"SS";
  menuitems[1].text = (char*)"LS";
  menuitems[2].text = (char*)"Lk0";
  menuitems[3].text = (char*)"Lk1";
#ifdef ENABLE_DEBUG
  menuitems[4].text = (char*)"Dbg0";
  menuitems[5].text = (char*)"Dbg1";
  menuitems[6].text = (char*)"FR0";
  menuitems[7].text = (char*)"FR1";
  menuitems[8].text = (char*)"Rst";
#endif
  
  Menu menu;
  menu.items=menuitems;
#ifdef ENABLE_DEBUG  
  menu.numitems=9;
#else 
  menu.numitems=4;
#endif
  menu.scrollout=1;
#ifdef ENABLE_DEBUG  
  menu.title = (char*)"CTRL";
#endif
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1: SaveSettings(); break;
        case 2: LoadSettings(); break;
        case 3: 
        case 4:
          SetSetting(SETTING_ENABLE_LOCK, menu.selection-3, 0); // do not autosave, to allow full control of the operator
          break;
#ifdef ENABLE_DEBUG          
        case 5:
        case 6:
          SetDebugMode(menu.selection-5);
          break;
        case 7:
        case 8:
          SetSetting(SETTING_IS_FIRST_RUN, menu.selection-7, 0); // do not autosave, to allow full control of the operator
          break;
        case 9:
          //CallbackAtQuitMainFunction( mycallback );
          Restart();
          break;
#endif
      }
    }
  }
}