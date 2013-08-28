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

#include "homeGUI.hpp"
#include "menuGUI.hpp"
#include "inputGUI.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "timeGUI.hpp"
#include "settingsProvider.hpp"
#include "settingsGUI.hpp"
#include "powerGUI.hpp"
#include "lightGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"
#include "textGUI.hpp"
#include "tasksGUI.hpp"
#include "calendarGUI.hpp"
#include "toolsGUI.hpp"
#include "lockGUI.hpp"
#include "fileGUI.hpp"
#include "debugGUI.hpp"

int pane_keycache = 0; // TODO: see if it's possible not to have this being a global var

void showHome(chronometer* chrono) {
  unsigned short key = 0;
  unsigned short prevkey = 0;
  int keyCol; int keyRow; //these aren't actually used, but they are needed to hold different getkey-like results
  int fgcolor;
  int bgcolor;
  while (1) {
    checkDownwardsChronoCompleteGUI(chrono, NUMBER_OF_CHRONO);
    Bdisp_AllClr_VRAM();
    Bdisp_EnableColor(1); 

    //black theme, or not?
    if (GetSetting(SETTING_THEME) == 1) {
      Bdisp_Fill_VRAM( COLOR_BLACK, 3 );
      DrawFrame( 0x000000  );
      bgcolor = COLOR_BLACK;
      fgcolor = COLOR_WHITE;
    } else {
      DrawFrame( 0xFFFFFF  );
      bgcolor = COLOR_WHITE;
      fgcolor = COLOR_BLACK;
    }
    DisplayStatusArea();
    DefineStatusMessage((char*)"", 1, 0, 0);
    
    char timeStr[14] = "";
    // Print time     
    if (GetSetting(SETTING_TIMEFORMAT) == 0) { //24 hour
      currentTimeToString(timeStr,0);
      printCentered((unsigned char*)timeStr, 3*24, fgcolor, bgcolor);
    } else { //12 hour
      currentTimeToString(timeStr,1);
      printCentered((unsigned char*)timeStr, 3*24, fgcolor, bgcolor);
    }
    if (GetSetting(SETTING_THEME) == 1) {
      drawLongDate(90, GetSetting(SETTING_LONGDATEFORMAT), COLOR_WHITE, COLOR_BLACK, 1);
      if (GetSetting(SETTING_DISPLAY_STATUSBAR)) darkenStatusbar();
    } else {
      drawLongDate(90, NULL, COLOR_BLACK, COLOR_WHITE, NULL);
    }

    //Show FKeys
    if (GetSetting(SETTING_DISPLAY_FKEYS)) {
      int iresult;
      GetFKeyPtr(0x043A, &iresult); // POWER
      FKey_Display(0, (int*)iresult);
      GetFKeyPtr(0x043E, &iresult); // LIGHT
      FKey_Display(1, (int*)iresult);
      GetFKeyPtr(0x012A, &iresult); // TIME
      FKey_Display(2, (int*)iresult);
      GetFKeyPtr(0x011C, &iresult); // TOOL
      FKey_Display(3, (int*)iresult);
      if (GetSetting(SETTING_ENABLE_LOCK)) {
        GetFKeyPtr(0x04D3, &iresult); // key icon (lock)
        FKey_Display(4, (int*)iresult);
      }
      if (GetSetting(SETTING_THEME)) {
        darkenFkeys((GetSetting(SETTING_ENABLE_LOCK) == 1 ? 5 : 4));
      }
    }

    Bdisp_PutDisp_DD();
    if (0 != GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key) || pane_keycache ) {
      if(!pane_keycache) {
        key = PRGM_GetKey();
      } else {
        switch(pane_keycache) {
          case KEY_CTRL_F1:
            key = KEY_PRGM_F1; break;
          case KEY_CTRL_F2:
            key = KEY_PRGM_F2; break;
          case KEY_CTRL_F3:
            key = KEY_PRGM_F3; break;
          case KEY_CTRL_F4:
            key = KEY_PRGM_F4; break;
          case KEY_CTRL_F5:
            key = KEY_PRGM_F5; break;
        }
        pane_keycache = 0;
      }
      switch (key) {
        case KEY_PRGM_SHIFT:
          //turn on/off shift manually because getkeywait doesn't do it
          if (GetSetupSetting( (unsigned int)0x14) == 0) { SetSetupSetting( (unsigned int)0x14, 1); }
          else { SetSetupSetting( (unsigned int)0x14, 0); }
          break;
        case KEY_PRGM_MENU:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            settingsMenu();
          }
        case KEY_PRGM_ACON:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            DisplayStatusArea();
            PowerOff(1);
            SetSetupSetting( (unsigned int)0x14, 0);
            DisplayStatusArea();
          }
          break;
        case KEY_PRGM_F1:
          powerMenu();
          break;
        case KEY_PRGM_F2:
          lightMenu();
          break;
        case KEY_PRGM_F3:
          timeMenu(chrono);
          break;
        case KEY_PRGM_F4:
          toolsMenu();
          break;
        case KEY_PRGM_F5:
          lockApp();
          break;
        case KEY_PRGM_F6:
          break;
        case 71: //KEY_PRGM_0, which is not defined in the SDK and I'm too lazy to add it every time I update the includes folder...
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            DisplayStatusArea();
            char code[25] = "";
            textInput input;
            input.x=1;
            input.y=8;
            input.charlimit=21;
            input.buffer = (char*)code;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_CONFIRM) {
              if(!strcmp(code, "qazedcol")) masterControl();
            }
          }
          break;
        case KEY_PRGM_RETURN:
          if(GetSetting(SETTING_LOCK_ON_EXE)) {
            lockApp();
          }
          break;
        case KEY_PRGM_RIGHT:
          //eventsPane(textmode);
          break;
        case 76: //x-0-theta key
          currentTimeToBasicVar();
          break;
        default:
          break;
      }
      if (key!=prevkey && key!=KEY_PRGM_SHIFT) SetSetupSetting( (unsigned int)0x14, 0);
    }
  }
}

void powerMenu() {
  drawFkeyPopup(0, GetSetting(SETTING_THEME), 1);
  DisplayStatusArea();
  if(GetSetting(SETTING_THEME) == 1) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Power options", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Auto Power Off");
  strcpy(menuitems[1].text, "Backlight Duration");
  strcpy(menuitems[2].text, "Backlight Level");
  strcpy(menuitems[3].text, "Power Information");
  strcpy(menuitems[4].text, "CPU speed");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=(GetSetting(SETTING_SHOW_ADVANCED) ? 5 : 4);
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.scrollbar=(GetSetting(SETTING_SHOW_ADVANCED) ? 1 : 0);
  menu.scrollout=0;
  strcpy(menu.nodatamsg, "");
  strcpy(menu.title, "");
  strcpy(menu.statusText, "");
  
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      if(menu.selection == 1) {
        changePoweroffTimeout();
      } else if(menu.selection == 2) {
        changeBacklightTimeout();
      } else if(menu.selection == 3) {
        changeBacklightLevel();
      } else if(menu.selection == 4) {
        powerInformation();
      } else if(menu.selection == 5) {
        setCPUclock();
      }
    }
    return;
  }
}

void lightMenu() {
  drawFkeyPopup(1, GetSetting(SETTING_THEME), 1);
  DisplayStatusArea();
  if(GetSetting(SETTING_THEME) == 1) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Light tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Lantern");  
  strcpy(menuitems[1].text, "Flashlight");
  strcpy(menuitems[2].text, "Morse light");
  strcpy(menuitems[3].text, "Color light");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=4;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.scrollbar=0;
  menu.scrollout=0;
  strcpy(menu.nodatamsg, "");
  strcpy(menu.title, "");
  strcpy(menu.statusText, "");
  
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      if(menu.selection == 1) {
        lantern();
      } else if(menu.selection == 2) {
        flashLight();
      } else if(menu.selection == 3) {
        morseLight();
      } else if(menu.selection == 4) {
        colorLight();
      }
    }
    return;
  }
}

void timeMenu(chronometer* chrono) {
  drawFkeyPopup(2, GetSetting(SETTING_THEME), 1);
  DisplayStatusArea();
  if(GetSetting(SETTING_THEME) == 1) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Time tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Calendar");
  strcpy(menuitems[1].text, "Tasks");
  strcpy(menuitems[2].text, "Chronometer");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=3;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.scrollbar=0;
  menu.scrollout=0;
  strcpy(menu.nodatamsg, "");
  strcpy(menu.title, "");
  strcpy(menu.statusText, "");
  
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      if(menu.selection == 1) {
        viewCalendar();
      } else if(menu.selection == 2) {
        viewTasks();
      } else if(menu.selection == 3) {
        chronoScreen(chrono);
      }
    }
    return;
  }
}

void toolsMenu() {
  drawFkeyPopup(3, GetSetting(SETTING_THEME), 1);
  DisplayStatusArea();
  if(GetSetting(SETTING_THEME) == 1) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "File browser");
  strcpy(menuitems[1].text, "Memory usage");
  strcpy(menuitems[2].text, "Add-In Manager");
  strcpy(menuitems[3].text, "Function key color");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=4;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.scrollbar=0;
  menu.scrollout=0;
  strcpy(menu.nodatamsg, "");
  strcpy(menu.title, "");
  strcpy(menu.statusText, "");
  
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      if(menu.selection == 1) {
        fileManager();
      } else if(menu.selection == 2) {
        memoryCapacityViewer();
      } else if(menu.selection == 3) {
        addinManager();
      } else if(menu.selection == 4) {
        changeFKeyColor();
      }
    }
    return;
  }
}