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
    int textY = scroll+(text->showtitle ? 24 : 0)+text->y; // 24 pixels for title (or not)
    int temptextY = 0;
    int temptextX = 0;
    while(cur < text->numelements) {
      if(text->elements[cur].newLine) {
        textX=text->x;
        textY=textY+text->lineHeight+text->elements[cur].lineSpacing; 
      }
      int tlen = strlen(text->elements[cur].text);
      unsigned char* singleword = (unsigned char*)malloc(tlen); // because of this, a single text element can't have more bytes than malloc can provide
      unsigned char* src = (unsigned char*)text->elements[cur].text;
      while(*src)
      {
        temptextX = 0;
        src = toksplit(src, ' ', (unsigned char*)singleword, tlen); //break into words; next word
        //check if printing this word would go off the screen, with fake PrintMini drawing:
        if(text->elements[cur].minimini) {
          PrintMiniMini( &temptextX, &temptextY, (unsigned char*)singleword, 0, text->elements[cur].color, 1 );
        } else {
          PrintMini(&temptextX, &temptextY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, text->elements[cur].color, COLOR_WHITE, 0, 0);
        }
        if(temptextX + textX > text->width-6) {
          //time for a new line
          textX=text->x;
          textY=textY+text->lineHeight;
        } //else still fits, print new word normally
        if(text->elements[cur].minimini) {
          PrintMiniMini( &textX, &textY, (unsigned char*)singleword, 0, text->elements[cur].color, 0 );
        } else {
          PrintMini(&textX, &textY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, text->elements[cur].color, COLOR_WHITE, 1, 0);
        }
        //add a space, since it was removed from token
        if(*src || text->elements[cur].spaceAtEnd) PrintMini(&textX, &textY, (unsigned char*)" ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      }
      free(singleword);
      if(isFirstDraw) {
        totalTextY = textY+(text->showtitle ? 0 : 24);
      } else if(textY>LCD_HEIGHT_PX) {
        break;
      }
      cur++;
    }
    isFirstDraw=0;
    if(text->showtitle) {
      clearLine(1,1);
      drawScreenTitle((char*)text->title);
    }
    int scrollableHeight = LCD_HEIGHT_PX-24*(text->showtitle ? 2 : 1)-text->y;
    //draw a scrollbar:
    if(text->scrollbar) {
      TScrollbar sb;
      sb.I1 = 0;
      sb.I5 = 0;
      sb.indicatormaximum = totalTextY;
      sb.indicatorheight = scrollableHeight;
      sb.indicatorpos = -scroll;
      sb.barheight = scrollableHeight;
      sb.bartop = (text->showtitle ? 24 : 0)+text->y;
      sb.barleft = text->width - 6;
      sb.barwidth = 6;

      Scrollbar(&sb);
    }
    if(text->type == TEXTAREATYPE_INSTANT_RETURN) return 0;
    mGetKey(&key);
    
    switch(key)
    {
      case KEY_CTRL_UP:
        if (scroll < 0) {
          scroll = scroll + 17;
          if(scroll > 0) scroll = 0;
        }
        break;
      case KEY_CTRL_DOWN:
        if (textY > scrollableHeight-(text->showtitle ? 0 : 17)) {
          scroll = scroll - 17;
          if(scroll < -totalTextY+scrollableHeight-(text->showtitle ? 0 : 17)) scroll = -totalTextY+scrollableHeight-(text->showtitle ? 0 : 17);
        }
        break;
      case KEY_CTRL_PAGEDOWN:
        if (textY > scrollableHeight-(text->showtitle ? 0 : 17)) {
          scroll = scroll - scrollableHeight;
          if(scroll < -totalTextY+scrollableHeight-(text->showtitle ? 0 : 17)) scroll = -totalTextY+scrollableHeight-(text->showtitle ? 0 : 17);
        }
        break;
      case KEY_CTRL_PAGEUP:
        if (scroll < 0) {
          scroll = scroll + scrollableHeight;
          if(scroll > 0) scroll = 0;
        }
        break;
      case KEY_CTRL_EXE:
        if(text->allowEXE) return TEXTAREA_RETURN_EXE;
        break;
      case KEY_CTRL_F1:
        if(text->allowF1) return TEXTAREA_RETURN_F1;
        break;
      case KEY_CTRL_EXIT: return TEXTAREA_RETURN_EXIT; break;
    }
  }
}