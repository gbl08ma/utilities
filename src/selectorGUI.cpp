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
#include "graphicsProvider.hpp"

int doSelector(Selector* selector) {
  int key;
  int initialValue = selector->value; // so we can restore later
  char buffer1[50] = "";
  char buffer2[50] = "";

  if(selector->clearVRAM) {
    Bdisp_AllClr_VRAM();
    DisplayStatusArea();
  }
  mPrintXY(1, 1, (char*)selector->title, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  mPrintXY(3, 2, (char*)selector->subtitle, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(5, (selector->type == SELECTORTYPE_LONGDATEFORMAT ? 3 : 4), (char*)"\xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  mPrintXY(5, 6, (char*)"\xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(1)
  {
    if(selector->type == SELECTORTYPE_LONGDATEFORMAT) clearLine(1,4);
    clearLine(5,5);
    strcpy(buffer1, "");
    if(selector->type != SELECTORTYPE_LONGDATEFORMAT) {
      if(selector->type == SELECTORTYPE_MONTH) {
        strcat(buffer1, getMonthAsString(selector->value));
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
      } else {
        itoa(selector->value, (unsigned char*)buffer2);
        strcat(buffer1, buffer2);
      }
      mPrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else {
      drawLongDate(82,selector->value, COLOR_BLACK, COLOR_WHITE, NULL);
    }
    if (selector->allowMkey) mGetKey(&key);
    else GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selector->value > selector->min) {
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
