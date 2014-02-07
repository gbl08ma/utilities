#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#ifndef __SELECTORGUI_H
#define __SELECTORGUI_H

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

#define SELECTORTYPE_NORMAL 0
#define SELECTORTYPE_MONTH 1
#define SELECTORTYPE_LONGDATEFORMAT 2
#define SELECTORTYPE_STARTUP_BRIGHTNESS 3
#define SELECTORTYPE_BACKLIGHT_DURATION 4
#define SELECTORTYPE_TIMEOUT_MINUTES 5
#define SELECTORTYPE_INSTANT_RETURN 6 // this type of selector insantly returns even if user hasn't confirmed a value (allows for e.g. redrawing the GUI behind it). if user hasn't exited or confirmed a value, selector will return SELECTOR_RETURN_INSTANT

typedef struct
{
  char title[42]; // text shown at the top, in blue
  char subtitle[42]; // text shown before the title, in black
  int value=0; // value of the selector
  int max; // maximum value. -1 for unlimited
  int min=0; // minimum value
  int cycle=1; //1: when value is max, pressing up key sets value as min (and vice-versa)
  int type=SELECTORTYPE_NORMAL; // set using SELECTORTYPE_*
  int clearVRAM=1; // 1: clears all screen before drawing; 0: only writes on screen areas that are changed (and VRAM clearing must be done by something else)
} Selector;

#define SELECTOR_RETURN_EXIT 0
#define SELECTOR_RETURN_SELECTION 1
#define SELECTOR_RETURN_INSTANT 2
int doSelector(Selector* selector);

#endif
