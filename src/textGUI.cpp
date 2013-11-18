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

#include "timeProvider.hpp"
#include "textGUI.hpp"
#include "stringsProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "selectorGUI.hpp"

int doTextArea(textArea* text) {
  int scroll = 0;
  int isFirstDraw = 1;
  int totalTextY = 0;
  int key;
  while(1) {
    drawRectangle(text->x, text->y+24, text->width, LCD_HEIGHT_PX-24, COLOR_WHITE);
    int cur = 0;
    int textX = text->x;
    int textY = scroll+(text->showtitle ? 24 : 0); // 24 pixels for title (or not)
    int temptextY = 0;
    int temptextX = 0;
    while(cur <= text->numelements-1) {
      if(text->elements[cur].newLine) {
        textX=text->x;
        textY=textY+text->lineHeight+text->elements[cur].lineSpacing; 
      }
      unsigned char* singleword = (unsigned char*)malloc(strlen(text->elements[cur].text)); // because of this, a single text element can't have more bytes than malloc can provide
      unsigned char* src = (unsigned char*)text->elements[cur].text;
      while(*src)
      {
        temptextX = 0;
        src = toksplit(src, ' ', (unsigned char*)singleword, strlen(text->elements[cur].text)); //break into words; next word
        //check if printing this word would go off the screen, with fake PrintMini drawing:
        PrintMini(&temptextX, &temptextY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, text->elements[cur].color, COLOR_WHITE, 0, 0);
        if(temptextX + textX > text->width-6) {
          //time for a new line
          textX=text->x;
          textY=textY+text->lineHeight;
          PrintMini(&textX, &textY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, text->elements[cur].color, COLOR_WHITE, 1, 0);
        } else {
          //still fits, print new word normally
          PrintMini(&textX, &textY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, text->elements[cur].color, COLOR_WHITE, 1, 0);
        }
        //add a space, since it was removed from token
        if(*src || text->elements[cur].spaceAtEnd) PrintMini(&textX, &textY, (unsigned char*)" ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      }
      free(singleword);
      if(isFirstDraw) {
        totalTextY = textY;
      } else {
        if(textY>LCD_HEIGHT_PX) {
          break;
        }
      }
      cur++;
    }
    isFirstDraw=0;
    if(text->showtitle) {
      clearLine(1,1);
      mPrintXY(1, 1, (char*)text->title, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    }
    int scrollableHeight = LCD_HEIGHT_PX-24*(text->showtitle ? 2 : 1);
    //draw a scrollbar:
    if(text->scrollbar) {
      TScrollbar sb;
      sb.I1 = 0;
      sb.I5 = 0;
      sb.indicatormaximum = totalTextY;
      sb.indicatorheight = scrollableHeight;
      sb.indicatorpos = -scroll;
      sb.barheight = scrollableHeight;
      sb.bartop = (text->showtitle ? 24 : 0);
      sb.barleft = text->width - 6;
      sb.barwidth = 6;

      Scrollbar(&sb);
    }
    if(text->type == TEXTAREATYPE_INSTANT_RETURN) return 0;
    if(text->allowMkey) mGetKey(&key);
    else GetKey(&key);
    
    switch(key)
    {
      case KEY_CTRL_UP:
        if (scroll < 0) {
          scroll = scroll + 17;
          if(scroll > 0) scroll = 0;
        }
        break;
      case KEY_CTRL_DOWN:
        if (textY > scrollableHeight) {
          scroll = scroll - 17;
          if(scroll < -totalTextY+scrollableHeight) scroll = -totalTextY+scrollableHeight;
        }
        break;
      case KEY_CTRL_EXE:
        if(text->allowEXE) return 1;
        break;
      case KEY_CTRL_F1:
        if(text->allowF1) return 2;
        break;
      case KEY_CTRL_EXIT: return 0; break;
    }
  }
}