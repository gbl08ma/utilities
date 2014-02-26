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
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "timeGUI.hpp"
#include "textGUI.hpp"
#include "settingsProvider.hpp"
#include "settingsGUI.hpp" 
#include "selectorGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"
#include "lightGUI.hpp"
#include "calendarGUI.hpp"
#include "inputGUI.hpp"

static int lastChronoComplete=0; // 1-based. 0 means no chrono completed. for notification on home screen.

inline void formatChronoString(chronometer* tchrono, int num, unsigned char* string)
{
  long long int unixtime = currentUnixTime();
  long long int unixdiff;
  char buffer[20] = "";
  itoa(num, (unsigned char*)buffer);
  if(tchrono->state == CHRONO_STATE_CLEARED) {
    strcpy((char*)string, "\xe6\xa6");
  } else if (tchrono->state == CHRONO_STATE_RUNNING) {
    if(tchrono->type == CHRONO_TYPE_UP) strcpy((char*)string, "\xe6\x9C");
    else strcpy((char*)string, "\xe6\x9D"); 
  } else {
    if(tchrono->type == CHRONO_TYPE_UP) strcpy((char*)string, "\xe6\xAC");
    else strcpy((char*)string, "\xe6\xAD"); 
  }
  strcat((char*)string, buffer);
  strcat((char*)string, ":");
  
  if(tchrono->state == CHRONO_STATE_CLEARED) { return; } //nothing else to add, chrono is clear
  else if(tchrono->state == CHRONO_STATE_STOPPED) {
    //diff will be calculated in a different way, so that it is always stopped
    if(tchrono->type == CHRONO_TYPE_DOWN) unixdiff = tchrono->starttime+tchrono->duration-tchrono->laststop;
    else unixdiff = tchrono->laststop-tchrono->starttime;    
  } else {
    if(tchrono->type == CHRONO_TYPE_DOWN) unixdiff = tchrono->starttime+tchrono->duration-unixtime;
    else unixdiff = unixtime-tchrono->starttime;
  }

  long long int days=0,hours=0,minutes=0,seconds=0, milliseconds=0;

  milliseconds=unixdiff;  
  seconds = milliseconds / 1000;
  milliseconds %= 1000;
  minutes = seconds / 60;
  seconds %= 60;
  hours = minutes / 60;
  minutes %= 60;
  days = hours / 24;
  hours %= 24;

  if (days) {
    itoa(days, (unsigned char*)buffer);
    strcat((char*)string, buffer);
    strcat((char*)string, (char*)"\xe7\x64"); // small "d"
  }
  
  itoa(hours, (unsigned char*)buffer);
  if (hours < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(minutes, (unsigned char*)buffer);
  if (minutes < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(seconds, (unsigned char*)buffer);
  if (seconds < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  
  strcat((char*)string, ".");
  itoa(milliseconds, (unsigned char*)buffer);
  strcat((char*)string, buffer);
}
void doNothing() {}
static int stubTimer=0;
// for the mGetKey function to call before longjump, when user presses Shift+Exit
void stopAndUninstallStubTimer() {
  if(stubTimer>0) {
    Timer_Stop(stubTimer);
    Timer_Deinstall(stubTimer);
    stubTimer=0;
  }
}
void chronoScreen(chronometer* chrono) {
  lastChronoComplete = 0; // clear possible notification on home screen
  // setting a timer is needed to change some aspects of GetKeyWait_OS
  stubTimer = Timer_Install(0, doNothing, 50);
  if (stubTimer > 0) { Timer_Start(stubTimer); }
  
  // construct menu items
  MenuItem menuitems[NUMBER_OF_CHRONO];
  int curitem = 0;
  int curcolor = TEXT_COLOR_BLUE;
  for(int curitem=0; curitem < NUMBER_OF_CHRONO; curitem++) {
    menuitems[curitem].type = MENUITEM_CHECKBOX;
    menuitems[curitem].value = MENUITEM_VALUE_NONE;
    menuitems[curitem].color = curcolor;
    switch(curcolor) {
      case TEXT_COLOR_BLUE: curcolor = TEXT_COLOR_RED; break;
      case TEXT_COLOR_RED: curcolor = TEXT_COLOR_GREEN; break;
      case TEXT_COLOR_GREEN: curcolor = TEXT_COLOR_PURPLE; break;
      case TEXT_COLOR_PURPLE: curcolor = TEXT_COLOR_BLACK; break;
      case TEXT_COLOR_BLACK: curcolor = TEXT_COLOR_BLUE; break;
    }
  }
  Menu menu;
  menu.items=menuitems;
  menu.numitems=NUMBER_OF_CHRONO;
  menu.type=MENUTYPE_NO_KEY_HANDLING; // NOTE doMenu won't handle keys for us!
  menu.height=7;
  menu.scrollout=1;
  menu.showtitle=1;
  strcpy(menu.title, "Chronometers");
  
  Bdisp_AllClr_VRAM();
  int iresult;
  short unsigned int key;
  unsigned short prevkey = 0;
  while(1) {
    checkChronoComplete();
    for(curitem=0; curitem < NUMBER_OF_CHRONO; curitem++) {
      unsigned char text[42];
      formatChronoString(&chrono[curitem], curitem+1, text);
      strcpy(menuitems[curitem].text, (char*)text);
    }
    if(menu.fkeypage==0) {
      GetFKeyPtr(0x0037, &iresult); // SELECT (white)
      FKey_Display(0, (int*)iresult);
      GetFKeyPtr(0x0010, &iresult); // SET
      FKey_Display(1, (int*)iresult);
      GetFKeyPtr(0x0149, &iresult); // CLEAR
      FKey_Display(2, (int*)iresult);
      GetFKeyPtr(0x040A, &iresult); // play icon
      FKey_Display(3, (int*)iresult);
      GetFKeyPtr(0x0031, &iresult); // stop icon
      FKey_Display(4, (int*)iresult);
      // "hack" the stop icon, turning it into a pause icon
      drawRectangle(286, 197, 4, 14, COLOR_WHITE);
      GetFKeyPtr(0x0156, &iresult); // BUILT-IN
      FKey_Display(5, (int*)iresult);
    } else if (menu.fkeypage==1) {
      clearLine(1,8);
      GetFKeyPtr(0x0037, &iresult); // SELECT (white)
      FKey_Display(0, (int*)iresult);
      GetFKeyPtr(0x0398, &iresult); // ALL (white)
      FKey_Display(1, (int*)iresult);
      GetFKeyPtr(0x016A, &iresult); // None (white)
      FKey_Display(2, (int*)iresult);
      GetFKeyPtr(0x045B, &iresult); // REVERSE (white)
      FKey_Display(3, (int*)iresult);
      GetFKeyPtr(0x0000, &iresult); // (empty)
      FKey_Display(4, (int*)iresult);
    }
    
    doMenu(&menu);
    
    int keyCol = 0, keyRow = 0;
    Bdisp_PutDisp_DD();
    if (0 != GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key) ) {
      key = PRGM_GetKey();
      switch(key)
      {
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
        case KEY_PRGM_DOWN:
          if(menu.selection == menu.numitems)
          {
            menu.selection = 1;
            menu.scroll = 0;
          }
          else
          {
            menu.selection++;
            if(menu.selection > menu.scroll+(menu.numitems>6 ? 6 : menu.numitems))
              menu.scroll = menu.selection -(menu.numitems>6 ? 6 : menu.numitems);
          }
          break;
        case KEY_PRGM_UP:
          if(menu.selection == 1)
          {
            menu.selection = menu.numitems;
            menu.scroll = menu.selection-6;
          }
          else
          {
            menu.selection--;
            if(menu.selection-1 < menu.scroll)
              menu.scroll = menu.selection -1;
          }
          break;
        case KEY_PRGM_RETURN:
          saveChronoArray(chrono, NUMBER_OF_CHRONO);
          break;
        case KEY_PRGM_F1:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            menu.fkeypage=1;
          } else {
            if(menuitems[menu.selection-1].value == MENUITEM_VALUE_CHECKED) menuitems[menu.selection-1].value=MENUITEM_VALUE_NONE;
            else menuitems[menu.selection-1].value=MENUITEM_VALUE_CHECKED;
          }
          break;
        case KEY_PRGM_F2:
          if(menu.fkeypage==0) { setChronoGUI(&menu, chrono); saveChronoArray(chrono, NUMBER_OF_CHRONO); }
          else if (menu.fkeypage==1) {
            // select all
            for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) menu.items[cur].value = MENUITEM_VALUE_CHECKED;
          }
          break;
        case KEY_PRGM_F3:
          if(menu.fkeypage==0) clearSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
          else if (menu.fkeypage==1) {
            // select none
            for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) menu.items[cur].value = MENUITEM_VALUE_NONE;
          }
          break;
        case KEY_PRGM_F4:
          if(menu.fkeypage==0) startSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
          else if (menu.fkeypage==1) {
            // reverse selection
            for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) menu.items[cur].value = !menu.items[cur].value;
          }
          break;
        case KEY_PRGM_F5:
          if(menu.fkeypage==0) stopSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
          break;
        case KEY_PRGM_F6:
          if(menu.fkeypage==0) setBuiltinChrono(&menu, chrono);
          break;
        case KEY_PRGM_EXIT:
          if(menu.fkeypage==0) {
            stopAndUninstallStubTimer();
            return;
          }
          else menu.fkeypage=0;
          break;
      }
      if (key!=prevkey && key!=KEY_PRGM_SHIFT) SetSetupSetting( (unsigned int)0x14, 0);
    }
  }
}

void startSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int hasPerformedAny = 0;
  for(int cur = 0; cur < count; cur++) {
    if(menu->items[cur].value) {
      startChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
  }
  if(!hasPerformedAny) startChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}

void stopSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int hasPerformedAny = 0;
  for(int cur = 0; cur < count; cur++) {
    if(menu->items[cur].value) {
      stopChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
  }
  if(!hasPerformedAny) stopChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}

void clearSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int hasPerformedAny = 0;
  for(int cur = 0; cur < count; cur++) {
    if(menu->items[cur].value) {
      clearChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
  }
  if(!hasPerformedAny) clearChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}



void setChronoGUI(Menu* menu, chronometer* tchrono) {
  long long int seconds = 0;
  int type = CHRONO_TYPE_UP;
  MenuItem menuitems[10];
  strcpy(menuitems[0].text, "Upwards");  
  strcpy(menuitems[1].text, "Downwards (period)");  
  strcpy(menuitems[2].text, "Downwards (date-time)");  
  
  Menu bmenu;
  bmenu.items=menuitems;
  bmenu.numitems=3;
  bmenu.height = 4;
  bmenu.scrollbar=0;
  strcpy(bmenu.title, (char*)"Set chronometer type");
  bmenu.showtitle=1;
  
  textArea text;
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.showtitle = 0;
  text.y = 4*24+5;

  textElement elem[2];
  text.elements = elem;
  text.scrollbar=0;
  
  elem[0].text = (char*)"Chronometers will start the moment you set them. You should familiarize yourself with the behavior of the chronometer function before using it for something important.";

  text.numelements = 1;
  Bdisp_AllClr_VRAM();
  
  while(1) {
    doTextArea(&text);
    int res = doMenu(&bmenu);
    if(res == MENU_RETURN_EXIT) {
      Bdisp_AllClr_VRAM();
      return;
    } else if(res == MENU_RETURN_SELECTION) {
      if(bmenu.selection == 1) {
        type=CHRONO_TYPE_UP;
        break;
      } else if(bmenu.selection == 2) {
        Selector sel;
        strcpy(sel.title, "Set downwards chrono.");
        strcpy(sel.subtitle, "Days");
        sel.max = -1; // no limit. long long int is big enough to accomodate a chronometer with a duration of over 2 million days.
        sel.cycle = 0;
        sel.type = SELECTORTYPE_NORMAL;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        long int days = sel.value;
        
        strcpy(sel.subtitle, "Hours");
        sel.max = 23;
        sel.value = 0;
        sel.cycle = 0;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        int hours = sel.value;
        
        strcpy(sel.subtitle, "Minutes");
        sel.max = 59;
        sel.value = 0;
        sel.cycle = 0;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        int minutes = sel.value;
        
        strcpy(sel.subtitle, "Seconds");
        // yes, we are assigning the truth value to two vars at once:
        sel.value = sel.min = (days == 0 && hours == 0 && minutes == 0);
        sel.max = 59;
        sel.cycle = 0;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        seconds = sel.value + 60*minutes + 60*60*hours + 60*60*24*days;
        type=CHRONO_TYPE_DOWN;
        break;
      } else if(bmenu.selection == 3) {
        short y=getCurrentYear(), m=getCurrentMonth(), d=getCurrentDay();
        if(chooseCalendarDate(&y, &m, &d, (char*)"Select chronometer end date", (char*)"", 1)) return;
        if(DateToDays(y, m, d) - DateToDays(getCurrentYear(), getCurrentMonth(), getCurrentDay()) < 0) {
          mMsgBoxPush(4);
          mPrintXY(3, 2, (char*)"Date is in the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 3, (char*)"past.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
          closeMsgBox();
          return;
        }
        Bdisp_AllClr_VRAM();
        mPrintXY(1, 1, (char*)"Set downwards chrono.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
        mPrintXY(1, 2, (char*)"Chronometer end time:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(8, 4, (char*)"HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        
        textInput input;
        input.x=8;
        input.width=6;
        input.charlimit=6;
        input.acceptF6=1;
        input.type=INPUTTYPE_TIME;
        char etbuffer[15] = "";
        input.buffer = (char*)etbuffer;
        short h=0,mi=0,s=0;
        while(1) {
          input.key=0;
          int res = doTextInput(&input);
          if (res==INPUT_RETURN_EXIT) return; // user aborted
          else if (res==INPUT_RETURN_CONFIRM) {
            if((int)strlen(etbuffer) == input.charlimit) {
                stringToTime(etbuffer, &h, &mi, &s);
                if(isTimeValid(h, mi, s)) {
                  break;
                } else invalidFieldMsg(1);
            } else {
              invalidFieldMsg(1);
            }
          } 
        }
        long long int duration = DateTime2Unix(y, m, d, h, mi, s, 0) - currentUnixTime();
        if(duration < 0) {
          mMsgBoxPush(4);
          mPrintXY(3, 2, (char*)"Time is in the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 3, (char*)"past.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
          closeMsgBox();
          return;
        } else {
          seconds = duration/1000;
          type=CHRONO_TYPE_DOWN;
          break;
        }
      }
    }
  }
  
  int hasPerformedAny = 0;
  for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      setChrono(&tchrono[cur], seconds*1000, type);
      hasPerformedAny=1;
    }
  }
  if(!hasPerformedAny) setChrono(&tchrono[menu->selection-1], seconds*1000, type); // if there was no selected chrono, do it for the currently selected menu position
}

void setBuiltinChrono(Menu* menu, chronometer* tchrono) {
  long long int duration = 0;
  MenuItem menuitems[10];
  strcpy(menuitems[0].text, "1 minute timer");  
  strcpy(menuitems[1].text, "5 minutes timer");  
  strcpy(menuitems[2].text, "15 minutes timer");  
  strcpy(menuitems[3].text, "30 minutes timer");
  strcpy(menuitems[4].text, "1 hour timer");
  strcpy(menuitems[5].text, "1 hour 30 min. timer");
  strcpy(menuitems[6].text, "2 hours timer");
  strcpy(menuitems[7].text, "5 hours timer");
  strcpy(menuitems[8].text, "12 hours timer");
  strcpy(menuitems[9].text, "1 day timer");
  
  Menu bmenu;
  bmenu.items=menuitems;
  bmenu.numitems=10;
  bmenu.scrollout=1;
  while(1) {
    int res = doMenu(&bmenu);
    if(res == MENU_RETURN_EXIT) {
      Bdisp_AllClr_VRAM();
      return;
    } else if(res == MENU_RETURN_SELECTION) {
      switch(bmenu.selection) {
        case 1: duration = 60*1000; break;
        case 2: duration = 5*60*1000; break;
        case 3: duration = 15*60*1000; break;
        case 4: duration = 30*60*1000; break;
        case 5: duration = 60*60*1000; break;
        case 6: duration = 90*60*1000; break;
        case 7: duration = 120*60*1000; break;
        case 8: duration = 5*60*60*1000; break;
        case 9: duration = 12*60*60*1000; break;
        case 10: duration = 24*60*60*1000; break;
      }
      break;
    }
  }
  
  int hasPerformedAny = 0;
  for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      setChrono(&tchrono[cur], duration, CHRONO_TYPE_DOWN);
      hasPerformedAny=1;
    }
  }
  if(!hasPerformedAny) setChrono(&tchrono[menu->selection-1], duration, CHRONO_TYPE_DOWN); // if there was no selected chrono, do it for the currently selected menu position
  Bdisp_AllClr_VRAM();
}

int getLastChronoComplete() { return lastChronoComplete; }

void checkDownwardsChronoCompleteGUI(chronometer* chronoarray, int count) {
  for(int cur = 0; cur < count; cur++) {
    if(chronoarray[cur].state == CHRONO_STATE_RUNNING && chronoarray[cur].type == CHRONO_TYPE_DOWN && //...
    // check if chrono is complete
    // if end time of chrono (start+duration) <= current time and chrono is running, chrono is complete
    /*...*/  chronoarray[cur].starttime+chronoarray[cur].duration<=currentUnixTime()) {
      //clear this chrono
      clearChrono(&chronoarray[cur]);
      saveChronoArray(chronoarray, NUMBER_OF_CHRONO);
      lastChronoComplete = cur+1; // lastChronoComplete is one-based
      if(GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE) && GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE) != 3) {
        // user wants notification with pop-up
        mMsgBoxPush(4);
        char buffer1[10] = "";
        itoa(cur+1, (unsigned char*)buffer1);
        mPrintXY(3,2,(char*)"Chronometer ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(15,2,(char*)buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(3,3,(char*)"ended.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
        
        if(GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE) == 1) {
          // notification with screen flashing
          Bdisp_PutDisp_DD();
          flashLight(1); // with parameter set to 1, it doesn't change VRAM, and since it returns on pressing EXIT...*/
          mMsgBoxPop();
        } else {
          // without screen flashing
          closeMsgBox();
        }
      }
    }
  }
}