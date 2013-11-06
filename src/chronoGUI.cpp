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
#include "settingsProvider.hpp"
#include "settingsGUI.hpp" 
#include "selectorGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"
#include "lightGUI.hpp"

void formatChronoString(chronometer* tchrono, int num, unsigned char* string)
{
  long long int unixtime = currentUnixTime();
  long long int unixdiff;
  char buffer[20] = "";
  itoa(num, (unsigned char*)buffer);
  if(tchrono->state == CHRONO_STATE_CLEARED) {
    strcpy((char*)string, "\xe6\xa6");
  } else {
    if(tchrono->type == CHRONO_TYPE_UP) strcpy((char*)string, "\xe6\x92");
    else strcpy((char*)string, "\xe6\x93"); 
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
  // setting a timer is needed to change some aspects of GetKeyWait_OS
  stubTimer = Timer_Install(0, doNothing, 50);
  if (stubTimer > 0) { Timer_Start(stubTimer); }
  
  // construct menu items
  MenuItem menuitems[NUMBER_OF_CHRONO];
  int curitem = 0;
  int curcolor = TEXT_COLOR_BLUE;
  while(curitem <= NUMBER_OF_CHRONO-1) {
    menuitems[curitem].type = MENUITEM_CHECKBOX;
    menuitems[curitem].value = MENUITEM_VALUE_NONE;
    menuitems[curitem].color = curcolor;
    if (curcolor==TEXT_COLOR_BLUE) curcolor = TEXT_COLOR_RED;
    else if (curcolor==TEXT_COLOR_RED) curcolor = TEXT_COLOR_GREEN;
    else if (curcolor==TEXT_COLOR_GREEN) curcolor = TEXT_COLOR_PURPLE;
    else if (curcolor==TEXT_COLOR_PURPLE) curcolor = TEXT_COLOR_BLACK;
    else if (curcolor==TEXT_COLOR_BLACK) curcolor = TEXT_COLOR_BLUE;
    curitem++;
  }
  Menu menu;
  menu.items=menuitems;
  menu.numitems=NUMBER_OF_CHRONO;
  menu.type=MENUTYPE_NO_KEY_HANDLING; // NOTE doMenu won't handle keys for us!
  menu.height=7;
  menu.scrollout=1;
  menu.showtitle=1;
  strcpy(menu.nodatamsg, "");
  strcpy(menu.title, "Chronometers");
  strcpy(menu.statusText, ""); 
  
  Bdisp_AllClr_VRAM();
  int iresult;
  short unsigned int key;
  unsigned short prevkey = 0;
  while(1) {
    checkChronoComplete();
    curitem=0;
    while(curitem <= NUMBER_OF_CHRONO-1) {
      unsigned char text[42] = "";
      formatChronoString(&chrono[curitem], curitem+1, text);
      strcpy(menuitems[curitem].text, (char*)text);
      curitem++;
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
    } else if (menu.fkeypage==1) {
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
          if (GetSetupSetting( (unsigned int)0x14) == 0) { SetSetupSetting( (unsigned int)0x14, 1); }
          else { SetSetupSetting( (unsigned int)0x14, 0); }
          break;
        case KEY_PRGM_MENU:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            settingsMenu();
            Bdisp_AllClr_VRAM();
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
            int cur = 0;
            while(cur <= NUMBER_OF_CHRONO-1) {
              menu.items[cur].value = MENUITEM_VALUE_CHECKED;
              cur++;
            } 
          }
          break;
        case KEY_PRGM_F3:
          if(menu.fkeypage==0) clearSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
          else if (menu.fkeypage==1) {
            // select none
            int cur = 0;
            while(cur <= NUMBER_OF_CHRONO-1) {
              menu.items[cur].value = MENUITEM_VALUE_NONE;
              cur++;
            } 
          }
          break;
        case KEY_PRGM_F4:
          if(menu.fkeypage==0) startSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
          else if (menu.fkeypage==1) {
            // reverse selection
            int cur = 0;
            while(cur <= NUMBER_OF_CHRONO-1) {
              menu.items[cur].value = !menu.items[cur].value;
              cur++;
            } 
          }
          break;
        case KEY_PRGM_F5:
          if(menu.fkeypage==0) stopSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
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
  int cur = 0, hasPerformedAny = 0;
  while(cur <= count-1) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      startChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
    cur++;
  }
  if(!hasPerformedAny) startChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}

void stopSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int cur = 0, hasPerformedAny = 0;
  while(cur <= count-1) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      stopChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
    cur++;
  }
  if(!hasPerformedAny) stopChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}

void clearSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int cur = 0, hasPerformedAny = 0;
  while(cur <= count-1) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      clearChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
    cur++;
  }
  if(!hasPerformedAny) clearChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}



void setChronoGUI(Menu* menu, chronometer* tchrono) {
  Selector sel;
  strcpy(sel.title, "Set timer");
  strcpy(sel.subtitle, "Type");
  sel.value = 0;
  sel.min = 0;
  sel.max = 1;
  sel.cycle = 1;
  sel.type = SELECTORTYPE_TIMERTYPE;
  int res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  int type = sel.value;
  if (type == CHRONO_TYPE_UP) {
    int cur = 0, hasPerformedAny = 0;
    while(cur <= NUMBER_OF_CHRONO-1) {
      if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
        setChrono(&tchrono[cur], 0, CHRONO_TYPE_UP);
        hasPerformedAny=1;
      }
      cur++;
    }
    if(!hasPerformedAny) setChrono(&tchrono[menu->selection-1], 0, CHRONO_TYPE_UP); // if there was no selected chrono, do it for the currently selected menu position
    return;
  }

  strcpy(sel.title, "Set timer");
  strcpy(sel.subtitle, "Days");
  sel.value = 0;
  sel.min = 0;
  sel.max = -1; // no limit. long long it is big enough to accomodate a chronometer with a duration of over 2 million days.
  sel.cycle = 0;
  sel.type = SELECTORTYPE_NORMAL;
  res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  unsigned int days = sel.value;
  
  strcpy(sel.title, "Set timer");
  strcpy(sel.subtitle, "Hours");
  sel.value = 0;
  sel.min = 0;
  sel.max = 23;
  sel.cycle = 0;
  sel.type = SELECTORTYPE_NORMAL;
  res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  int hours = sel.value;
  
  strcpy(sel.title, "Set timer");
  strcpy(sel.subtitle, "Minutes");
  sel.value = 0;
  sel.min = 0;
  sel.max = 59;
  sel.allowMkey = 1;
  sel.cycle = 0;
  sel.type = SELECTORTYPE_NORMAL;
  res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  int minutes = sel.value;
  
  strcpy(sel.title, "Set timer");
  strcpy(sel.subtitle, "Seconds");
  if(days == 0 && hours == 0 && minutes == 0) {
    sel.value = 1;
    sel.min = 1;
  } else {
    sel.value = 0;
    sel.min = 0;
  }
  sel.max = 59;
  sel.allowMkey = 1;
  sel.cycle = 0;
  sel.type = SELECTORTYPE_NORMAL;
  res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  long long int seconds = sel.value;
  
  seconds = seconds + 60*minutes + 60*60*hours + 60*60*24*days;
  
  int cur = 0, hasPerformedAny = 0;
  while(cur <= NUMBER_OF_CHRONO-1) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      setChrono(&tchrono[cur], seconds*1000, type);
      hasPerformedAny=1;
    }
    cur++;
  }
  if(!hasPerformedAny) setChrono(&tchrono[menu->selection-1], seconds*1000, type); // if there was no selected chrono, do it for the currently selected menu position
}

void checkDownwardsChronoCompleteGUI(chronometer* chronoarray, int count) {
  int cur = 0;
  while(cur <= count-1) {
    if(chronoarray[cur].state == CHRONO_STATE_RUNNING && chronoarray[cur].type == CHRONO_TYPE_DOWN) {
    // check if chrono is complete
    // if end time of chrono (start+duration) <= current time and chrono is running, chrono is complete
      if(chronoarray[cur].starttime+chronoarray[cur].duration<=currentUnixTime()) {
        //clear this chrono
        clearChrono(&chronoarray[cur]);
        saveChronoArray(chronoarray, NUMBER_OF_CHRONO);
        MsgBoxPush(4);
        PrintXY(3,2,(char*)"  Timer Complete", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        char buffer1[10] = "";
        itoa(cur+1, (unsigned char*)buffer1);
        char buffer2[25] = "";
        strcpy(buffer2, "  Chronometer: ");
        strcat(buffer2, buffer1);
        PrintXY(3,3,(char*)buffer2, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
        
        Bdisp_PutDisp_DD();
        flashLight(1); // with parameter set to 1, it doesn't change VRAM, and since it returns on pressing EXIT...
        MsgBoxPop();
        //and return
        return;
      }
    }
    cur++;
  }
}

