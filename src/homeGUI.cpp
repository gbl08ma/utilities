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
#include "editorGUI.hpp"
#include "debugGUI.hpp"

void showHome(chronometer* chrono) {
  unsigned short key = 0;
  unsigned short prevkey = 0;
  int keyCol; int keyRow; //these aren't actually used, but they are needed to hold different getkey-like results
  int pane_keycache = 0;
  while (1) {
    //black theme, or not?
    if (GetSetting(SETTING_THEME) == 1) {
      Bdisp_Fill_VRAM( COLOR_BLACK, 3 );
      DrawFrame( 0x000000  );
    } else {
      Bdisp_AllClr_VRAM();
      DrawFrame( 0xFFFFFF  );
    }
    if(GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE) == 3 && getLastChronoComplete()) {
      char buffer[10] = "";
      itoa(getLastChronoComplete(), (unsigned char*)buffer);
      char message[50];
      strcpy(message, "Chronometer ");
      strcat(message, buffer);
      strcat(message, " ended");
      DefineStatusMessage(message, 1, 4, 0);
    } else DefineStatusMessage((char*)"", 1, 0, 0);
    DisplayStatusArea();
    
    // Print time
    drawHomeClock(GetSetting(SETTING_CLOCK_TYPE));
    if (GetSetting(SETTING_THEME)) darkenStatusbar();

    //Show FKeys
    if (GetSetting(SETTING_DISPLAY_FKEYS)) {
      drawFkeyLabels(0x043A, 0x043E, 0x012A, 0x011C, (GetSetting(SETTING_ENABLE_LOCK) ? 0x04D3 : -1)); //POWER, LIGHT, TIME, TOOL, key icon (lock)
      if (GetSetting(SETTING_THEME)) {
        darkenFkeys((GetSetting(SETTING_ENABLE_LOCK) == 1 ? 5 : 4));
      }
    }
    if(!pane_keycache) {
      Bdisp_PutDisp_DD();
      checkDownwardsChronoCompleteGUI(chrono, NUMBER_OF_CHRONO);
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
      }
      pane_keycache = 0;
    }
    switch (key) {
      case KEY_PRGM_SHIFT:
        //turn on/off shift manually because getkeywait doesn't do it
        SetSetupSetting( (unsigned int)0x14, (GetSetupSetting( (unsigned int)0x14) == 0));
        break;
      case KEY_PRGM_MENU:
        if (GetSetupSetting( (unsigned int)0x14) == 1) {
          SetSetupSetting( (unsigned int)0x14, 0);
          saveVRAMandCallSettings();
        }
        break;
      case KEY_PRGM_ACON:
        if (GetSetupSetting( (unsigned int)0x14) == 1) {
          // following the suspicion that calling PowerOff after GetKeyWait was running
          // may cause problems (or that it isn't even related to GetKeyWait),
          // users now have to manually press Shift+AC twice to turn off the calculator.
          // (the second time is for GetKey)
          SetSetupSetting( (unsigned int)0x14, 0);
          DisplayStatusArea();
          mMsgBoxPush(4);
          mPrintXY(3, 2, (char*)"To turn off,", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 3, (char*)"repeat what you", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 4, (char*)"just did:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 5, (char*)"Press Shift...", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          int gkey;
          GetKey(&gkey);
          mMsgBoxPop();
          if(gkey!=KEY_CTRL_SHIFT) continue;
          mMsgBoxPush(4);
          mPrintXY(3, 2, (char*)"...now press", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 3, (char*)"AC/ON.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          GetKey(&gkey);
          mMsgBoxPop();
        }
        break;
      case KEY_PRGM_F1:
        powerMenu(&pane_keycache);
        break;
      case KEY_PRGM_F2:
        lightMenu(&pane_keycache);
        break;
      case KEY_PRGM_F3:
        timeMenu(chrono, &pane_keycache);
        break;
      case KEY_PRGM_F4:
        toolsMenu(&pane_keycache);
        break;
      case KEY_PRGM_RETURN:
        if(!GetSetting(SETTING_LOCK_ON_EXE)) break;
        // else fallthrough
      case KEY_PRGM_F5:
        lockApp();
        break;
      case KEY_PRGM_F6:
        break;
      case 71: //KEY_PRGM_0, which is not defined in the SDK and I'm too lazy to add it every time I update the includes folder...
        if (GetSetupSetting( (unsigned int)0x14) == 1) {
          SetSetupSetting( (unsigned int)0x14, 0);
          char code[25] = "";
          textInput input;
          input.y=8;
          input.charlimit=21;
          input.buffer = (char*)code;
          int res = doTextInput(&input);
          if (res==INPUT_RETURN_CONFIRM && !strcmp(code, "qazedcol")) masterControl();
        }
        break;
      case KEY_PRGM_RIGHT:
        if(GetSetting(SETTING_HOME_PANES)) eventsPane(&pane_keycache);
        break;
      case 76: //x-0-theta key
        currentTimeToBasicVar();
        break;
    }
    if (key!=prevkey && key!=KEY_PRGM_SHIFT) SetSetupSetting( (unsigned int)0x14, 0);
  }
}

inline void powerMenu(int* pane_keycache) {
  drawFkeyPopup(0, (char*)"Power options");
  
  MenuItem menuitems[5];
  menuitems[0].text = (char*)"Auto power off";
  menuitems[1].text = (char*)"Backlight duration";
  menuitems[2].text = (char*)"Backlight level";
  menuitems[3].text = (char*)"Power information";
  menuitems[4].text = (char*)"CPU speed";
  
  Menu menu;
  menu.items=menuitems;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  if(GetSetting(SETTING_SHOW_ADVANCED)) {
    menu.numitems=5;
  } else {
    menu.numitems=4;
    menu.scrollbar=0;
  }
  menu.darken=GetSetting(SETTING_THEME);
  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: changePoweroffTimeout(); break;
          case 2: changeBacklightTimeout(); break;
          case 3: changeBacklightLevel(); break;
          case 4: powerInformation(); break;
          case 5: setCPUclock(); break;
        }
        return;
      case KEY_CTRL_F5:
        if(!GetSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

inline void lightMenu(int* pane_keycache) {
  drawFkeyPopup(1, (char*)"Light tools");
  
  MenuItem menuitems[5];
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
  menu.scrollbar=0;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=GetSetting(SETTING_THEME);
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
      case KEY_CTRL_F5:
        if(!GetSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F1:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

inline void timeMenu(chronometer* chrono, int* pane_keycache) {
  drawFkeyPopup(2, (char*)"Time tools");
  
  MenuItem menuitems[5];
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
  menu.scrollbar=0;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=GetSetting(SETTING_THEME);
  
  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: viewCalendar(); break;
          case 2: viewTasks(); break;
          case 3: chronoScreen(chrono); break;
        }
        return;
      case KEY_CTRL_F5:
        if(!GetSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F4:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}

inline void toolsMenu(int* pane_keycache) {
  drawFkeyPopup(3, (char*)"Tools");
  
  MenuItem menuitems[6];
  menuitems[0].text = (char*)"File manager";
  menuitems[1].text = (char*)"Memory usage";
  menuitems[2].text = (char*)"Add-In manager";
  menuitems[3].text = (char*)"Function key color";
  menuitems[4].text = (char*)"System information";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=5;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=GetSetting(SETTING_THEME);

  while(1) {
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        DrawFrame(COLOR_WHITE);
        switch(menu.selection) {
          case 1: fileManager(); break;
          case 2: memoryCapacityViewer(); break;
          case 3: addinManager(); break;
          case 4: changeFKeyColor(); break;
          case 5: systemInfo(); break;
        }
        return;
      case KEY_CTRL_F5:
        if(!GetSetting(SETTING_ENABLE_LOCK)) break; // else keep on
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
        *pane_keycache = res;
      case MENU_RETURN_EXIT:
        return;
    }
  }
}
inline void pane_drawTodayEvents(CalendarEvent* calevents, int startx, int starty, int numevents, int maxevents) {
  color_t color_fg, color_bg, color_title;
  if (GetSetting(SETTING_THEME)) { color_fg = COLOR_WHITE; color_bg = COLOR_BLACK; color_title = COLOR_ORANGE; } else
  { color_fg = COLOR_BLACK; color_bg = COLOR_WHITE; color_title = COLOR_BLUE; }
  
  int textX = startx;
  int textY = starty;
  if (numevents>0) {
    int curevent = 0; //current processing event
    unsigned char itemtext[25];
    PrintMini(&textX, &textY, (unsigned char*)"Events starting today", 0, 0xFFFFFFFF, 0, 0, color_title, color_bg, 1, 0); //draw
    while(curevent < numevents && curevent < maxevents) {
      textX = startx + 5;
      textY = textY + 18;
      strcpy((char*)itemtext, "- ");
      strcat((char*)itemtext, (char*)calevents[curevent].title);
      int tcolor = calevents[curevent].category-1;
      if (GetSetting(SETTING_THEME) && tcolor == TEXT_COLOR_BLACK) tcolor = TEXT_COLOR_WHITE;
      color_t tfcolor = textColorToFullColor(tcolor);
      PrintMini(&textX, &textY, (unsigned char*)itemtext, 0, 0xFFFFFFFF, 0, 0, tfcolor, color_bg, 1, 0); //draw
      curevent++;
    }
    if(numevents>maxevents) {
      textX = startx + 5;
      textY = textY + 20;
      strcpy((char*)itemtext, (char*)"  ...and ");
      unsigned char buffer[10] = "";
      itoa(numevents-maxevents, buffer);
      strcat((char*)itemtext, (char*)buffer);
      strcat((char*)itemtext, (char*)" more event");
      if(numevents-maxevents > 1) strcat((char*)itemtext, (char*)"s");
      PrintMini(&textX, &textY, (unsigned char*)itemtext, 0, 0xFFFFFFFF, 0, 0, color_fg, color_bg, 1, 0); //draw
    }
  } else {
    PrintMini(&textX, &textY, (unsigned char*)"  No events starting today", 0, 0xFFFFFFFF, 0, 0, color_fg, color_bg, 1, 0); //draw
  } 
}
#define HOME_EVENTS_DISPLAY_FULL 6
void eventsPane(int* pane_keycache) {
  int key;
  EventDate thisday;
  thisday.day = getCurrentDay(); thisday.month = getCurrentMonth(); thisday.year = getCurrentYear();
  int numevents = GetEventsForDate(&thisday, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* calevents = NULL;
  if (numevents > 0) {
    int getnum = (numevents>HOME_EVENTS_DISPLAY_FULL?HOME_EVENTS_DISPLAY_FULL:numevents); // number of events to parse.
    calevents = (CalendarEvent*)alloca(getnum*sizeof(CalendarEvent)); // we don't want to allocate more than what we need for the few events we want to display in full.
    GetEventsForDate(&thisday, CALENDARFOLDER, calevents, HOME_EVENTS_DISPLAY_FULL);
  }
  int inscreen = 1;
  if (GetSetting(SETTING_THEME)) {
    Bdisp_Fill_VRAM( COLOR_BLACK, 2 ); //fill between the status area and f-key area
    DrawFrame( 0x000000  );
  } else {
    Bdisp_Fill_VRAM( COLOR_WHITE, 2 ); //fill between the status area and f-key area
    DrawFrame( 0xfffff  );
  }
  pane_drawTodayEvents(calevents, 0, 0, numevents, HOME_EVENTS_DISPLAY_FULL);
  if(GetSetting(SETTING_SHOW_CALENDAR_BUSY_MAP)) drawDayBusyMap(&thisday, 0, LCD_HEIGHT_PX-44, LCD_WIDTH_PX, 15, 1,0,0);
  while (inscreen) {
    if (GetSetting(SETTING_THEME)) DrawFrame( 0x000000  );
    mGetKey(&key, GetSetting(SETTING_THEME));
    switch(key) {
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
        *pane_keycache = key;
      case KEY_CTRL_LEFT:
      case KEY_CTRL_EXIT:
        return; //return to the pane to the left (main)
    }
  }
}