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
  int newLine=0; // if 1, new line will be drawn before the text
  color_t color=COLOR_BLACK;
  int spaceAtEnd=0;
  int lineSpacing=0;
} textElement;

typedef struct
{
  int x=0;
  int y=0;
  int width=LCD_WIDTH_PX;
  //int height=LCD_HEIGHT_PX-24;
  int lineHeight=17;
  textElement* elements;
  int numelements;
  char title[42];
  int showtitle = 1;
  int scrollbar=1;
  int allowEXE=0; //whether to allow EXE to exit the screen
  int allowF1=0; //whether to allow F1 to exit the screen
} textArea;

int doTextArea(textArea* text); //returns 0 when user EXITs, 1 when allowEXE is true and user presses EXE, 2 when allowF1 is true and user presses F1.

#endif 
