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
  menuitems[0].text = (char*)"Set time";
  
  menuitems[1].text = (char*)"Set date";
  
  menuitems[2].text = (char*)"Time format";
  
  menuitems[3].text = (char*)"Long date format";
  
  menuitems[4].text = (char*)"Date format";
  
  menuitems[5].text = (char*)"Home settings";
  
  menuitems[6].text = (char*)"Display statusbar";
  menuitems[6].type = MENUITEM_CHECKBOX;
  
  menuitems[7].text = (char*)"Show advanced tools";
  menuitems[7].type = MENUITEM_CHECKBOX;
  
  menuitems[8].text = (char*)"Startup brightness";
  
  menuitems[9].text = (char*)"Calc. lock settings";
  
  menuitems[10].text = (char*)"Calendar settings";
  
  menuitems[11].text = (char*)"Chrono. notification";

  menuitems[12].text = (char*)"About this add-in";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=13;
  menu.scrollout=1;
  menu.statusText = (char*)"";
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
          char firstitem[21];
          strcpy(firstitem, "24-hour: ");
          currentTimeToString(firstitem, 0);
          menuitems[0].text = firstitem;
          char seconditem[21];
          strcpy(seconditem, "12-hour: ");
          currentTimeToString(seconditem, 1);
          menuitems[1].text = seconditem;
          
          Menu menu;
          menu.items=menuitems;
          menu.numitems=2;
          menu.scrollbar=0;
          menu.width=22;
          menu.selection=GetSetting(SETTING_TIMEFORMAT)+1;
          menu.title = (char*)"Set time format";
          int res = doMenu(&menu);
          if(res==MENU_RETURN_SELECTION) SetSetting(SETTING_TIMEFORMAT, menu.selection-1, 1);
          break;
        }
        case 4: // set long date format
        { Selector format;
          format.title = (char*)"Set long date format";
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
          char items[3][21];
          for(int i = 0; i < 3; i++) {
            currentDateToString(items[i], i);
            menuitems[i].text = items[i];
          }
          
          Menu menu;
          menu.items=menuitems;
          menu.numitems=3;
          menu.scrollbar=0;
          menu.width=22;
          menu.selection=GetSetting(SETTING_DATEFORMAT)+1;
          menu.title = (char*)"Set date format";
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
          sel.title = (char*)"Set start brightness";
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
          MenuItem smallmenuitems[4];
          smallmenuitems[0].text = (char*)"No notification";        
          smallmenuitems[1].text = (char*)"Flashing pop-up";        
          smallmenuitems[2].text = (char*)"Simple pop-up";
          smallmenuitems[3].text = (char*)"Note on home";
          
          Menu smallmenu;
          smallmenu.items=smallmenuitems;
          smallmenu.numitems=4;
          smallmenu.width=17;
          smallmenu.height=5;
          smallmenu.startX=3;
          smallmenu.startY=2;
          smallmenu.scrollbar=0;
          smallmenu.selection=GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE)+1;
          smallmenu.title = (char*)"Chrono. notif.";

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
  menuitems[0].text = (char*)"Set lock code";
  
  menuitems[1].text = (char*)"Show last code char";
  menuitems[1].type = MENUITEM_CHECKBOX;
  
  menuitems[2].text = (char*)"Off after locking";
  menuitems[2].type = MENUITEM_CHECKBOX;
  
  menuitems[3].text = (char*)"Lock on [EXE]";
  menuitems[3].type = MENUITEM_CHECKBOX;
  
  menuitems[4].text = (char*)"Run-Mat on unlock";
  
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
          smallmenuitems[0].text = (char*)"Off";
          smallmenuitems[1].text = (char*)"On";
          smallmenuitems[2].text = (char*)"Ask";
          
          Menu smallmenu;
          smallmenu.items=smallmenuitems;
          smallmenu.numitems=3;
          smallmenu.width=17;
          smallmenu.height=4;
          smallmenu.startX=3;
          smallmenu.startY=2;
          smallmenu.scrollbar=0;
          smallmenu.selection=GetSetting(SETTING_UNLOCK_RUNMAT)+1;
          smallmenu.title = (char*)"Run-Mat on unlock";
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

inline static int selectPaneType(int orig) {
  mMsgBoxPush(5);
  MenuItem smallmenuitems[10];
  
  Menu smallmenu;
  smallmenu.items=smallmenuitems;
  smallmenu.numitems=7;
  smallmenu.width=17;
  smallmenu.height=5;
  smallmenu.startX=3;
  smallmenu.startY=2;
  smallmenu.scrollout=1;
  smallmenu.title = (char*)"Select Panel Type";
  smallmenu.selection = orig + 1;
  smallmenuitems[0].text = (char*)"Disabled";
  smallmenuitems[1].text = (char*)"Today events";
  smallmenuitems[2].text = (char*)"Memory usage";
  smallmenuitems[3].text = (char*)"Run-Mat shortcut";
  smallmenuitems[4].text = (char*)"File mgr.shortcut";
  smallmenuitems[5].text = (char*)"Calendar shortcut";
  smallmenuitems[6].text = (char*)"Tasks shortcut";
  int sres = doMenu(&smallmenu);
  mMsgBoxPop();
  if(sres == MENU_RETURN_SELECTION) return smallmenu.selection - 1;
  else return -1;
}

inline static void clockSettingsMenu() {
  MenuItem menuitems[8];
  menuitems[0].text = (char*)"Set clock type";
  
  menuitems[1].text = (char*)"Show seconds";
  menuitems[1].type = MENUITEM_CHECKBOX;
  
  menuitems[2].text = (char*)"Show F. keys labels";
  menuitems[2].type = MENUITEM_CHECKBOX;
  
  menuitems[3].text = (char*)"Dark theme";
  menuitems[3].type = MENUITEM_CHECKBOX;

  menuitems[4].text = (char*)"Top home panel";
  menuitems[5].text = (char*)"Right home panel";
  menuitems[6].text = (char*)"Bottom home panel";
  menuitems[7].text = (char*)"Left home panel";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=8;
  menu.scrollout=1;
  while(1) {
    menuitems[1].value = GetSetting(SETTING_CLOCK_SECONDS);
    menuitems[2].value = GetSetting(SETTING_DISPLAY_FKEYS);
    menuitems[3].value = GetSetting(SETTING_THEME);
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1: {
          int inscreen = 1;
          int cur = GetSetting(SETTING_CLOCK_TYPE);
          while(inscreen) {
            Bdisp_AllClr_VRAM();
            int key;
            drawHomeClock(cur, 0); // force white theme
            int textX=0, textY=7*24;
            PrintMini(&textX, &textY, (char*)"\xe6\x92/\xe6\x93: select; EXE: confirm; EXIT: cancel", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
            mGetKey(&key);
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
          SetSetting(SETTING_DISPLAY_FKEYS, menuitems[2].value, 1);
          break;
        case 4:
          menuitems[3].value = !menuitems[3].value;
          SetSetting(SETTING_THEME, menuitems[3].value, 1); 
          break;
        case 5:
        case 6:
        case 7:
        case 8:
        {
          int s = menu.selection-5+25;
          int t = selectPaneType(GetSetting(s));
          if(t >= 0) SetSetting(s, t, 1);
          break;
        }
      }
    }
  }
}

void calendarSettingsMenu() {
  MenuItem menuitems[5];
  menuitems[0].text = (char*)"Show events count";
  menuitems[0].type = MENUITEM_CHECKBOX;
  
  menuitems[1].text = (char*)"Default calendar view";
  
  menuitems[2].text = (char*)"First day of week";
  
  menuitems[3].text = (char*)"Show busy timetables";
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
          if(menu.selection == 2) {
            smallmenu.title = (char*)"Default Calendar";
            smallmenu.selection=GetSetting(SETTING_DEFAULT_CALENDAR_VIEW)+1;
            smallmenuitems[0].text = (char*)"Week";
            smallmenuitems[1].text = (char*)"Month";
          } else {
            smallmenu.title = (char*)"Week starts on";
            smallmenu.selection=GetSetting(SETTING_WEEK_START_DAY)+1;
            smallmenuitems[0].text = (char*)getDOWAsString(1); // Sunday
            smallmenuitems[1].text = (char*)getDOWAsString(2); // Monday
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