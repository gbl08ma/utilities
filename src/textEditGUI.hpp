#ifndef __TEXTEDITGUI_H
#define __TEXTEDITGUI_H

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

#define TEXTEDIT_RETURN_EXIT 0
#define TEXTEDIT_RETURN_CONFIRM 1
#define TEXTEDIT_RETURN_KEYCODE 2

typedef struct {
  int x=0; // x and y are in pixels
  int y=0;
  int width=LCD_WIDTH_PX;
  int charlimit; // maximum number of chars to admit in bytes (which means that if users enter multibyte it will allow for less chars)
  int key=0; // put a key code here to provide for the initial keypress. also, when returning TEXTEDIT_RETURN_KEYCODE, the keycode is here.
  int cursor=0;
  int start=0;
  char* buffer;
} textEdit;

int doTextEdit(textEdit* edit);

#endif