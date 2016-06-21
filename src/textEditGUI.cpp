#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fxcg/display.h>
#include <fxcg/keyboard.h>

#include "textEditGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "inputGUI.hpp"
#include "textGUI.hpp"
#include "stringsProvider.hpp"
#include "menuGUI.hpp"
#include "selectorGUI.hpp"

static const unsigned char symbols[] = {'!', '\\', '"', '#', '$', '%', '&', '\'', ':', ';', '<', '>', '?', '@', '\\', '_', '`', '|', '~'};

int doTextEdit(textEdit* edit) {
  if (edit->key)
    edit->cursor = EditMBStringChar((unsigned char*)edit->buffer, edit->charlimit, edit->cursor,
                                    edit->key);

  textArea text;
  text.type = TEXTAREATYPE_CURSOR;
  
  textElement elem[1];
  text.elements = elem;
  
  elem[0].text = (char*)edit->buffer;
  
  text.numelements = 1;
  int enterClip = 0;
  int defaultStatusMsg = 0;
  while(1) {
    if(!text.updateCursor) {
      if(!defaultStatusMsg) {
        defaultStatusMsg = 1;
        char message[100];
        stringToMini(message, "Press SHIFT to show function menu labels");
        DefineStatusMessage((char*)message, 1, 0, 0);
      }
    } else {
      defaultStatusMsg = 0;
    }
    text.cursor = edit->cursor;
    int renderResult = 0;
    do {
      renderResult = doTextArea(&text);
    } while(renderResult == TEXTAREA_RETURN_REDRAW);
    edit->cursor = text.cursor;
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    if(enterClip) {
      defaultStatusMsg = 0;
      // in clip mode

      // shadow text area
      struct display_fill fill;
      fill.x1 = 0;
      fill.y1 = 24;
      fill.x2 = LCD_WIDTH_PX-1;
      fill.y2 = LCD_HEIGHT_PX-24*2;
      fill.mode = 2;
      Bdisp_AreaClr(&fill, 1, COLOR_LIGHTGRAY);
      // draw UI elements
      if(keyflag == 0x02) {
        drawFkeyLabels(0x0034, 0x0069, 0, 0, 0, 0); // COPY (white), CUT (white)
        DefineStatusMessage((char*)"Select the text to copy or cut", 1, 0, 0);
      } else {
        clearLine(1, 8);
        DefineStatusMessage((char*)"Press Shift+8 to begin selecting", 1, 0, 0);
      }
      
      clearLine(1, 7); // remove aestethically unpleasing bit of background at the end
      DisplayMBString2(0, (unsigned char*)edit->buffer, edit->start, edit->cursor, 0, 1, 7*24-24, 21, 0);
    } else if(keyflag == 0x01) {
      // Shift enabled, show labels
      drawFkeyLabels(0, 0x01FC, 0x018D, 0x02A1, 0x0307, 0x0302); // JUMP, SYMBOL, CHAR, A<>a, SAVE
    }
    mGetKey(&edit->key);
    // remap certain keys/characters
    switch(edit->key) {
      case KEY_CTRL_EXE:
        edit->key = '\n';
        break;
      case KEY_CHAR_SQUARE:
        edit->key = '!';
        break;
      case KEY_CHAR_POW:
        edit->key = '^';
        break;
      case KEY_CHAR_MULT:
        edit->key = '*';
        break;
      case KEY_CHAR_DIV:
        edit->key = '/';
        break;
      case KEY_CHAR_PLUS:
        edit->key = '+';
        break;
      case KEY_CHAR_MINUS:
        edit->key = '-';
        break;
      case KEY_CHAR_PMINUS:
        edit->key = '_';
        break;
      case KEY_CHAR_LN:
        edit->key = '<';
        break;
      case KEY_CHAR_SIN:
        edit->key = '>';
        break;
      case KEY_CHAR_STORE:
        edit->key = ';';
        break;
      case KEY_CHAR_FRAC:
        edit->key = '&';
        break;
      case KEY_CTRL_FD:
        edit->key = '|';
        break;
      case KEY_CTRL_XTT:
        edit->key = '%';
        break;
      case KEY_CHAR_LOG:
        edit->key = '#';
        break;
      case KEY_CHAR_COS:
        edit->key = '\'';
        break;
      case KEY_CHAR_TAN:
        edit->key = '\\';
        break;
    }

    if (GetSetupSetting((unsigned int)0x14) == 0x01 ||
        GetSetupSetting((unsigned int)0x14) == 0x04 ||
        GetSetupSetting( (unsigned int)0x14) == 0x84) {
      keyflag = GetSetupSetting( (unsigned int)0x14); // make sure the flag we're using is the
      // updated one. we can't update always because that way alpha-not-lock will cancel when F5 is
      // pressed.
    }
    if(edit->key == KEY_CTRL_EXIT) {
      Cursor_SetFlashOff();
      if(enterClip) {
        enterClip = 0;
        continue;
      }
      // Aborted
      return TEXTEDIT_RETURN_EXIT;
    } else if(edit->key == KEY_CTRL_F2) {
      int i = 0;
      int cursorline = 0, curline = 1;
      for(; edit->buffer[i]; curline++) {
        int prevWasMBLead = 0;
        for(; (edit->buffer[i] != '\n' && edit->buffer[i]) || prevWasMBLead; i++) {
          prevWasMBLead = isMBfirst(edit->buffer[i]);
        }
        if(!cursorline && edit->cursor <= i) {
          cursorline = curline;
        }
        i++;
      }
      Selector linesel;
      linesel.title = (char*)"Go to line";
      linesel.value = cursorline;
      linesel.min = 1;
      linesel.max = curline-1;
      int res = doSelector(&linesel);
      if (res == SELECTOR_RETURN_SELECTION) {
        i = 0;
        curline = 1;
        for(; edit->buffer[i]; curline++) {
          if(curline == linesel.value) {
            edit->cursor = i;
            break;
          }
          int prevWasMBLead = 0;
          for(; (edit->buffer[i] != '\n' && edit->buffer[i]) || prevWasMBLead; i++) {
            prevWasMBLead = isMBfirst(edit->buffer[i]);
          }
          i++;
        }
      }
    } else if(edit->key == KEY_CTRL_F3) {
      mMsgBoxPush(6);
      char itemtext[20][2];
      MenuItem smallmenuitems[20];
      for(int i = 0; i < 19; i++) {
        itemtext[i][0] = symbols[i];
        itemtext[i][1] = 0;
        smallmenuitems[i].text = itemtext[i];
      }
      
      Menu smallmenu;
      smallmenu.items=smallmenuitems;
      smallmenu.numitems=19;
      smallmenu.width=17;
      smallmenu.height=6;
      smallmenu.startX=3;
      smallmenu.startY=2;
      smallmenu.scrollout=1;
      smallmenu.title = (char*)"Insert symbol";
      int sres = doMenu(&smallmenu);
      mMsgBoxPop();
      
      if(sres == MENU_RETURN_SELECTION) {
        edit->cursor = EditMBStringChar((unsigned char*)edit->buffer, edit->charlimit,
                                        edit->cursor, itemtext[smallmenu.selection-1][0]);
      }
    } else if(edit->key == KEY_CTRL_F4) {
      short character = selectCharacterAux();
      if (character)
        edit->cursor = EditMBStringChar((unsigned char*)edit->buffer, edit->charlimit,
                                         edit->cursor, character);
    } else if(edit->key == KEY_CTRL_F5) {
      // switch between lower and upper-case alpha
      switch(keyflag) {
        case 0x08:
        case 0x88:
          SetSetupSetting((unsigned int)0x14, keyflag-0x04);
          continue; //do not process the key, because otherwise we will leave alpha status
        case 0x04:
        case 0x84:
          SetSetupSetting((unsigned int)0x14, keyflag+0x04);
          continue; //do not process the key, because otherwise we will leave alpha status
      }
    } else if(edit->key == KEY_CTRL_DOWN) {
      text.shadowCursorY += text.lineHeight;
      text.updateCursor = 1;
    } else if(edit->key == KEY_CTRL_PAGEDOWN) {
      text.shadowCursorY += 11*text.lineHeight;
      text.updateCursor = 1;
    } else if(edit->key == KEY_CTRL_UP) {
      text.shadowCursorY -= text.lineHeight;
      text.updateCursor = 1;
    } else if(edit->key == KEY_CTRL_PAGEUP) {
      text.shadowCursorY -= 11*text.lineHeight;
      text.updateCursor = 1;
    } else if(edit->key == KEY_CTRL_F6) {
      return INPUT_RETURN_CONFIRM;
    } else if(edit->key == KEY_CTRL_CLIP) {
      enterClip = 1;
    }
    if(edit->key && edit->key < 30000) {
      if ((GetSetupSetting((unsigned int)0x14) == 0x08 ||
           GetSetupSetting((unsigned int)0x14) == 0x88) &&
          edit->key >= KEY_CHAR_A && edit->key <= KEY_CHAR_Z) //if lowercase and key is char...
        edit->key = edit->key + 32; // to switch to lower-case characters

      edit->cursor = EditMBStringChar((unsigned char*)edit->buffer, edit->charlimit,
                                       edit->cursor, edit->key);
    } else {
      EditMBStringCtrl2((unsigned char*)edit->buffer, edit->charlimit+1, &edit->start,
                        &edit->cursor, &edit->key, 1, 7*24-24, 1, 20 );
      if(keyflag == 0x02) {
        enterClip = 0;
      }
      // EditMBStringCtrl2 enables the OS cursor, we don't want it
      Cursor_SetFlashOff();
    }
  }
  Cursor_SetFlashOff();
  return TEXTEDIT_RETURN_CONFIRM;
}