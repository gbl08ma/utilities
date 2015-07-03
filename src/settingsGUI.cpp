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
inline static void homeSettingsMenu();

void settingsMenu() {
  DrawFrame(COLOR_WHITE);
  
  MenuItem menuitems[11];
  menuitems[0].text = (char*)"Set time";
  
  menuitems[1].text = (char*)"Set date";
  
  menuitems[2].text = (char*)"Time format";
  
  menuitems[3].text = (char*)"Date format";
  
  menuitems[4].text = (char*)"Home settings";

  menuitems[5].text = (char*)"Calendar settings";

  menuitems[6].text = (char*)"Calc. lock settings";

  menuitems[7].text = (char*)"Chrono. notification";
  
  menuitems[8].text = (char*)"Show advanced tools";
  menuitems[8].type = MENUITEM_CHECKBOX;
  
  menuitems[9].text = (char*)"Startup brightness";

  menuitems[10].text = (char*)"About this add-in";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=11;
  menu.scrollout=1;
  menu.statusText = (char*)"";
  setmGetKeyMode(MGETKEY_MODE_RESTRICT_SETTINGS);
  while(1) {
    menuitems[8].value = getSetting(SETTING_SHOW_ADVANCED);
    
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) {
      setmGetKeyMode(MGETKEY_MODE_NORMAL);
      return;
    }
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1:
          setTimeScreen();
          break;
        case 2:
          setDateScreen();
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
          menu.selection=getSetting(SETTING_TIMEFORMAT)+1;
          menu.title = (char*)"Set time format";
          int res = doMenu(&menu);
          if(res==MENU_RETURN_SELECTION) setSetting(SETTING_TIMEFORMAT, menu.selection-1, 1);
          break;
        }
        case 4: // set date format
        { MenuItem menuitems[5];
          char items[3][21];
          for(int i = 0; i < 3; i++) {
            currentDateToString(items[i], i);
            menuitems[i].text = items[i];
          }
          
          Menu menu;
          menu.items=menuitems;
          menu.numitems=3;
          menu.selection=getSetting(SETTING_DATEFORMAT)+1;
          menu.title = (char*)"Set date format";
          int res = doMenu(&menu);
          if(res==MENU_RETURN_SELECTION) setSetting(SETTING_DATEFORMAT, menu.selection-1, 1);
          break;
        }
        case 5:
          homeSettingsMenu();
          break;
        case 6:
          calendarSettingsMenu();
          break;
        case 7:
          lockSettingsMenu();
          break;
        case 8:
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
          smallmenu.selection=getSetting(SETTING_CHRONO_NOTIFICATION_TYPE)+1;
          smallmenu.title = (char*)"Chrono. notif.";

          int sres = doMenu(&smallmenu);
          if(sres == MENU_RETURN_SELECTION) setSetting(SETTING_CHRONO_NOTIFICATION_TYPE, smallmenu.selection-1, 1);
          mMsgBoxPop();
          break;
        }
        case 9:
          menuitems[8].value = !menuitems[8].value;
          setSetting(SETTING_SHOW_ADVANCED, menuitems[8].value, 1); 
          break;
        case 10: // set startup brightness
        { Selector sel;
          sel.title = (char*)"Set start brightness";
          sel.value = getSetting(SETTING_STARTUP_BRIGHTNESS);
          sel.min = 1;
          sel.max = 250;
          sel.type = SELECTORTYPE_STARTUP_BRIGHTNESS;
          int res = doSelector(&sel);
          if (res == SELECTOR_RETURN_EXIT) continue;
          setSetting(SETTING_STARTUP_BRIGHTNESS, sel.value, 1);
          break;
        }
        case 11:
          aboutScreen();
          break;
      }
    }
  }
}

inline static void lockSettingsMenu() {
  MenuItem menuitems[6];
  menuitems[0].text = (char*)"Set password";
  
  menuitems[1].text = (char*)"Show last character";
  menuitems[1].type = MENUITEM_CHECKBOX;
  
  menuitems[2].text = (char*)"Off after locking";
  menuitems[2].type = MENUITEM_CHECKBOX;
  
  menuitems[3].text = (char*)"Lock on [EXE]";
  menuitems[3].type = MENUITEM_CHECKBOX;

  menuitems[4].text = (char*)"Show owner info";
  menuitems[4].type = MENUITEM_CHECKBOX;
  
  menuitems[5].text = (char*)"Run-Mat on unlock";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=6;
  menu.scrollout=1;
  while(1) {
    menuitems[1].value = getSetting(SETTING_PASSWORD_PRIVACY);
    menuitems[2].value = getSetting(SETTING_LOCK_AUTOOFF);
    menuitems[3].value = getSetting(SETTING_LOCK_ON_EXE);
    menuitems[4].value = getSetting(SETTING_LOCK_USERNAME);
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1:
          setPassword();
          break;
        case 2:
          menuitems[1].value = !menuitems[1].value;
          setSetting(SETTING_PASSWORD_PRIVACY, menuitems[1].value, 1); 
          break;
        case 3:
          menuitems[2].value = !menuitems[2].value;
          setSetting(SETTING_LOCK_AUTOOFF, menuitems[2].value, 1);
          break;
        case 4:
          menuitems[3].value = !menuitems[3].value;
          setSetting(SETTING_LOCK_ON_EXE, menuitems[3].value, 1);
          break;
        case 5:
          menuitems[4].value = !menuitems[4].value;
          setSetting(SETTING_LOCK_USERNAME, menuitems[4].value, 1);
          break;
        case 6: {
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
          smallmenu.selection=getSetting(SETTING_UNLOCK_RUNMAT)+1;
          smallmenu.title = (char*)"Run-Mat on unlock";
          int sres = doMenu(&smallmenu);
          if(sres == MENU_RETURN_SELECTION) {
            setSetting(SETTING_UNLOCK_RUNMAT, smallmenu.selection-1, 1);
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
  MenuItem smallmenuitems[11];
  
  Menu smallmenu;
  smallmenu.items=smallmenuitems;
  smallmenu.numitems=11;
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
  smallmenuitems[7].text = (char*)"Chrono. shortcut";
  smallmenuitems[8].text = (char*)"Bal. mgr.shortcut";
  smallmenuitems[9].text = (char*)"Pw. gen. shortcut";
  smallmenuitems[10].text = (char*)"TOTP shortcut";
  int sres = doMenu(&smallmenu);
  mMsgBoxPop();
  if(sres == MENU_RETURN_SELECTION) return smallmenu.selection - 1;
  else return -1;
}

inline static void homeSettingsMenu() {
  MenuItem menuitems[10];
  menuitems[0].text = (char*)"Clock type";
  menuitems[1].text = (char*)"Date appearance";
  
  menuitems[2].text = (char*)"Show seconds";
  menuitems[2].type = MENUITEM_CHECKBOX;
  
  menuitems[3].text = (char*)"Show F. keys labels";
  menuitems[3].type = MENUITEM_CHECKBOX;

  menuitems[4].text = (char*)"Display statusbar";
  menuitems[4].type = MENUITEM_CHECKBOX;
  
  menuitems[5].text = (char*)"Dark theme";
  menuitems[5].type = MENUITEM_CHECKBOX;

  menuitems[6].text = (char*)"Top home panel";
  menuitems[7].text = (char*)"Right home panel";
  menuitems[8].text = (char*)"Bottom home panel";
  menuitems[9].text = (char*)"Left home panel";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=10;
  menu.scrollout=1;
  while(1) {
    menuitems[2].value = getSetting(SETTING_CLOCK_SECONDS);
    menuitems[3].value = getSetting(SETTING_DISPLAY_FKEYS);
    menuitems[4].value = getSetting(SETTING_DISPLAY_STATUSBAR);
    menuitems[5].value = getSetting(SETTING_THEME);
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1: {
          int inscreen = 1;
          int cur = getSetting(SETTING_CLOCK_TYPE);
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
                setSetting(SETTING_CLOCK_TYPE, cur, 1); 
                //deliberate fallthrough
              case KEY_CTRL_EXIT:
                inscreen = 0;
                break;
            }
          }
          break;
        }
        case 2:
        { Selector format;
          format.title = (char*)"Set long date format";
          format.value = getSetting(SETTING_LONGDATEFORMAT);
          format.min = 0;
          format.max = 9;
          format.type = SELECTORTYPE_LONGDATEFORMAT;
          int res = doSelector(&format);
          if (res == SELECTOR_RETURN_EXIT) continue;
          setSetting(SETTING_LONGDATEFORMAT, format.value, 1);
          break;
        }
        case 3:
          menuitems[2].value = !menuitems[2].value;
          setSetting(SETTING_CLOCK_SECONDS, menuitems[2].value, 1);
          break;
        case 4:
          menuitems[3].value = !menuitems[3].value;
          setSetting(SETTING_DISPLAY_FKEYS, menuitems[3].value, 1);
          break;
        case 5:
          menuitems[4].value = !menuitems[4].value;
          setSetting(SETTING_DISPLAY_STATUSBAR, menuitems[4].value, 1);
          break;
        case 6:
          menuitems[5].value = !menuitems[5].value;
          setSetting(SETTING_THEME, menuitems[5].value, 1); 
          break;
        case 7:
        case 8:
        case 9:
        case 10:
        {
          int s = menu.selection-7+25;
          int t = selectPaneType(getSetting(s));
          if(t >= 0) setSetting(s, t, 1);
          break;
        }
      }
    }
  }
}

void calendarSettingsMenu() {
  MenuItem menuitems[4];
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
    menuitems[0].value = getSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT);
    menuitems[3].value = getSetting(SETTING_SHOW_CALENDAR_BUSY_MAP);
    int res = doMenu(&menu);
    if(res == MENU_RETURN_EXIT) return;
    else if(res == MENU_RETURN_SELECTION) {
      switch(menu.selection) {
        case 1:
          menuitems[0].value = !menuitems[0].value;
          setSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT, menuitems[0].value, 1);
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
          if(menu.selection == 2) {
            smallmenu.title = (char*)"Default Calendar";
            smallmenu.selection=getSetting(SETTING_DEFAULT_CALENDAR_VIEW)+1;
            smallmenuitems[0].text = (char*)"Week";
            smallmenuitems[1].text = (char*)"Month";
          } else {
            smallmenu.title = (char*)"Week starts on";
            smallmenu.selection=getSetting(SETTING_WEEK_START_DAY)+1;
            smallmenuitems[0].text = (char*)getDOWAsString(1); // Sunday
            smallmenuitems[1].text = (char*)getDOWAsString(2); // Monday
          }
          int sres = doMenu(&smallmenu);
          if(sres == MENU_RETURN_SELECTION)
            setSetting((menu.selection == 2 ? SETTING_DEFAULT_CALENDAR_VIEW : SETTING_WEEK_START_DAY), smallmenu.selection-1, 1);
          mMsgBoxPop();
          break;
        }
        case 4:
          menuitems[3].value = !menuitems[3].value;
          setSetting(SETTING_SHOW_CALENDAR_BUSY_MAP, menuitems[3].value, 1);
          break;
      }
    }
  }
}