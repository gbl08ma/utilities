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
#include <alloca.h>

#include "homeGUI.hpp"
#include "menuGUI.hpp"
#include "inputGUI.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "timeGUI.hpp"
#include "settingsProvider.hpp"
#include "stringsProvider.hpp"
#include "settingsGUI.hpp"
#include "powerGUI.hpp"
#include "lightGUI.hpp"
#include "chronoGUI.hpp"
#include "textGUI.hpp"
#include "tasksGUI.hpp"
#include "calendarGUI.hpp"
#include "toolsGUI.hpp"
#include "memsysGUI.hpp"
#include "lockGUI.hpp"
#include "fileGUI.hpp"
#include "editorGUI.hpp"
#include "sprites.h"

void homeScreen(int isLocked) {
  unsigned short key = 0;
  int keyCol; int keyRow; // these are needed only to feed to syscalls, their value is ignored.
  int pane_keycache = 0;

  // find the registered username, if any:
  char* username = NULL;
  char* organization = NULL;
  if(isLocked && getSetting(SETTING_LOCK_USERNAME)) {
    char* flagpointer = (char*)0x80BE0000;
    while(*flagpointer == 0x0F) {
      if(*(flagpointer+0x2C) != '\0') {
        username = (flagpointer+0x18);
        organization = (flagpointer+0x04);
      }
      flagpointer = flagpointer + 0x40;
    }
  }
  while (1) {
    //black theme, or not?
    if (getSetting(SETTING_THEME) == 1) {
      Bdisp_Fill_VRAM(COLOR_BLACK, 3);
      DrawFrame(COLOR_BLACK);
    } else {
      Bdisp_AllClr_VRAM();
      DrawFrame(COLOR_WHITE);
    }
    if(getSetting(SETTING_CHRONO_NOTIFICATION_TYPE) == 3 && getLastCompleteChrono()) {
      char message[30];
      sprintf(message, "Chronometer %d ended", getLastCompleteChrono());
      DefineStatusMessage(message, 1, 4, 0);
    } else if (isDSTchangeToday() && !isLocked) {
      char message[100];
      stringToMini(message, "To adjust the clock: SHIFT then MENU");
      DefineStatusMessage(message, 1, 0, 0);
    } else DefineStatusMessage((char*)"", 1, 0, 0);
    if(getSetting(SETTING_DISPLAY_STATUSBAR)) {
      DisplayStatusArea();
      if(isLocked && !GetSetupSetting((unsigned int)0x14)) {
        setBrightnessToStartupSetting();
        drawRectangle(18, 2, 18, 18, COLOR_BLACK);
        drawRectangle(20, 4, 14, 14, COLOR_CYAN);
        CopySpriteNbitMasked(lock_icon, 22, 5, 10, 12, lock_icon_palette, 0xffff, 1);
      }
      if (getSetting(SETTING_THEME)) darkenStatusbar();
    }
   
    // Print time
    drawHomeClock(getSetting(SETTING_CLOCK_TYPE));

    if(!isLocked) {
      // Show FKeys
      if (getSetting(SETTING_DISPLAY_FKEYS)) {
        drawFkeyLabels(0x043A, 0x043E, 0x012A, 0x011C, 0x03E7, 
                       getSetting(SETTING_ENABLE_LOCK) ? 0x04D3 : -1);
                       //POWER, LIGHT, TIME, TOOL, M&S, key icon (lock)
        if (getSetting(SETTING_THEME)) {
          darkenFkeys((getSetting(SETTING_ENABLE_LOCK) == 1 ? 6 : 5));
        }
      }
    } else if(username) {
      color_t c = getSetting(SETTING_THEME) ? COLOR_BLACK : COLOR_WHITE;
      int textX = 0, textY = LCD_HEIGHT_PX - 58;
      PrintMini(&textX, &textY, username, 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, c, 0, 0);
      textX = LCD_WIDTH_PX - textX;
      PrintMini(&textX, &textY, username, 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, c, 1, 0);
      textX = 0; textY += 17;
      PrintMini(&textX, &textY, organization, 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, c, 0, 0);
      textX = LCD_WIDTH_PX - textX;
      PrintMini(&textX, &textY, organization, 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, c, 1, 0);
    }
    if(!pane_keycache) {
      Bdisp_PutDisp_DD();
      checkChronoComplete(getChronoArrayPtr());
    }
    GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key); //this is here just to handle the Menu key
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
        case KEY_CTRL_F6:
          key = KEY_PRGM_F6; break;
      }
      pane_keycache = 0;
    }
    if(key == KEY_PRGM_SHIFT) {
      //turn shift on/off manually because getkeywait doesn't do it
      SetSetupSetting( (unsigned int)0x14, (GetSetupSetting( (unsigned int)0x14) == 0));
    }
    if(isLocked) {
      if(key == KEY_PRGM_ALPHA) return;
    } else {
      switch (key) {
        case KEY_PRGM_MENU:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            saveVRAMandCallSettings();
          }
          break;
        case KEY_PRGM_F1:
          powerMenu(&pane_keycache);
          break;
        case KEY_PRGM_F2:
          lightMenu(&pane_keycache);
          break;
        case KEY_PRGM_F3:
          timeMenu(&pane_keycache);
          break;
        case KEY_PRGM_F4:
          toolsMenu(&pane_keycache);
          break;
        case KEY_PRGM_F5:
          memsysMenu(&pane_keycache);
          break;
        case KEY_PRGM_RETURN:
          if(!getSetting(SETTING_LOCK_ON_EXE)) break;
          // else fallthrough
        case KEY_PRGM_F6:
          lockApp();
          break;
        case 71: //KEY_PRGM_0, which is not defined in the standard libfxcg headers
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            char code[25] = "";
            textInput input;
            input.charlimit=21;
            input.buffer = (char*)code;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_CONFIRM && !strcmp(code, "qazedcol"))
              return; // kill main interface; currently this opens masterControl.
          }
          break;
        case KEY_PRGM_UP:
        case KEY_PRGM_RIGHT:
        case KEY_PRGM_DOWN:
        case KEY_PRGM_LEFT:
          handleHomePane(key, &pane_keycache);
          break;
        case 76: //x-0-theta key
          currentTimeToBasicVar();
          break;
      }
    }
    if (key && key!=KEY_PRGM_SHIFT) SetSetupSetting( (unsigned int)0x14, 0);
  }
}

void powerMenu(int* pane_keycache) {
  drawFkeyPopup(0, (char*)"Power options");
  
  MenuItem menuitems[5];
  menuitems[0].text = (char*)"Auto power off";
  menuitems[1].text = (char*)"Backlight duration";
  menuitems[2].text = (char*)"Backlight level";
  menuitems[3].text = (char*)"Power information";
  
  Menu menu;
  menu.items=menuitems;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  menu.numitems=4;
  menu.darken=getSetting(SETTING_THEME);
  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: setPoweroffTimeout(); break;
          case 2: setBacklightTimeout(); break;
          case 3: setBacklightLevel(); break;
          case 4: powerInformation(); break;
        }
        return;
      case KEY_CTRL_F6:
        if(!getSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

void lightMenu(int* pane_keycache) {
  drawFkeyPopup(1, (char*)"Light tools");
  
  MenuItem menuitems[4];
  menuitems[0].text = (char*)"Lantern";
  menuitems[1].text = (char*)"Flashlight";
  menuitems[2].text = (char*)"Morse light";
  menuitems[3].text = (char*)"Color light";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=4;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=getSetting(SETTING_THEME);
  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: lantern(); break;
          case 2: flashLight(); break;
          case 3: morseLight(); break;
          case 4: colorLight(); break;
        }
        return;
      case KEY_CTRL_F6:
        if(!getSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F1:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

void timeMenu(int* pane_keycache) {
  drawFkeyPopup(2, (char*)"Time tools");
  
  MenuItem menuitems[3];
  menuitems[0].text = (char*)"Calendar";
  menuitems[1].text = (char*)"Tasks";
  menuitems[2].text = (char*)"Chronometer";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=3;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=getSetting(SETTING_THEME);
  
  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: viewCalendar(); break;
          case 2: viewTasks(); break;
          case 3: chronoScreen(); break;
        }
        return;
      case KEY_CTRL_F6:
        if(!getSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

void toolsMenu(int* pane_keycache) {
  drawFkeyPopup(3, (char*)"Tools");
  
  MenuItem menuitems[3];
  menuitems[0].text = (char*)"Balance manager";
  menuitems[1].text = (char*)"Password generator";
  menuitems[2].text = (char*)"TOTP authenticator";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=3;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=getSetting(SETTING_THEME);

  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: balanceManager(); break;
          case 2: passwordGenerator(); break;
          case 3: totpClient(); break;
        }
        return;
      case KEY_CTRL_F6:
        if(!getSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F5:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

void memsysMenu(int* pane_keycache) {
  drawFkeyPopup(4, (char*)"Memory & System");
  
  MenuItem menuitems[5];
  menuitems[0].text = (char*)"File manager";
  menuitems[1].text = (char*)"Memory usage";
  menuitems[2].text = (char*)"Function key color";
  menuitems[3].text = (char*)"System information";
  menuitems[4].text = (char*)"Add-In manager";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems = 4 + !!getSetting(SETTING_SHOW_ADVANCED);
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=getSetting(SETTING_THEME);

  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: fileManager(); break;
          case 2: memoryCapacityScreen(); break;
          case 3: changeFKeyColor(); break;
          case 4: systemInfo(); break;
          case 5: addinManager(); break;
        }
        return;
      case KEY_CTRL_F6:
        if(!getSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

void handleHomePane(int key, int* pane_keycache) {
  int ptype, retkey;
  switch(key) {
    case KEY_PRGM_UP:    retkey = KEY_CTRL_DOWN; ptype = getSetting(SETTING_HOME_PANE_TOP); break;
    case KEY_PRGM_RIGHT: retkey = KEY_CTRL_LEFT; ptype = getSetting(SETTING_HOME_PANE_RIGHT); break;
    case KEY_PRGM_DOWN:  retkey = KEY_CTRL_UP; ptype = getSetting(SETTING_HOME_PANE_BOTTOM); break;
    default:             retkey = KEY_CTRL_RIGHT; ptype = getSetting(SETTING_HOME_PANE_LEFT); break;
  }
  if(ptype > 2) DrawFrame(COLOR_WHITE);
  switch(ptype) {
    case 0: return;
    case 1: eventsPane(retkey, pane_keycache); break;
    case 2: memoryUsagePane(retkey, pane_keycache); break;
    case 3: openRunMat(); break;
    case 4: fileManager(); break;
    case 5: viewCalendar(); break;
    case 6: viewTasks(); break;
    case 7: chronoScreen(); break;
    case 8: balanceManager(); break;
    case 9: passwordGenerator(); break;
    case 11:
      if(getSetting(SETTING_SHOW_ADVANCED)) {
        addinManager(); break;
      } //else deliberate fallthrough
    case 10: totpClient(); break;
  }
}

void paneHandleBasicKeys(int retkey, int* pane_keycache) {
  while (1) {
    if (getSetting(SETTING_THEME)) DrawFrame(COLOR_BLACK);
    int key;
    mGetKey(&key, getSetting(SETTING_THEME));
    switch(key) {
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
        *pane_keycache = key;
      case KEY_CTRL_EXIT:
        return; //return to the main pane
    }
    if(key == retkey) return; //return to the main pane
  }
}

void pane_drawTodayEvents(CalendarEvent* calevents, int startx, int starty, int numevents,
                          int maxevents) {
  color_t color_fg, color_title;
  if (getSetting(SETTING_THEME)) { color_fg = COLOR_WHITE; color_title = COLOR_ORANGE; } else
  { color_fg = COLOR_BLACK; color_title = COLOR_BLUE; }

  if (numevents>0) {
    int curevent = 0; //current processing event
    char itemtext[25];
    multiPrintMini(startx, starty, "Events starting today", color_title);
    while(curevent < numevents && curevent < maxevents) {
      starty += 18;
      strcpy(itemtext, "- ");
      strcat(itemtext, (char*)calevents[curevent].title);
      int tcolor = calevents[curevent].category-1;
      if (getSetting(SETTING_THEME) && tcolor == TEXT_COLOR_BLACK) tcolor = TEXT_COLOR_WHITE;
      color_t tfcolor = textColorToFullColor(tcolor);
      multiPrintMini(startx + 5, starty, itemtext, tfcolor);
      curevent++;
    }
    if(numevents>maxevents) {
      sprintf(itemtext, "...and %d more event%s", numevents-maxevents,
              numevents-maxevents > 1 ? "s" : "");
      multiPrintMini(startx + 5, starty + 20, itemtext, color_fg);
    }
  } else {
    multiPrintMini(startx, starty, "  No events starting today", color_fg);
  } 
}
#define HOME_EVENTS_DISPLAY_FULL 6
void eventsPane(int retkey, int* pane_keycache) {
  EventDate thisday;
  thisday.day = getCurrentDay(); thisday.month = getCurrentMonth(); thisday.year = getCurrentYear();
  int numevents = getEvents(&thisday, CALENDARFOLDER, NULL); //get event count
  CalendarEvent* calevents = NULL;
  if (numevents > 0) {
    int getnum = (numevents>HOME_EVENTS_DISPLAY_FULL?HOME_EVENTS_DISPLAY_FULL:numevents);
    // ^ number of events to parse. allocate memory only for the few events we want to display
    calevents = (CalendarEvent*)alloca(getnum*sizeof(CalendarEvent));
    getEvents(&thisday, CALENDARFOLDER, calevents, HOME_EVENTS_DISPLAY_FULL);
  }
  if (getSetting(SETTING_THEME)) {
    Bdisp_Fill_VRAM( COLOR_BLACK, 2 ); //fill between the status area and f-key area
    DrawFrame(COLOR_BLACK);
  } else {
    Bdisp_Fill_VRAM( COLOR_WHITE, 2 ); //fill between the status area and f-key area
    DrawFrame(COLOR_WHITE);
  }
  pane_drawTodayEvents(calevents, 0, 0, numevents, HOME_EVENTS_DISPLAY_FULL);
  if(getSetting(SETTING_SHOW_CALENDAR_BUSY_MAP))
    drawBusymapDay(&thisday, 0, LCD_HEIGHT_PX-44, LCD_WIDTH_PX, 15, 1,0,0);
  paneHandleBasicKeys(retkey, pane_keycache);
}

void memoryUsagePane(int retkey, int* pane_keycache) {
  if (getSetting(SETTING_THEME)) DrawFrame(COLOR_BLACK);
  else DrawFrame(COLOR_WHITE);
  Bdisp_Fill_VRAM( COLOR_WHITE, 2 ); //fill between the status area and f-key area
  memoryCapacityScreen(-24);
  if(getSetting(SETTING_THEME)) {
    replaceColorInArea(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24*2-1, COLOR_BLACK, COLOR_CYAN);
    replaceColorInArea(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24*2-1, COLOR_WHITE, COLOR_BLACK);
    replaceColorInArea(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24*2-1, COLOR_CYAN, COLOR_WHITE);
  }
  paneHandleBasicKeys(retkey, pane_keycache);
}