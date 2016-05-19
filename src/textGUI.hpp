#ifndef __TEXTGUI_H
#define __TEXTGUI_H
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

typedef struct
{
  char* text;
  color_t color=COLOR_BLACK;
  char newLine=0; // if 1, new line will be drawn before the text
  char spaceAtEnd=0;
  char lineSpacing=0;
  char minimini=0;
} textElement;

#define TEXTAREATYPE_NORMAL 0
#define TEXTAREATYPE_INSTANT_RETURN 1
#define TEXTAREATYPE_CURSOR 2 // like instant return, but with a cursor. the fields change meaning
typedef struct
{
  int x=0;
  int y=0;
  int width=LCD_WIDTH_PX;
  int lineHeight=17;
  textElement* elements;
  int numelements;
  char* title = NULL;
  int scrollbar=1;
  int allowEXE=0; //whether to allow EXE to exit the screen
  int allowLeft=0; //whether pressing Left exits the screen like EXIT.
  int allowF1=0; //whether to allow F1 to exit the screen
  int type=TEXTAREATYPE_NORMAL;
  // fields below are for TEXTAREATYPE_CURSOR only:
  int shadowCursorX=0;
  int shadowCursorY=0;
  int scroll = 0; // used to set scroll when type is TEXTAREATYPE_CURSOR 
  int cursor = 0; // cursor index
  int updateCursor = 0; // for TEXTAREATYPE_CURSOR

} textArea;

#define TEXTAREA_RETURN_EXIT 0
#define TEXTAREA_RETURN_EXE 1
#define TEXTAREA_RETURN_F1 2
#define TEXTAREA_RETURN_REDRAW 3
int doTextArea(textArea* text); //returns 0 when user EXITs, 1 when allowEXE is true and user presses EXE, 2 when allowF1 is true and user presses F1.

#endif 
