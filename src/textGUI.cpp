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

typedef scrollbar TScrollbar;

void printAreaText(int* x, int* y, const char* string, color_t color, int micro, int preview) {
  if(micro) PrintMiniMini(x, y, string, 0, color, preview);
  else PrintMini(x, y, string, 0, 0xFFFFFFFF, 0, 0, color, COLOR_WHITE, !preview, 0);
}
int doTextArea(textArea* text) {
  int scroll = text->scroll;
  int isFirstDraw = 1;
  int totalTextY = 0;
  int key;
  int showtitle = text->title != NULL;
  while(1) {
    drawRectangle(text->x, text->y+24, text->width, LCD_HEIGHT_PX-24-text->y, COLOR_WHITE);
    int textX = text->x;
    int textY = scroll+(showtitle ? 24 : 0)+text->y; // 24 pixels for title (or not)
    int updateCursorIndex = 0; // whether the cursor index is to be recalculated graphically based on the shadow cursor.
    // 0: no need to update; 1: will update; 2: has updated

    int cursorX = 0, cursorY = 0;
    if(text->type == TEXTAREATYPE_CURSOR) {
      updateCursorIndex = text->updateCursor == 1;
    }
    int cursorLine = 0;
    int lineCount = 1;
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
        int linelength = 0, prevWasMBLead = 0;
        for(; (src[linelength] != '\n' && src[linelength]) || prevWasMBLead; linelength++) {
          prevWasMBLead = isMBfirst(src[linelength]);
        }
        char singleword[linelength+2]; // +1 for NULL at the end
        int i = 0; // index in line
        while(i < linelength) {
          int w = 0; // index in word
          prevWasMBLead = 0;
          while(((src[i] != ' ' && src[i]) || prevWasMBLead) && i < linelength) {
            prevWasMBLead = isMBfirst(src[i]);
            singleword[w] = src[i];
            i++;
            w++;
          }
          if(i < linelength) {
            // add space which was removed from token
            singleword[w++] = ' ';
            i++;
          }
          int cursorIndex = -1;
          if(text->type == TEXTAREATYPE_CURSOR && !updateCursorIndex) {
            cursorIndex = text->cursor - (src - text->elements[cur].text) - i + w;
            if(cursorIndex < 0 || cursorIndex >= w) {
              cursorIndex = -1;
            }
          }
          singleword[w] = 0;
          int temptextX = 0, temptextY = 0;
          //check if printing this word would go off the screen, by previewing the length:
          printAreaText(&temptextX, &temptextY, singleword, text->elements[cur].color,
                        text->elements[cur].minimini, 1);
          if(temptextX + textX > text->width-(text->scrollbar ? 6 : 0)) {
            //time for a new line

            if(updateCursorIndex == 1 && textY >= text->shadowCursorY && textX < text->shadowCursorX) {
              text->cursor = src - text->elements[cur].text + i - w - 1; // -1 to go before the line-breaking space;
              cursorX = textX - 7; // - 7 to appear before the space
              cursorY = textY;
              cursorLine = lineCount;
              updateCursorIndex = 2;
            }

            textX=text->x;
            textY=textY+(text->elements[cur].minimini ? -7 : 0)+text->lineHeight;
          }

          if(updateCursorIndex == 1 && textY >= text->shadowCursorY && textX+temptextX >= text->shadowCursorX) {
            // graphical calculation of cursor position based on shadow cursor coordinates
            // now we know the cursor index must be located in the previous word
            // print character by character, in order to find the cursor position
            char smallstring[3];
            int x = textX, y = textY;
            for(int j = 0; j < w; j++) {
              smallstring[0] = singleword[j];
              smallstring[1] = (isMBfirst(singleword[j]) && j < w - 1 ? singleword[++j] : 0);
              smallstring[2] = 0;
              printAreaText(&x, &y, smallstring, text->elements[cur].color,
                            text->elements[cur].minimini, 0);
              if(x > text->shadowCursorX) {
                // the cursor must be on this index
                cursorIndex = j - !!smallstring[1];
                text->cursor = j + src - text->elements[cur].text + i - w - !!smallstring[1];
                cursorLine = lineCount;
                updateCursorIndex = 2;
                break;
              }
            }
          }
          // print new word normally
          // (or just increment textX, if we are not "on stage" yet)
          if(textY >= -24 && textY < LCD_HEIGHT_PX) {
            if(cursorIndex == -1) {
              printAreaText(&textX, &textY, singleword, text->elements[cur].color,
                            text->elements[cur].minimini, 0);
            } else {
              // print character by character, in order to find the cursor position
              char smallstring[3];
              for(int j = 0; j < w; j++) {
                if(j == cursorIndex) {
                  cursorX = textX;
                  cursorY = textY;
                  cursorLine = lineCount;
                }
                smallstring[0] = singleword[j];
                smallstring[1] = (isMBfirst(singleword[j]) && j < w - 1 ? singleword[++j] : 0);
                smallstring[2] = 0;
                printAreaText(&textX, &textY, smallstring, text->elements[cur].color,
                              text->elements[cur].minimini, 0);
              }
            }
          } else {
            textX += temptextX;
            if(cursorIndex != -1) {
              // cursor was in this word, but it is not on screen
              if(textY < -24) {
                text->scroll += text->lineHeight;
                return TEXTAREA_RETURN_REDRAW;
              } else if(textY >= LCD_HEIGHT_PX) {
                text->scroll -= text->lineHeight;
                return TEXTAREA_RETURN_REDRAW;
              }
            }
          }
        }
        src += linelength;
        while(*src == '\n') {
          if(updateCursorIndex == 1 && textY >= text->shadowCursorY && textX <= text->shadowCursorX) {
            // graphical calculation of cursor position based on shadow cursor coordinates
            text->cursor = src - text->elements[cur].text;
            updateCursorIndex = 2;
          }
          if(text->type == TEXTAREATYPE_CURSOR && text->cursor == src - text->elements[cur].text) {
            cursorLine = lineCount;
            cursorX = textX;
            cursorY = textY;
          }
          // there's more text in the element, new line
          textX=text->x;
          textY=textY+(text->elements[cur].minimini ? -7 : 0)+text->lineHeight;
          lineCount++;
          src++;
        }
        if(isFirstDraw)
          totalTextY = textY+(showtitle ? 0 : 24)-scroll;
        else if(textY>LCD_HEIGHT_PX)
          break;
      }
      if(updateCursorIndex == 1 && textY <= text->shadowCursorY && textX <= text->shadowCursorX) {
        // graphical calculation of cursor position based on shadow cursor coordinates
        text->cursor = tlen;
        cursorLine = lineCount;
        updateCursorIndex = 2;
      }
      if(text->cursor >= tlen) {
        cursorX = textX;
        cursorY = textY;
      }
      if(text->type == TEXTAREATYPE_CURSOR && text->shadowCursorY > textY) {
        text->shadowCursorY = textY;
        return TEXTAREA_RETURN_REDRAW;
      }

      if(text->elements[cur].spaceAtEnd) {
        if(textY >= -24 && textY < LCD_HEIGHT_PX) {
          printAreaText(&textX, &textY, " ", text->elements[cur].color,
                        text->elements[cur].minimini, 0);
        } else {
           textX += 7; // size of a PrintMini space*/
        }
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
    if(text->type == TEXTAREATYPE_CURSOR) {
      if(updateCursorIndex != 1) {
        text->shadowCursorY = cursorY;
      }
      if(!text->updateCursor)
        text->shadowCursorX = cursorX;
      else {
        if(cursorLine < 1) cursorLine = 1;
        char message[25];
        sprintf(message, (char*)"Line %d of %d", cursorLine, lineCount);
        DefineStatusMessage(message, 1, 0, 0);
      }
      text->updateCursor = 2;
      if(cursorY > LCD_HEIGHT_PX - text->lineHeight - 24) {
        text->scroll -= text->lineHeight;
        return TEXTAREA_RETURN_REDRAW;
      } else if(cursorY < 0) {
        text->scroll += text->lineHeight;
        return TEXTAREA_RETURN_REDRAW;
      }
      text->updateCursor = 0;
      drawLine(cursorX, cursorY + 24, cursorX, cursorY + 15 + 24, COLOR_BLACK);
      drawLine(cursorX+1, cursorY + 24, cursorX+1, cursorY + 15 + 24, COLOR_BLACK);
    }
    if(text->type >= TEXTAREATYPE_INSTANT_RETURN) return 0;
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