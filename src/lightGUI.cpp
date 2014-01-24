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
  GetKey(&key);
  SetBacklightSubLevel_RAW(prevlevel);
  return;
}

void flashLight(int noDraw) { // if noDraw is true, this function will just change the backlight levels without changing VRAM contents
  unsigned short key; 
  int keyCol, keyRow; 
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  unsigned int prevlevel = 249;
  if(!noDraw) Bdisp_AllClr_VRAM();
  int previousTicks = RTC_GetTicks();
  while (1) {
    if(!noDraw) Bdisp_PutDisp_DD();
    //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
    if (0 != GetKeyWait_OS(&keyCol,&keyRow,2,0,1, &key) ) {
      if(keyCol == 4 && keyRow == 8) {
        SetBacklightSubLevel_RAW(initlevel);
        if(!noDraw) DrawFrame( COLOR_WHITE );
        return;
      }
    }
    if(getMSdiff(previousTicks, RTC_GetTicks()) >= 500) {
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
  }
}

void morseLight() {
  unsigned short key; 
  int keyCol, keyRow; 
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  Bdisp_AllClr_VRAM();
  while (1) {
    Bdisp_PutDisp_DD();
    //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
    if (0 != GetKeyWait_OS(&keyCol,&keyRow,KEYWAIT_HALTOFF_TIMEROFF,0,1, &key) ) {
      if (keyCol == 4 && keyRow == 8) { SetBacklightSubLevel_RAW(initlevel); return; }
      Bdisp_Fill_VRAM( COLOR_WHITE, 3 ); DrawFrame( COLOR_WHITE );
      SetBacklightSubLevel_RAW(249);
    } else {
      SetBacklightSubLevel_RAW(0);
      Bdisp_Fill_VRAM( COLOR_BLACK, 3 ); DrawFrame( COLOR_BLACK );
    }
    
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
  
  Bdisp_AllClr_VRAM();
  SetBacklightSubLevel_RAW(249);
  switch (menu.selection) {
    case 1: Bdisp_Fill_VRAM( COLOR_BLUE, 3 ); DrawFrame( COLOR_BLUE  ); break;
    case 2: Bdisp_Fill_VRAM( COLOR_GREEN, 3 ); DrawFrame( COLOR_GREEN  ); break;
    case 3: Bdisp_Fill_VRAM( COLOR_RED, 3 ); DrawFrame( COLOR_RED  ); break;
    case 4: Bdisp_Fill_VRAM( COLOR_ORANGE, 3 ); DrawFrame( COLOR_ORANGE  ); break;
    case 5: Bdisp_Fill_VRAM( COLOR_YELLOW, 3 ); DrawFrame( COLOR_YELLOW  ); break;
    case 6: Bdisp_Fill_VRAM( COLOR_CYAN, 3 ); DrawFrame( COLOR_CYAN  ); break;
    case 7: Bdisp_Fill_VRAM( COLOR_BROWN, 3 ); DrawFrame( COLOR_BROWN  ); break;
    case 8: Bdisp_Fill_VRAM( COLOR_MAGENTA, 3 ); DrawFrame( COLOR_MAGENTA  ); break;
  }

  Bdisp_PutDisp_DD();
  GetKey(&gkey);
  SetBacklightSubLevel_RAW(initlevel);
}