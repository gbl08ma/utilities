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

#include "settingsGUI.hpp"
#include "menuGUI.hpp"
#include "selectorGUI.hpp"
#include "aboutGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp" 
#include "timeGUI.hpp"
#include "lockGUI.hpp"

inline static void lockSettingsMenu();
inline static void clockSettingsMenu();

void settingsMenu() {
  MenuItem menuitems[15];
  strcpy(menuitems[0].text, "Set time");
  
  strcpy(menuitems[1].text, "Set date");
  
  strcpy(menuitems[2].text, "Time format");
  
  strcpy(menuitems[3].text, "Long date format");
  
  strcpy(menuitems[4].text, "Date format");
  
  strcpy(menuitems[5].text, "Home appearance");
  
  strcpy(menuitems[6].text, "Display statusbar");
  menuitems[6].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[7].text, "Show advanced tools");
  menuitems[7].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[8].text, "Startup brightness");
  
  strcpy(menuitems[9].text, "Calc. lock settings");
  
  strcpy(menuitems[10].text, "Calendar settings");
  
  strcpy(menuitems[11].text, "Chrono. notification");

  strcpy(menuitems[12].text, "About this add-in");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=13;
  menu.scrollout=1;
  setmGetKeyMode(MGETKEY_MODE_RESTRICT_SETTINGS);
  while(1) {
    menuitems[6].value = GetSetting(SETTING_DISPLAY_STATUSBAR);
    menuitems[7].value = GetSetting(SETTING_SHOW_ADVANCED);
    
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) {
      setmGetKeyMode(MGETKEY_MODE_NORMAL);
      return;
    }
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1:
          setTimeGUI();
          break;
        case 2:
          setDateGUI();
          break;
        case 3: // set time format
        { MenuItem menuitems[5];
          strcpy(menuitems[0].text, "24-hour: ");
          currentTimeToString(menuitems[0].text, 0);
          strcpy(menuitems[1].text, "12-hour: ");
          currentTimeToString(menuitems[1].text, 1);
          
          Menu menu;
          menu.items=menuitems;
          menu.numitems=2;
          menu.showtitle=1;
          menu.scrollbar=0;
          menu.width=22;
          menu.selection=GetSetting(SETTING_TIMEFORMAT)+1;
          strcpy(menu.title, "Set time format");
          int res = doMenu(&menu);
          if(res==MENU_RETURN_SELECTION) SetSetting(SETTING_TIMEFORMAT, menu.selection-1, 1);
          break;
        }
        case 4: // set long date format
        { Selector format;
          strcpy(format.title, "Set long date format");
          strcpy(format.subtitle, "");
          format.value = GetSetting(SETTING_LONGDATEFORMAT);
          format.min = 0;
          format.max = 9;
          format.type = SELECTORTYPE_LONGDATEFORMAT;
          int res = doSelector(&format);
          if (res == SELECTOR_RETURN_EXIT) continue;
          SetSetting(SETTING_LONGDATEFORMAT, format.value, 1);
          break;
        }
        case 5: // set date format
        { MenuItem menuitems[5];
          strcpy(menuitems[0].text, "");
          currentDateToString(menuitems[0].text, 0);
          strcpy(menuitems[1].text, "");
          currentDateToString(menuitems[1].text, 1);
          strcpy(menuitems[2].text, "");
          currentDateToString(menuitems[2].text, 2);
          
          Menu menu;
          menu.items=menuitems;
          menu.numitems=3;
          menu.showtitle=1;
          menu.scrollbar=0;
          menu.width=22;
          menu.selection=GetSetting(SETTING_DATEFORMAT)+1;
          strcpy(menu.title, "Set date format");
          int res = doMenu(&menu);
          if(res==MENU_RETURN_SELECTION) SetSetting(SETTING_DATEFORMAT, menu.selection-1, 1);
          break;
        }
        case 6:
          clockSettingsMenu();
          break;
        case 7:
          menuitems[6].value = !menuitems[6].value;
          SetSetting(SETTING_DISPLAY_STATUSBAR, menuitems[6].value, 1);
          break;
        case 8:
          menuitems[7].value = !menuitems[7].value;
          SetSetting(SETTING_SHOW_ADVANCED, menuitems[7].value, 1); 
          break;
        case 9: // set startup brightness
        { Selector sel;
          strcpy(sel.title, "Set start brightness");
          strcpy(sel.subtitle, "");
          sel.value = GetSetting(SETTING_STARTUP_BRIGHTNESS);
          sel.min = 1;
          sel.max = 250;
          sel.type = SELECTORTYPE_STARTUP_BRIGHTNESS;
          int res = doSelector(&sel);
          if (res == SELECTOR_RETURN_EXIT) continue;
          SetSetting(SETTING_STARTUP_BRIGHTNESS, sel.value, 1);
          break;
        }
        case 10:
          lockSettingsMenu();
          break;
        case 11:
          calendarSettingsMenu();
          break;
        case 12:
        { mMsgBoxPush(5);
          MenuItem smallmenuitems[3];
          strcpy(smallmenuitems[0].text, "No notification");        
          strcpy(smallmenuitems[1].text, "Flashing pop-up");        
          strcpy(smallmenuitems[2].text, "Simple pop-up");
          strcpy(smallmenuitems[3].text, "Note on home");
          
          Menu smallmenu;
          smallmenu.items=smallmenuitems;
          smallmenu.numitems=4;
          smallmenu.width=17;
          smallmenu.height=5;
          smallmenu.startX=3;
          smallmenu.startY=2;
          smallmenu.scrollbar=0;
          smallmenu.showtitle=1;
          smallmenu.selection=GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE)+1;
          strcpy(smallmenu.title, "Chrono. notif.");

          int sres = doMenu(&smallmenu);
          if(sres == MENU_RETURN_SELECTION) SetSetting(SETTING_CHRONO_NOTIFICATION_TYPE, smallmenu.selection-1, 1);
          mMsgBoxPop();
          break;
        }
        case 13:
          showAbout();
          break;
      }
    }
  }
}

inline static void lockSettingsMenu() {
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Set lock code");  
  
  strcpy(menuitems[1].text, "Show last code char");
  menuitems[1].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[2].text, "Off after locking");
  menuitems[2].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[3].text, "Lock on [EXE]");
  menuitems[3].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[4].text, "Run-Mat on unlock");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=5;
  menu.scrollout=1;
  while(1) {
    menuitems[1].value = GetSetting(SETTING_PASSWORD_PRIVACY);
    menuitems[2].value = GetSetting(SETTING_LOCK_AUTOOFF);
    menuitems[3].value = GetSetting(SETTING_LOCK_ON_EXE);
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1:
          setPassword();
          break;
        case 2:
          menuitems[1].value = !menuitems[1].value;
          SetSetting(SETTING_PASSWORD_PRIVACY, menuitems[1].value, 1); 
          break;
        case 3:
          menuitems[2].value = !menuitems[2].value;
          SetSetting(SETTING_LOCK_AUTOOFF, menuitems[2].value, 1);
          break;
        case 4:
          menuitems[3].value = !menuitems[3].value;
          SetSetting(SETTING_LOCK_ON_EXE, menuitems[3].value, 1);
          break;
        case 5: {
          mMsgBoxPush(4);
          MenuItem smallmenuitems[3];
          strcpy(smallmenuitems[0].text, "Off");        
          strcpy(smallmenuitems[1].text, "On");        
          strcpy(smallmenuitems[2].text, "Ask");
          
          Menu smallmenu;
          smallmenu.items=smallmenuitems;
          smallmenu.numitems=3;
          smallmenu.width=17;
          smallmenu.height=4;
          smallmenu.startX=3;
          smallmenu.startY=2;
          smallmenu.scrollbar=0;
          smallmenu.showtitle=1;
          smallmenu.selection=GetSetting(SETTING_UNLOCK_RUNMAT)+1;
          strcpy(smallmenu.title, "Run-Mat on unlock");
          int sres = doMenu(&smallmenu);
          if(sres == MENU_RETURN_SELECTION) {
            SetSetting(SETTING_UNLOCK_RUNMAT, smallmenu.selection-1, 1);
          }
          mMsgBoxPop();
          break;
        }
      }
    }
  }
}

inline static void clockSettingsMenu() {
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Set clock type");
  
  strcpy(menuitems[1].text, "Show seconds");
  menuitems[1].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[2].text, "Enable events pane");
  menuitems[2].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[3].text, "Show F. keys labels");
  menuitems[3].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[4].text, "Dark theme");
  menuitems[4].type = MENUITEM_CHECKBOX;
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=5;
  menu.scrollout=1;
  while(1) {
    menuitems[1].value = GetSetting(SETTING_CLOCK_SECONDS);
    menuitems[2].value = GetSetting(SETTING_HOME_PANES);
    menuitems[3].value = GetSetting(SETTING_DISPLAY_FKEYS);
    menuitems[4].value = GetSetting(SETTING_THEME);
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1: {
          int inscreen = 1;
          int cur = GetSetting(SETTING_CLOCK_TYPE);
          while(inscreen) {
            Bdisp_AllClr_VRAM();
            DisplayStatusArea();
            int key;
            drawHomeClock(cur, 0); // force white theme
            int textX=0, textY=7*24;
            PrintMini(&textX, &textY, (unsigned char*)"\xe6\x92/\xe6\x93: select; EXE: confirm; EXIT: cancel", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
            GetKey(&key);
            switch(key) {
              case KEY_CTRL_UP:
                cur++;
                if(cur>11) cur = 0;
                break;
              case KEY_CTRL_DOWN:
                cur--;
                if(cur<0) cur = 11;
                break;
              case KEY_CTRL_EXE:
                SetSetting(SETTING_CLOCK_TYPE, cur, 1); 
                //deliberate fallthrough
              case KEY_CTRL_EXIT:
                inscreen = 0;
                break;
            }
          }
          break;
        }
        case 2:
          menuitems[1].value = !menuitems[1].value;
          SetSetting(SETTING_CLOCK_SECONDS, menuitems[1].value, 1);
          break;
        case 3:
          menuitems[2].value = !menuitems[2].value;
          SetSetting(SETTING_HOME_PANES, menuitems[2].value, 1);
          break;
        case 4:
          menuitems[3].value = !menuitems[3].value;
          SetSetting(SETTING_DISPLAY_FKEYS, menuitems[3].value, 1);
          break;
        case 5:
          menuitems[4].value = !menuitems[4].value;
          SetSetting(SETTING_THEME, menuitems[4].value, 1); 
          break;
      }
    }
  }
}

void calendarSettingsMenu() {
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Show events count");
  menuitems[0].type = MENUITEM_CHECKBOX;
  
  strcpy(menuitems[1].text, "Default calendar view");
  
  strcpy(menuitems[2].text, "First day of week");
  
  strcpy(menuitems[3].text, "Show busy timetables");
  menuitems[3].type = MENUITEM_CHECKBOX;
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=4;
  menu.scrollout=1;
  while(1) {
    menuitems[0].value = GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT);
    menuitems[3].value = GetSetting(SETTING_SHOW_CALENDAR_BUSY_MAP);
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1:
          menuitems[0].value = !menuitems[0].value;
          SetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT, menuitems[0].value, 1);
          break;
        case 2:
        case 3: {
          mMsgBoxPush(4);
          MenuItem smallmenuitems[3];
          
          Menu smallmenu;
          smallmenu.items=smallmenuitems;
          smallmenu.numitems=2;
          smallmenu.width=17;
          smallmenu.height=4;
          smallmenu.startX=3;
          smallmenu.startY=2;
          smallmenu.scrollbar=0;
          smallmenu.showtitle=1;
          if(menu.selection == 2) {
            strcpy(smallmenu.title, "Default Calendar");
            smallmenu.selection=GetSetting(SETTING_DEFAULT_CALENDAR_VIEW)+1;
            strcpy(smallmenuitems[0].text, "Week");
            strcpy(smallmenuitems[1].text, "Month");
          } else {
            strcpy(smallmenu.title, "Week starts on");
            smallmenu.selection=GetSetting(SETTING_WEEK_START_DAY)+1;
            strcpy(smallmenuitems[0].text, getDOWAsString(1)); // Sunday
            strcpy(smallmenuitems[1].text, getDOWAsString(2)); // Monday
          }
          int sres = doMenu(&smallmenu);
          if(sres == MENU_RETURN_SELECTION)
            SetSetting((menu.selection == 2 ? SETTING_DEFAULT_CALENDAR_VIEW : SETTING_WEEK_START_DAY), smallmenu.selection-1, 1);
          mMsgBoxPop();
          break;
        }
        case 4:
          menuitems[3].value = !menuitems[3].value;
          SetSetting(SETTING_SHOW_CALENDAR_BUSY_MAP, menuitems[3].value, 1);
          break;
      }
    }
  }
}