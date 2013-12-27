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
  while(1) {
    GetKey(&key);
    break;
  } 
  MsgBoxPop();
}
void setInitStackPtr(int val) {
  initStackPtr = val;
}
void showRAMused() {
  int usedStack = (int)GetStackPtr();
  int ramused = initStackPtr - usedStack;
  debugMessage((char*)"  RAM USED", (char*)"  bytes:", ramused);
  debugMessage((char*)"  MsgBox PUSHED", (char*)"  count:", getNumberOfMsgBoxPushed());
}

void showRAMusedStatus() {
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
}

/*void mycallback() {
  debugMessage((char*)"  My menu", (char*)"  callback", 0);
}*/

void masterControl() {
  MenuItem menuitems[10];
  strcpy(menuitems[0].text, "SaveSettings()");;
  strcpy(menuitems[1].text, "LoadSettings()");
  strcpy(menuitems[2].text, "Lock Off");
  strcpy(menuitems[3].text, "Lock On");
  strcpy(menuitems[4].text, "Debug Off");
  strcpy(menuitems[5].text, "Debug On");
  strcpy(menuitems[6].text, "First Run Off");
  strcpy(menuitems[7].text, "First Run On");
  strcpy(menuitems[8].text, "Restart");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=9;
  menu.scrollout=1;
  menu.showtitle=1;
  menu.allowMkey=0;
  strcpy(menu.title, "UTILITIES CONTROL");
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      if(menu.selection == 1) {
        SaveSettings();
      }
      if(menu.selection == 2) { 
        LoadSettings();
      }
      if(menu.selection == 3) { 
        SetSetting(SETTING_ENABLE_LOCK, 0, 0); // do not autosave, to allow full control of the operator
      }
      if(menu.selection == 4) { 
        SetSetting(SETTING_ENABLE_LOCK, 1, 0); // do not autosave, to allow full control of the operator
      }
      if(menu.selection == 5) { 
        SetDebugMode(0);
      }
      if(menu.selection == 6) { 
        SetDebugMode(1);
      }
      if(menu.selection == 7) { 
        SetSetting(SETTING_IS_FIRST_RUN, 0, 0); // do not autosave, to allow full control of the operator
      }
      if(menu.selection == 8) { 
        SetSetting(SETTING_IS_FIRST_RUN, 1, 0); // do not autosave, to allow full control of the operator
      }
      if(menu.selection == 9) {
        //CallbackAtQuitMainFunction( mycallback );
        Restart();
      }
    }
  }
}