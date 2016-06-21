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

/* This file contains the bulk of the text and cursor rendering mechanism used in the text areas
 * and in the multi-line text editor. Most of the complexity and branches in the code is due to
 * cursor positioning and rendering.
 * The rendering code is made to support multiple text elements and newlines within these elements.
 * It automatically wraps text to the width of the screen. Extra care was taken to make sure that
 * rendering is correct when the space or newline characters appear the second byte of a multi-byte
 * character.
 *
 * The cursor logic is built with support for multiple ways to position the cursor. The cursor can
 * be positioned through its index in the text (default), or it can be updated graphically through
 * the use of a "shadow cursor" that indicates roughly where the cursor should be in screen
 * coordinates. The real, visible cursor is then positioned to be as close to this shadow cursor as
 * possible. This allows for maintaining the horizontal coordinate of the cursor when the user moves
 * up and down in the text. The shadow cursor is updated automatically when the cursor position is
 * set as an index.
 * To move the cursor up and down, external code should decrease or increase (respectively)
 * shadowCursorY by the height of one line, and set updateCursor to 1 so that the cursor index is
 * recalculated based on the shadow cursor coordinates.
 * The cursor code goes mostly out of the way when not in editing mode. It was written with tight
 * coupling with the editing code in textEditGUI in mind.
 */

// Helper function that draws a string with the 18px or 10px font (micro == 1 -> 10px)
// and updates the y coordinate according to the string length
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
    // clear background
    drawRectangle(text->x, text->y+24, text->width, LCD_HEIGHT_PX-24-text->y, COLOR_WHITE);

    // textX and textY are the coordinates where text is currently being rendered
    // text should only be passed to PrintMini/PrintMiniMini when textY takes values between 0 and
    // LCD_HEIGHT_PX
    int textX = text->x;
    int textY = scroll+(showtitle ? 24 : 0)+text->y; // 24 pixels for title (or not)

    // whether the cursor index is to be recalculated graphically based on the shadow cursor.
    // 0: no need to update; 1: will update; 2: has updated
    int updateCursorIndex = 0; 

    int cursorX = 0, cursorY = 0;
    if(text->type == TEXTAREATYPE_CURSOR) {
      updateCursorIndex = text->updateCursor == 1;
    }
    int cursorLine = 0;
    int lineCount = 1;

    // iterate over each element and render it according to its style flags
    // when editing text, we only expect to see one text element. The code is not prepared to deal
    // with more than one element in text editing mode.
    for(int cur = 0; cur < text->numelements; cur++) {
      if(text->elements[cur].newLine) {
        textX=text->x;
        textY=textY+text->lineHeight+text->elements[cur].lineSpacing;
      }

      int tlen = strlen(text->elements[cur].text);
      // is this element empty? if yes, move on to the next one
      if(!tlen) continue;
      
      const char* src = (char*)text->elements[cur].text;
      // go through the text in this element, doing newlines and updating the cursor as necessary
      while(*src && src < text->elements[cur].text + tlen) {
        // support \n within text elements, disregarding \n that are part of a multi-byte character:
        int linelength = 0, prevWasMBLead = 0;
        for(; (src[linelength] != '\n' && src[linelength]) || prevWasMBLead; linelength++) {
          prevWasMBLead = isMBfirst(src[linelength]);
        }
        char singleword[linelength+2]; // includes space for NULL at the end

        int i = 0; // index in line
        while(i < linelength) {
          int w = 0; // index in word
          prevWasMBLead = 0;
          // copy the next word into singleword, disregarding spaces that are part of a multi-byte
          // character:
          while(((src[i] != ' ' && src[i]) || prevWasMBLead) && i < linelength) {
            prevWasMBLead = isMBfirst(src[i]);
            singleword[w] = src[i];
            i++;
            w++;
          }

          // add space to the end of the word, unless this was the last word in the line:
          // (the space was not copied into singleword in the previous step)
          if(i < linelength) {
            singleword[w++] = ' ';
            i++;
          }

          // text editing - cursor positioning: if the cursor index is not to be recalculated
          // graphically, see if the cursor index falls in this word
          int cursorIndex = -1;
          if(text->type == TEXTAREATYPE_CURSOR && !updateCursorIndex) {
            cursorIndex = text->cursor - (src - text->elements[cur].text) - i + w;
            if(cursorIndex < 0 || cursorIndex >= w) {
              // the cursor is not in this word
              cursorIndex = -1;
            }
          }

          singleword[w] = 0;
          int temptextX = 0, temptextY = 0;
          //check if printing this word would go off the screen, by previewing the length:
          printAreaText(&temptextX, &temptextY, singleword, text->elements[cur].color,
                        text->elements[cur].minimini, 1);

          if(temptextX + textX > text->width-(text->scrollbar ? 6 : 0)) {
            // yes, it would go out of the drawing area. time for a new line

            // text editing - graphical recalculation of the cursor index:
            // if the cursor index is to be recalculated graphically (based on the shadow cursor)
            // and the current drawing coordinates (textX, textY) are "similar" to that of the
            // shadow cursor, calculate the cursor index and coordinates based on the shadow cursor.
            if(updateCursorIndex == 1 && textY >= text->shadowCursorY && textX < text->shadowCursorX) {
              text->cursor = src - text->elements[cur].text + i - w - 1; // -1 to go before the line-breaking space;
              cursorX = textX - 7; // - 7 to appear before the space
              cursorY = textY;
              cursorLine = lineCount;
              // mark cursor as recalculated:
              updateCursorIndex = 2;
            }

            textX=text->x;
            textY=textY+(text->elements[cur].minimini ? -7 : 0)+text->lineHeight;
          }

          // text editing - graphical recalculation of the cursor index:
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
                // the cursor must be on this index.
                cursorIndex = j - !!smallstring[1];
                text->cursor = j + src - text->elements[cur].text + i - w - !!smallstring[1];
                cursorLine = lineCount;
                // signal that cursor is updated:
                updateCursorIndex = 2;
                break;
              }
            }
          }

          // finally, print new word normally (or just update coordinates,
          // if we are not "on stage" yet)
          if(textY >= -24 && textY < LCD_HEIGHT_PX) {
            // we are "on stage". do a real print and show the cursor if needed
            if(cursorIndex == -1) {
              // the cursor is not in this word:
              printAreaText(&textX, &textY, singleword, text->elements[cur].color,
                            text->elements[cur].minimini, 0);
            } else {
              // the cursor is in this word
              // print character by character, in order to find the cursor coordinates
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
            // we are not on stage, just move the current rendering coordinates
            textX += temptextX;

            if(cursorIndex != -1) {
              // cursor is in this word, but this word is not "on stage"
              // this means the current scroll position is inadequate for the cursor position
              // recalculate the scrolling and the position of the shadow cursor
              // (to compensate for the fact that we scrolled) and restart rendering

              if(textY < -24) {
                // above the stage, scroll up
                text->scroll += -24 - textY;
                text->shadowCursorY += -24 - textY;
                return TEXTAREA_RETURN_REDRAW;
              } else if(textY >= LCD_HEIGHT_PX) {
                // below the stage, scroll down
                text->shadowCursorY -= textY - LCD_HEIGHT_PX + 1;
                text->scroll -= textY - LCD_HEIGHT_PX + 1;
                return TEXTAREA_RETURN_REDRAW;
              }
            }
          }
        }
        src += linelength;
        // at this point we dealt with yet another line

        // is there another newline?
        while(*src == '\n') {
          if(updateCursorIndex == 1 && textY >= text->shadowCursorY && textX <= text->shadowCursorX) {
            // graphical calculation of cursor index based on shadow cursor coordinates
            text->cursor = src - text->elements[cur].text;
            // signal that cursor index is updated:
            updateCursorIndex = 2;
          }

          if(text->type == TEXTAREATYPE_CURSOR && text->cursor == src - text->elements[cur].text) {
            // update cursor coordinates since the cursor index corresponds to this newline
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

        // if on self-contained mode (i.e. not instant return or text editing modes),
        // cache the height of the whole rendered text so that scrolling is faster.
        // we can't do this in other modes as we can't guarantee other code won't change the
        // text of the elements between calls
        if(isFirstDraw)
          totalTextY = textY+(showtitle ? 0 : 24)-scroll;
        else if(textY>LCD_HEIGHT_PX)
          break;
      }

      // need to update cursor based on shadow cursor, and we're yet to do it?
      // then cursor must be at the end of the text
      if(updateCursorIndex == 1 && textY <= text->shadowCursorY && textX <= text->shadowCursorX) {
        text->cursor = tlen;
        cursorLine = lineCount;
        updateCursorIndex = 2;
      }

      if(text->cursor >= tlen) {
        // the cursor index is past the end of the text,
        // so set its coordinates of that of the end of the text
        cursorX = textX;
        cursorY = textY;
      }

      if(text->type == TEXTAREATYPE_CURSOR && text->shadowCursorY > textY) {
        // the shadow cursor vertical coordinate is past the end of the text,
        // so set it to the end of the text
        text->shadowCursorY = textY;
        return TEXTAREA_RETURN_REDRAW;
      }

      // if this element is meant to have a space at the end, add it, or at least increment textX
      // to account for it
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

    // take care of the scrollbar:
    int scrollableHeight = LCD_HEIGHT_PX-24*(showtitle ? 2 : 1)-text->y;
    
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
    // end of the scrollbar code

    // if editing text, maybe update the coordinates of the shadow cursor
    // and update the line number indicator
    if(text->type == TEXTAREATYPE_CURSOR) {
      if(updateCursorIndex != 1) {
        text->shadowCursorY = cursorY;
      }
      if(!text->updateCursor)
        text->shadowCursorX = cursorX;
      else {
        if(cursorLine < 1) cursorLine = lineCount;
        char message[25];
        sprintf(message, (char*)"Line %d of %d", cursorLine, lineCount);
        DefineStatusMessage(message, 1, 0, 0);
      }
      text->updateCursor = 2;

      if(cursorY > LCD_HEIGHT_PX - text->lineHeight - 24) {
        // cursor is past the end of the "stage"
        // this means the current scrolling position is inadequate.
        // recalculate scrolling and return for redrawing
        text->scroll -= cursorY - (LCD_HEIGHT_PX - text->lineHeight - 24);

        if(scroll < -totalTextY+scrollableHeight-(showtitle ? 0 : 17))
            scroll = -totalTextY+scrollableHeight-(showtitle ? 0 : 17);
        
        return TEXTAREA_RETURN_REDRAW;
      } else if(cursorY < 0) {
        // cursor is before the start of the "stage"
        // recalculate scrolling and return for redrawing
        text->scroll += -cursorY;
        
        if(scroll > 0) scroll = 0;
        
        return TEXTAREA_RETURN_REDRAW;
      }

      text->updateCursor = 0;
      drawLine(cursorX, cursorY + 24, cursorX, cursorY + 15 + 24, COLOR_BLACK);
      drawLine(cursorX+1, cursorY + 24, cursorX+1, cursorY + 15 + 24, COLOR_BLACK);
    }
    // if we're editing text or if configured to not handle keys, return
    if(text->type >= TEXTAREATYPE_INSTANT_RETURN) return 0;

    // looks like we're working in self-contained mode.
    // act like a stand-alone control and take care of scrolling and return codes:
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