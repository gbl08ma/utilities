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
#include "stringsProvider.hpp"
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

void formatChronoString(chronometer* tchrono, int num, char* string, int isChronoView) {
  long long int unixtime = currentUEBT();
  long long int unixdiff;
  char buffer[20];
  if(!isChronoView) {
    itoa(num, (unsigned char*)buffer);
    if(tchrono->state == CHRONO_STATE_CLEARED) {
      strcpy(string, "\xe6\xa6");
    } else if (tchrono->state == CHRONO_STATE_RUNNING) {
      if(tchrono->type == CHRONO_TYPE_UP) strcpy(string, "\xe6\x9C");
      else strcpy(string, "\xe6\x9D"); 
    } else {
      if(tchrono->type == CHRONO_TYPE_UP) strcpy(string, "\xe6\xAC");
      else strcpy(string, "\xe6\xAD"); 
    }
    string += 2;
    string += strncpy_retlen(string, buffer, 3);
    *string = ':'; string++; *string = 0;
  } else strcpy(string, "");
  
  if(tchrono->state == CHRONO_STATE_CLEARED) { return; } //nothing else to add, chrono is clear
  else if(tchrono->state == CHRONO_STATE_STOPPED) {
    //diff will be calculated in a different way, so that it is always stopped
    if(tchrono->type == CHRONO_TYPE_DOWN) unixdiff = tchrono->starttime+tchrono->duration-tchrono->laststop;
    else unixdiff = tchrono->laststop-tchrono->starttime;    
  } else {
    if(tchrono->type == CHRONO_TYPE_DOWN) unixdiff = tchrono->starttime+tchrono->duration-unixtime;
    else unixdiff = unixtime-tchrono->starttime;
  }

  int days, hours, minutes;
  long long int seconds, milliseconds;

  milliseconds=unixdiff;  
  seconds = milliseconds / 1000;
  milliseconds %= 1000;
  minutes = (int)(seconds / 60);
  seconds %= 60;
  hours = minutes / 60;
  minutes %= 60;
  days = hours / 24;
  hours %= 24;

  if(tchrono->state != CHRONO_STATE_STOPPED || unixtime % 1000 < 500) {
    if (days) {
      itoa(days, (unsigned char*)buffer);
      string += strncpy_retlen(string, buffer, 11);
      strcpy(string, (char*)"\xe7\x64"); // small "d"
      string += 2;
    }
    timeToString(string, hours, minutes, (int)seconds, 0, !isChronoView || days < 100000);
    
    if(!isChronoView || !days) {
      string += 5;
      if(!isChronoView || days < 100000) string += 3;
      *string = '.'; string++;
      itoa_zeropad((int)milliseconds, string, 3);
    }
  }
  if(isChronoView) {
    drawAnalogChronometer(LCD_WIDTH_PX / 2, LCD_HEIGHT_PX / 2 + 10, 65, COLOR_WHITE, COLOR_BLACK, days, hours, minutes, seconds*1000 + milliseconds);
  }
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
void chronoScreen() {
  chronometer* chrono = getChronoArrayPtr();
  lastChronoComplete = 0; // clear possible notification on home screen
  // setting a timer is needed to change some aspects of GetKeyWait_OS
  stubTimer = Timer_Install(0, doNothing, 50);
  if (stubTimer > 0) { Timer_Start(stubTimer); }
  
  // construct menu items
  MenuItem menuitems[NUMBER_OF_CHRONO];
  int curcolor = TEXT_COLOR_BLUE;
  for(int curitem=0; curitem < NUMBER_OF_CHRONO; curitem++) {
    menuitems[curitem].type = MENUITEM_CHECKBOX;
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
  menu.title = (char*)"Chronometers";
  
  short unsigned int key;
  while(1) {
    checkChronoComplete(chrono);
    char text[NUMBER_OF_CHRONO][42];
    int hasSelection = 0;
    for(int curitem=0; curitem < NUMBER_OF_CHRONO; curitem++) {
      formatChronoString(&chrono[curitem], curitem+1, text[curitem], 0);
      menuitems[curitem].text = text[curitem];
      if(menuitems[curitem].value) hasSelection = 1;
    }
    if(menu.fkeypage==0) {
      drawFkeyLabels(0x0037, // SELECT (white)
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED ? 0x0010 : 0), // SET
        (hasSelection || chrono[menu.selection-1].state != CHRONO_STATE_CLEARED ? 0x0149 : 0), // CLEAR
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_STOPPED ? 0x040A : 0), // play icon
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_RUNNING ? 0x0031 : 0), // stop icon
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED ? 0x0156 : 0)); // BUILT-IN
      // "hack" the stop icon, turning it into a pause icon
      drawRectangle(286, 197, 4, 14, COLOR_WHITE);
    } else if (menu.fkeypage==1) {
      // SELECT (white), ALL (white), None (white), REVERSE (white), empty, empty
      drawFkeyLabels(0x0037, 0x0398, 0x016A, 0x045B, 0, 0);
    }
    DisplayStatusArea();
    doMenu(&menu);
    
    int keyCol = 0, keyRow = 0;
    Bdisp_PutDisp_DD();
    GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key); // just to handle the Menu key
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
        if(menu.selection == menu.numitems) {
          menu.selection = 1;
          menu.scroll = 0;
        } else {
          menu.selection++;
          if(menu.selection > menu.scroll+(menu.numitems>6 ? 6 : menu.numitems))
            menu.scroll = menu.selection -(menu.numitems>6 ? 6 : menu.numitems);
        }
        break;
      case KEY_PRGM_UP:
        if(menu.selection == 1) {
          menu.selection = menu.numitems;
          menu.scroll = menu.selection-6;
        } else {
          menu.selection--;
          if(menu.selection-1 < menu.scroll)
            menu.scroll = menu.selection -1;
        }
        break;
      case KEY_PRGM_RETURN:
      case KEY_PRGM_RIGHT:
        viewChrono(&menu, chrono);
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
        if(menu.fkeypage==0 && (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED)) setChronoScreen(&menu, chrono, 0);
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
        if(menu.fkeypage==0 && (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED)) setPresetChrono(&menu, chrono);
        break;
      case KEY_PRGM_EXIT:
        if(menu.fkeypage==0) {
          stopAndUninstallStubTimer();
          return;
        }
        else menu.fkeypage=0;
        break;
    }
    if (key && key!=KEY_PRGM_SHIFT) SetSetupSetting( (unsigned int)0x14, 0);
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

void setChronoScreen(Menu* menu, chronometer* tchrono, int menuSelOnly) {
  long long int ms = 0;
  int type = CHRONO_TYPE_UP;
  MenuItem menuitems[10];
  menuitems[0].text = (char*)"Upwards";
  menuitems[1].text = (char*)"Downwards (period)";
  menuitems[2].text = (char*)"Downwards (date-time)";
  
  Menu bmenu;
  bmenu.items=menuitems;
  bmenu.numitems=3;
  bmenu.height = 4;
  bmenu.title = (char*)"Set chronometer type";
  
  textArea text;
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.y = 4*24+5;

  textElement elem[2];
  text.elements = elem;
  text.scrollbar=0;
  
  elem[0].text = (char*)"Chronometers will start the moment they are set. You should familiarize yourself with the behavior of the chronometer functions before relying on them.";

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
        sel.title =  (char*)"Set downwards chrono.";
        sel.subtitle = (char*)"Days";
        sel.max = 1000000; // some sort of overflow happens with very big values, even though in theory it would all be within the limits of int64
        sel.cycle = 0;
        sel.type = SELECTORTYPE_NORMAL;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        int days = sel.value;
        
        sel.subtitle = (char*)"Hours";
        sel.max = 23;
        sel.value = 0;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        int hours = sel.value;
        
        sel.subtitle = (char*)"Minutes";
        sel.max = 59;
        sel.value = 0;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        int minutes = sel.value;
        
        sel.subtitle = (char*)"Seconds";
        // yes, we are assigning the truth value to two vars at once:
        sel.value = sel.min = (days == 0 && hours == 0 && minutes == 0);
        sel.max = 59;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        ms = 1000LL * ((long long int)sel.value + 60LL*(long long int)minutes +
                        60LL*60LL*(long long int)hours + 60LL*60LL*24LL*(long long int)days);
        type=CHRONO_TYPE_DOWN;
        break;
      } else if(bmenu.selection == 3) {
        int y=getCurrentYear(), m=getCurrentMonth(), d=getCurrentDay();
        if(selectDateScreen(&y, &m, &d, (char*)"Select chronometer end date", (char*)"", 1)) return;
        if(dateToDays(y, m, d) - dateToDays(getCurrentYear(), getCurrentMonth(), getCurrentDay()) < 0) {
          mMsgBoxPush(4);
          multiPrintXY(3, 2, "Date is in the\npast.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          closeMsgBox();
          return;
        }
        Bdisp_AllClr_VRAM();
        drawScreenTitle("Set downwards chrono.", "Chronometer end time:");
        mPrintXY(8, 4, "HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        
        textInput input;
        input.x=8;
        input.width=6;
        input.charlimit=6;
        input.acceptF6=1;
        input.type=INPUTTYPE_TIME;
        char etbuffer[15];
        etbuffer[0] = 0;
        input.buffer = (char*)etbuffer;
        int h=0,mi=0,s=0;
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
        ms = dateTimeToUEBT(y, m, d, h, mi, s, 0) - currentUEBT();
        if(ms < 0) {
          mMsgBoxPush(4);
          multiPrintXY(3, 2, "Time is in the\npast.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          closeMsgBox();
          return;
        }
        type=CHRONO_TYPE_DOWN;
        break;
      }
    }
  }
  
  int hasPerformedAny = 0;
  if(!menuSelOnly) {
    for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) {
      if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
        setChrono(&tchrono[cur], ms, type);
        hasPerformedAny=1;
      }
    }
  }
  if(!hasPerformedAny) setChrono(&tchrono[menu->selection-1], ms, type); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO);
}

void setPresetChrono(Menu* menu, chronometer* tchrono) {
  long long int duration = 0;
  MenuItem menuitems[10];
  menuitems[0].text = (char*)"1 minute timer";  
  menuitems[1].text = (char*)"5 minutes timer";  
  menuitems[2].text = (char*)"15 minutes timer";  
  menuitems[3].text = (char*)"30 minutes timer";
  menuitems[4].text = (char*)"1 hour timer";
  menuitems[5].text = (char*)"1 hour 30 min. timer";
  menuitems[6].text = (char*)"2 hours timer";
  menuitems[7].text = (char*)"5 hours timer";
  menuitems[8].text = (char*)"12 hours timer";
  menuitems[9].text = (char*)"1 day timer";
  
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
  saveChronoArray(tchrono, NUMBER_OF_CHRONO);
  Bdisp_AllClr_VRAM();
}

int getLastCompleteChrono() { return lastChronoComplete; }

void checkChronoComplete(chronometer* chronoarray) {
  long long int curtime = currentUEBT();
  for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) {
    if(chronoarray[cur].state == CHRONO_STATE_RUNNING && chronoarray[cur].type == CHRONO_TYPE_DOWN && //...
    // check if chrono is complete
    // if end time of chrono (start+duration) <= current time and chrono is running, chrono is complete
    /*...*/  chronoarray[cur].starttime+chronoarray[cur].duration<=curtime) {
      //clear this chrono
      clearChrono(&chronoarray[cur]);
      saveChronoArray(chronoarray, NUMBER_OF_CHRONO);
      lastChronoComplete = cur+1; // lastChronoComplete is one-based
      if(getSetting(SETTING_CHRONO_NOTIFICATION_TYPE) && getSetting(SETTING_CHRONO_NOTIFICATION_TYPE) != 3) {
        // user wants notification with pop-up
        mMsgBoxPush(4);
        char buffer[50];
        sprintf(buffer, "Chronometer %d\nended.", cur+1);
        multiPrintXY(3,2, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        
        if(getSetting(SETTING_CHRONO_NOTIFICATION_TYPE) == 1) {
          // notification with screen flashing
          PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
          flashLight(1); // with parameter set to 1, it doesn't change VRAM, and since it returns on pressing EXIT...
          mMsgBoxPop();
        } else {
          // without screen flashing
          closeMsgBox();
        }
        lastChronoComplete = 0; // otherwise, notification may show on home for a previous timer, if the user changes the setting in the meantime.
      }
    }
  }
}

void viewChrono(Menu* menu, chronometer* chrnarr) {
  unsigned short key = 0; int keyCol, keyRow;
  Bdisp_AllClr_VRAM();
  int key_zeroed = 0;
  chronometer* chrn = &chrnarr[menu->selection-1];
  while(key != KEY_PRGM_EXIT && key != KEY_PRGM_LEFT) {
    checkChronoComplete(chrnarr);
    clearLine(1,1);

    char tbuf[42];
    formatChronoString(chrn, menu->selection, tbuf, 1); // also takes care of drawing analog chrono
    if(strlen(tbuf)) printCentered(tbuf, 8*24-1, COLOR_BLACK, COLOR_WHITE);
    else clearLine(1,8);

    sprintf(tbuf, "Chronometer %d (", menu->selection);
    if(chrn->state == CHRONO_STATE_CLEARED) {
      strcat(tbuf, "\xe6\xa6");
      drawFkeyLabels(0, -1, -1, -1, -1, 0x0010); // SET
    } else if (chrn->state == CHRONO_STATE_RUNNING) {
      drawFkeyLabels(0x0031, -1, -1, -1, -1, 0x0149); // stop icon, CLEAR
      // "hack" the stop icon, turning it into a pause icon
      drawRectangle(30, 197, 4, 14, COLOR_WHITE);
      if(chrn->type == CHRONO_TYPE_UP) strcat(tbuf, "\xe6\x9C");
      else strcat(tbuf, "\xe6\x9D");
    } else {
      drawFkeyLabels(0x040A, -1, -1, -1, -1, 0x0149); // play icon, CLEAR
      if(chrn->type == CHRONO_TYPE_UP) strcat(tbuf, "\xe6\xAC");
      else strcat(tbuf, "\xe6\xAD"); 
    }
    strcat(tbuf, ")");
    drawScreenTitle(tbuf);

    DisplayStatusArea();
    Bdisp_PutDisp_DD();
    key = PRGM_GetKey();
    if(!key) key_zeroed = 1;
    else if(key_zeroed) {
      key_zeroed = 0;
      if(key == KEY_PRGM_MENU) GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key); //this is here just to handle the Menu key
      else if(key == KEY_PRGM_F1) {
        if(chrn->state == CHRONO_STATE_RUNNING) stopChrono(chrn);
        else if (chrn->state == CHRONO_STATE_STOPPED) startChrono(chrn);
        saveChronoArray(chrnarr, NUMBER_OF_CHRONO);
      } else if(key == KEY_PRGM_F6) {
        if (chrn->state != CHRONO_STATE_CLEARED) {
          clearChrono(chrn);
          saveChronoArray(chrnarr, NUMBER_OF_CHRONO);
        } else setChronoScreen(menu, chrnarr, 1);
        Bdisp_AllClr_VRAM();
      } else if(key == KEY_PRGM_UP || key == KEY_PRGM_DOWN) {
        if(key == KEY_PRGM_UP) {
          menu->selection--;
          if(menu->selection == 0) menu->selection = NUMBER_OF_CHRONO;
        } else {
          menu->selection++;
          if(menu->selection > NUMBER_OF_CHRONO) menu->selection = 1;
        }
        chrn = &chrnarr[menu->selection-1];
        Bdisp_AllClr_VRAM();
      }
    }
  }
  // key debouncing:
  GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key);
}