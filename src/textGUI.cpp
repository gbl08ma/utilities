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

#include "textGUI.hpp"
#include "stringsProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"

#include "debugGUI.hpp"

typedef scrollbar TScrollbar;

void printAreaText(int* x, int* y, const char* string, color_t color, int micro, int preview) {
  if(micro) PrintMiniMini(x, y, string, 0, color, preview);
  else PrintMini(x, y, string, 0, 0xFFFFFFFF, 0, 0, color, COLOR_WHITE, !preview, 0);
}
int doTextArea(textArea* text) {
  int scroll = 0;
  int isFirstDraw = 1;
  int totalTextY = 0;
  int key;
  int showtitle = text->title != NULL;
  while(1) {
    drawRectangle(text->x, text->y+24, text->width, LCD_HEIGHT_PX-24-text->y, COLOR_WHITE);
    int textX = text->x;
    int textY = scroll+(showtitle ? 24 : 0)+text->y; // 24 pixels for title (or not)
    for(int cur = 0; cur < text->numelements; cur++) {
      if(text->elements[cur].newLine) {
        textX=text->x;
        textY=textY+text->lineHeight+text->elements[cur].lineSpacing; 
      }
      int tlen = strlen(text->elements[cur].text);
      if(!tlen) continue;
      const char* src = (char*)text->elements[cur].text;
      while(*src && src < text->elements[cur].text + tlen) {
        // support \n within text elements:
        int linelength = 0;
        for(; src[linelength] != '\n' && src[linelength]; linelength++);
        char singleword[linelength+1]; // +1 for NULL at the end
        int i = 0;
        while(i < linelength) {
          int w = 0;
          while(src[i] != ' ' && src[i] && i < linelength) {
            singleword[w] = src[i];
            i++;
            w++;
          }
          if(i < linelength) i++;
          singleword[w] = 0;
          int temptextX = 0, temptextY = 0;
          //check if printing this word would go off the screen, by previewing the length:
          printAreaText(&temptextX, &temptextY, singleword, text->elements[cur].color,
                        text->elements[cur].minimini, 1);
          if(temptextX + textX > text->width-(text->scrollbar ? 6 : 0)) {
            //time for a new line
            textX=text->x;
            textY=textY+(text->elements[cur].minimini ? -7 : 0)+text->lineHeight;
          }
          // else still fits, print new word normally
          // (or just increment textX, if we are not "on stage" yet)

          if(textY >= -24 && textY < LCD_HEIGHT_PX) {
            printAreaText(&textX, &textY, singleword, text->elements[cur].color,
                          text->elements[cur].minimini, 0);
            //add a space, since it was removed from token
            if(i < linelength || text->elements[cur].spaceAtEnd)
              printAreaText(&textX, &textY, " ", text->elements[cur].color,
                            text->elements[cur].minimini, 0);
          } else {
            textX += temptextX;
            if(i < linelength || text->elements[cur].spaceAtEnd)
              textX += 7; // size of a PrintMini space
          }
        }
        src += linelength;
        while(*src == '\n') {
          // there's more text in the element, new line
          textX=text->x;
          textY=textY+(text->elements[cur].minimini ? -7 : 0)+text->lineHeight;
          src++;
        }
        if(isFirstDraw)
          totalTextY = textY+(showtitle ? 0 : 24);
        else if(textY>LCD_HEIGHT_PX)
          break;
      }
    }
    isFirstDraw=0;
    if(showtitle) {
      clearLine(1,1);
      drawScreenTitle(text->title);
    }
    int scrollableHeight = LCD_HEIGHT_PX-24*(showtitle ? 2 : 1)-text->y;
    //draw a scrollbar:
    if(text->scrollbar && totalTextY > scrollableHeight) {
      TScrollbar sb;
      sb.I1 = 0;
      sb.I5 = 0;
      sb.indicatormaximum = totalTextY;
      sb.indicatorheight = scrollableHeight;
      sb.indicatorpos = -scroll;
      sb.barheight = scrollableHeight;
      sb.bartop = (showtitle ? 24 : 0)+text->y;
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
        if (textY > scrollableHeight-(showtitle ? 0 : 17)) {
          scroll = scroll - 17;
          if(scroll < -totalTextY+scrollableHeight-(showtitle ? 0 : 17))
            scroll = -totalTextY+scrollableHeight-(showtitle ? 0 : 17);
        }
        break;
      case KEY_CTRL_PAGEDOWN:
        if (textY > scrollableHeight-(showtitle ? 0 : 17)) {
          scroll = scroll - scrollableHeight;
          if(scroll < -totalTextY+scrollableHeight-(showtitle ? 0 : 17))
            scroll = -totalTextY+scrollableHeight-(showtitle ? 0 : 17);
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
      case KEY_CTRL_LEFT:
        if(!text->allowLeft) break;
      case KEY_CTRL_EXIT: return TEXTAREA_RETURN_EXIT; break;
    }
  }
}