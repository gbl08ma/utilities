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

#include "powerGUI.hpp"
#include "menuGUI.hpp"
#include "selectorGUI.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "timeProvider.hpp"
 
void lantern() {
  int key;  
  unsigned int prevlevel = GetBacklightSubLevel_RAW();
  SetBacklightSubLevel_RAW(249);
  Bdisp_AllClr_VRAM();
  SetGetkeyToMainFunctionReturnFlag(0); //Disable menu return so that we always have a chance to set the brightness correctly
  while(1) {
    GetKey(&key);
    if(key==KEY_CTRL_EXIT || key==KEY_CTRL_MENU) break;
  }
  SetBacklightSubLevel_RAW(prevlevel);
  SetGetkeyToMainFunctionReturnFlag(1); //Enable menu return
  return;
}

void flashLight(int noDraw) { // if noDraw is true, this function will just change the backlight levels without changing VRAM contents
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  unsigned int prevlevel = 0;
  int previousTicks = 0;
  while (1) {
    //the following keyboard reading method does not process MENU so we always have a chance to set the brightness correctly
    if(PRGM_GetKey() == KEY_PRGM_EXIT) {
      SetBacklightSubLevel_RAW(initlevel);
      if(!noDraw) DrawFrame( COLOR_WHITE );
      return;
    }
    if(!previousTicks || getMSdiff(previousTicks, RTC_GetTicks()) >= 500) {
      if (prevlevel == 249) {
        SetBacklightSubLevel_RAW(0);
        prevlevel = 0;
        if(!noDraw) {
          Bdisp_Fill_VRAM( COLOR_BLACK, 3 );
          DrawFrame( COLOR_BLACK );
        }
      } else {
        SetBacklightSubLevel_RAW(249);
        prevlevel = 249; 
        if(!noDraw) {
          Bdisp_Fill_VRAM( COLOR_WHITE, 3 ); 
          DrawFrame( COLOR_WHITE );
        }
      }
      previousTicks = RTC_GetTicks();
    }
    if(!noDraw) Bdisp_PutDisp_DD();
  }
}

void morseLight() {
  unsigned short key; 
  int keyCol, keyRow; 
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  while (1) {
    //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
    if (0 != GetKeyWait_OS(&keyCol,&keyRow,KEYWAIT_HALTOFF_TIMEROFF,0,1, &key) ) {
      if (keyCol == 4 && keyRow == 8) { SetBacklightSubLevel_RAW(initlevel); return; }
      Bdisp_Fill_VRAM( COLOR_WHITE, 3 ); DrawFrame( COLOR_WHITE );
      SetBacklightSubLevel_RAW(249);
    } else {
      SetBacklightSubLevel_RAW(0);
      Bdisp_Fill_VRAM( COLOR_BLACK, 3 ); DrawFrame( COLOR_BLACK );
    }
    Bdisp_PutDisp_DD();
  }
}

void colorLight() {
  int gkey;
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  
  MenuItem menuitems[10];
  strcpy(menuitems[0].text, "Blue");  
  strcpy(menuitems[1].text, "Green");
  strcpy(menuitems[2].text, "Red");
  strcpy(menuitems[3].text, "Orange");
  strcpy(menuitems[4].text, "Yellow");
  strcpy(menuitems[5].text, "Cyan");
  strcpy(menuitems[6].text, "Brown");
  strcpy(menuitems[7].text, "Magenta");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=8;
  menu.showtitle=1;
  strcpy(menu.title, "Color light");
  int res = doMenu(&menu);
  if (res == MENU_RETURN_EXIT) return;
  
  SetBacklightSubLevel_RAW(249);
  color_t color = COLOR_WHITE;
  switch (menu.selection) {
    case 1: color = COLOR_BLUE; break;
    case 2: color = COLOR_GREEN; break;
    case 3: color = COLOR_RED; break;
    case 4: color = COLOR_ORANGE; break;
    case 5: color = COLOR_YELLOW; break;
    case 6: color = COLOR_CYAN; break;
    case 7: color = COLOR_BROWN; break;
    case 8: color = COLOR_MAGENTA; break;
  }
  Bdisp_Fill_VRAM(color, 3 );
  DrawFrame(color);
  SetGetkeyToMainFunctionReturnFlag(0); //Disable menu return so that we always have the chance to set the brightness correctly
  while(1) {
    GetKey(&gkey);
    if(gkey==KEY_CTRL_EXIT) break;
  }
  SetGetkeyToMainFunctionReturnFlag(1); //Enable menu return
  SetBacklightSubLevel_RAW(initlevel);
}