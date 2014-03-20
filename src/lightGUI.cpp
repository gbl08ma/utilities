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

void pushBogusKey() {
  Keyboard_PutKeycode( 5, 3, 0);
}

void flashLight(int noDraw) { // if noDraw is true, this function will just change the backlight levels without changing VRAM contents
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  unsigned int prevlevel = 0;
  int timer = Timer_Install(0, pushBogusKey, 500);
  SetGetkeyToMainFunctionReturnFlag(0); //Disable menu return. This way, we always have a chance to set the brightness correctly
  if (timer > 0) { Timer_Start(timer); }
  while (1) {
    // attempt to fix a bug which will hang calc if flashLight is called right after turning on.
    int key;
    
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
    GetKey(&key);
    if(key == KEY_CTRL_EXIT) {
      SetBacklightSubLevel_RAW(initlevel);
      if(!noDraw) DrawFrame( COLOR_WHITE );
      if(timer>0) {
        Timer_Stop(timer);
        Timer_Deinstall(timer);
      }
      SetGetkeyToMainFunctionReturnFlag(1); //Enable menu return
      return;
    }
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
  menuitems[0].text = (char*)"Blue";  
  menuitems[1].text = (char*)"Green";
  menuitems[2].text = (char*)"Red";
  menuitems[3].text = (char*)"Orange";
  menuitems[4].text = (char*)"Yellow";
  menuitems[5].text = (char*)"Cyan";
  menuitems[6].text = (char*)"Brown";
  menuitems[7].text = (char*)"Magenta";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=8;
  menu.title = (char*)"Color light";
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