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
#include "inputGUI.hpp"
#include "keyboardProvider.hpp"
#include "timeProvider.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"

int doSelector(Selector* selector) {
  int key;
  int initialValue = selector->value; // so we can restore later

  if(selector->clearVRAM)
    Bdisp_AllClr_VRAM();
  if(selector->title != NULL)
    drawScreenTitle(selector->title);
  if(selector->subtitle != NULL)
    mPrintXY(3, 2, selector->subtitle, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  while(1)
  {
    clearLine(5,(selector->type == SELECTORTYPE_LONGDATEFORMAT ? 3 : 4));
    if(selector->cycle == 1 || selector->value != selector->max)
      mPrintXY(5, (selector->type == SELECTORTYPE_LONGDATEFORMAT ? 3 : 4),
               "\xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
    clearLine(5,6);
    if(selector->cycle == 1 || selector->value != selector->min)
      mPrintXY(5, 6, "\xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
    clearLine(5,5);
    if(selector->type == SELECTORTYPE_LONGDATEFORMAT) {
      clearLine(1,4);
      drawLongDate(82,selector->value, COLOR_BLACK, COLOR_WHITE);
    } else {
      char buffer1[50];
      switch(selector->type) {
        case SELECTORTYPE_MONTH:
          strcpy(buffer1, getMonthAsString(selector->value));
          break;
        case SELECTORTYPE_STARTUP_BRIGHTNESS:
          if(selector->value == 250) strcpy(buffer1, "Do not change");
          else itoa(selector->value, (unsigned char*)buffer1);
          break;
        case SELECTORTYPE_BACKLIGHT_DURATION:
          itoa(selector->value/2, (unsigned char*)buffer1);
          strcat(buffer1, " Minute");
          if(selector->value != 2 && selector->value != 3) strcat(buffer1, "s");
          if(selector->value % 2 != 0) strcat(buffer1, " 30 Sec.");
          break;
        case SELECTORTYPE_TIMEOUT_MINUTES:
          itoa(selector->value, (unsigned char*)buffer1);
          strcat(buffer1, " Minute");
          if(selector->value != 1) strcat(buffer1, "s");
          break;
        default:
          itoa(selector->value, (unsigned char*)buffer1);
          break;
      }
      mPrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
    mGetKey(&key);
    switch(key) {
      case KEY_CTRL_DOWN:
        if (selector->value > selector->min) selector->value--;
        else if(selector->cycle) selector->value=selector->max;
        if(selector->type == SELECTORTYPE_INSTANT_RETURN) return SELECTOR_RETURN_INSTANT;
        break;
      case KEY_CTRL_UP:
        if (selector->max == -1 || selector->value < selector->max) selector->value++;
        else if(selector->cycle) selector->value=selector->min;
        if(selector->type == SELECTORTYPE_INSTANT_RETURN) return SELECTOR_RETURN_INSTANT;
        break;
      case KEY_CTRL_EXE:
        return SELECTOR_RETURN_SELECTION;
        break;
      case KEY_CTRL_EXIT:
        selector->value = initialValue;
        return SELECTOR_RETURN_EXIT;
      case KEY_CHAR_0:
      case KEY_CHAR_1:
      case KEY_CHAR_2:
      case KEY_CHAR_3:
      case KEY_CHAR_4:
      case KEY_CHAR_5:
      case KEY_CHAR_6:
      case KEY_CHAR_7:
      case KEY_CHAR_8:
      case KEY_CHAR_9:
        if(selector->type != SELECTORTYPE_BACKLIGHT_DURATION &&
           selector->type != SELECTORTYPE_LONGDATEFORMAT) {
          // find maximum number of digits allowed
          // limit of a 32-bits number has 10 digits, so even unlimited isn't really unlimited
          int ndigits = 9;
          if(selector->max && selector->max != -1) {
            int v = selector->max;
            for(ndigits = 0; v != 0; ndigits++)
              v /= 10;
          }
          char nvals[25] = "";
          textInput input;
          input.type = INPUTTYPE_NUMERIC;
          input.charlimit=ndigits;
          input.width = ndigits;
          input.x = 5;
          input.y = 5;
          input.key = key;
          input.buffer = (char*)nvals;
          clearLine(5,5);
          if(selector->type == SELECTORTYPE_TIMEOUT_MINUTES) {
            mPrintXY(9, 5, "Minutes", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          }
          int res = doTextInput(&input);
          if (res==INPUT_RETURN_CONFIRM && strlen(nvals)) {
            int nval = atoi(nvals);
            if(nval >= selector->min && (selector->max == -1 || nval <= selector->max)) {
              selector->value = nval;
              if(selector->type == SELECTORTYPE_INSTANT_RETURN) return SELECTOR_RETURN_INSTANT;
            }
          }
        }
        break;
    }
  }
  return SELECTOR_RETURN_EXIT;
}
