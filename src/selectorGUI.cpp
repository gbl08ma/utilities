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

#include "selectorGUI.hpp"
#include "timeGUI.hpp"
#include "keyboardProvider.hpp"
#include "timeProvider.hpp"

int doSelector(Selector* selector) {
  int key;
  int initialValue = selector->value; // so we can restore later
  char buffer1[50] = "";
  char buffer2[50] = "";

  if(selector->clearVRAM) {
    Bdisp_AllClr_VRAM();
    DisplayStatusArea();
  }
  strcpy(buffer1, "  ");
  strcat(buffer1, selector->title);
  PrintXY(1, 1, (char*)buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  strcpy(buffer1, "  ");
  strcat(buffer1, selector->subtitle);
  PrintXY(3, 2, (char*)buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, (selector->type == SELECTORTYPE_LONGDATEFORMAT ? 3 : 4), (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(1)
  {
    if(selector->type == SELECTORTYPE_LONGDATEFORMAT) PrintXY(1, 4, (char*)"                           ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    if(selector->type != SELECTORTYPE_LONGDATEFORMAT) {
      if(selector->type == SELECTORTYPE_MONTH) {
        strcat(buffer1, getMonthAsString(selector->value));
      } else if(selector->type == SELECTORTYPE_TIMEFORMAT) {
        strcpy(buffer2, "");
        currentTimeToString(buffer2, selector->value);
        strcat(buffer1, buffer2);
      } else if(selector->type == SELECTORTYPE_DATEFORMAT) {
        strcpy(buffer2, "");
        currentDateToString(buffer2, selector->value);
        strcat(buffer1, buffer2);
      } else if(selector->type == SELECTORTYPE_STARTUP_BRIGHTNESS) {
        strcpy(buffer2, "");
        if(selector->value == 250) strcpy(buffer2, "Do not force");
        else itoa(selector->value, (unsigned char*)buffer2);
        strcat(buffer1, buffer2);
      } else if(selector->type == SELECTORTYPE_BACKLIGHT_DURATION) {
        if(selector->value % 2 == 0) { //even, so timeout is X min 0 sec.
          itoa(selector->value/2, (unsigned char*)buffer2);
        } else { // timeout is X min 30 sec.
          itoa((selector->value-1)/2, (unsigned char*)buffer2);
        }
        strcat(buffer1, buffer2);
        strcat(buffer1, " Minutes");
        if(selector->value % 2 != 0) strcat(buffer1, " 30 Sec.");
      } else if(selector->type == SELECTORTYPE_TIMEOUT_MINUTES) {
        itoa(selector->value, (unsigned char*)buffer2);
        strcat(buffer1, buffer2);
        strcat(buffer1, " Minutes");
      } else if(selector->type == SELECTORTYPE_TEXTCOLOR) {
        switch (selector->value) {
          case 0: strcat(buffer1, "Blue"); break;
          case 1: strcat(buffer1, "Green"); break;
          case 2: strcat(buffer1, "Red"); break;
          case 3: strcat(buffer1, "Orange"); break;
          case 4: strcat(buffer1, "Yellow"); break;
          case 5: strcat(buffer1, "Cyan"); break;
          case 6: strcat(buffer1, "Brown"); break;
          case 7: strcat(buffer1, "Magenta"); break;
          default: strcat(buffer1, "Unknown"); break;
        }
      } else if(selector->type == SELECTORTYPE_TIMERTYPE) {
        switch (selector->value) {
          case 0: strcat(buffer1, "Upwards"); break;
          case 1: strcat(buffer1, "Downwards"); break;
          default: strcat(buffer1, "Unknown"); break;
        }
      } else {
        itoa(selector->value, (unsigned char*)buffer2);
        strcat(buffer1, buffer2);
      }
      PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else {
      drawLongDate(82,selector->value, COLOR_BLACK, COLOR_WHITE, NULL);
    }
    if (selector->allowMkey) mGetKey(&key);
    else GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selector->value > selector->min) { //don't allow to set below 1970 so it is Unix-time compatible and always has 4 digits
          selector->value--;
        } else {
          if(selector->cycle) selector->value=selector->max;
        }
        if(selector->type == SELECTORTYPE_INSTANT_RETURN) return SELECTOR_RETURN_INSTANT;
        break;
      case KEY_CTRL_UP:
        if (selector->max == -1 || selector->value < selector->max) {
          selector->value++;
        } else {
          if(selector->cycle) selector->value=selector->min;
        }
        if(selector->type == SELECTORTYPE_INSTANT_RETURN) return SELECTOR_RETURN_INSTANT;
        break;
      case KEY_CTRL_EXE:
        return SELECTOR_RETURN_SELECTION;
        break;
      case KEY_CTRL_EXIT:
        selector->value = initialValue;
        return SELECTOR_RETURN_EXIT;
    }
  }
  return SELECTOR_RETURN_EXIT;
}
