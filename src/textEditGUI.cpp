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
#include "debugGUI.hpp"

int doTextEdit(textEdit* edit) {
  int wasInClip=0;
  if (edit->key)
    edit->cursor = EditMBStringChar((unsigned char*)edit->buffer, edit->charlimit, edit->cursor,
                                    edit->key);

  textArea text;
  text.type = TEXTAREATYPE_CURSOR;
  
  textElement elem[1];
  text.elements = elem;
  
  elem[0].text = (char*)edit->buffer;
  
  text.numelements = 1;
  while(1) {
    text.cursor = edit->cursor;
    int renderResult = 0;
    do {
      renderResult = doTextArea(&text);
    } while(renderResult == TEXTAREA_RETURN_REDRAW);
    edit->cursor = text.cursor;

    int keyflag = GetSetupSetting( (unsigned int)0x14);
    if(keyflag == 0x02) {
      // in clip mode
      wasInClip=1;
      drawFkeyLabels(0x0034, 0x0069); // COPY (white), CUT (white)
    } else if(wasInClip) {
      // clear, because we were in clip mode before
      wasInClip=0;
      drawFkeyLabels(0,0); // empty first two
    } else if(keyflag == 0x01) {
      // Shift enabled, show labels
      drawFkeyLabels(0, 0, 0, 0x02A1, 0x0307, 0x0302); // CHAR, A<>a, SAVE
    }

    mGetKey(&edit->key);
    if (GetSetupSetting((unsigned int)0x14) == 0x01 ||
        GetSetupSetting((unsigned int)0x14) == 0x04 ||
        GetSetupSetting( (unsigned int)0x14) == 0x84) {
      keyflag = GetSetupSetting( (unsigned int)0x14); // make sure the flag we're using is the
      // updated one. we can't update always because that way alpha-not-lock will cancel when F5 is
      // pressed.
    }
    if(edit->key == KEY_CTRL_EXE) {
      edit->cursor = EditMBStringChar((unsigned char*)edit->buffer, edit->charlimit,
                                      edit->cursor, '\n');
    } else if(edit->key == KEY_CTRL_EXIT) {
      // Aborted
      Cursor_SetFlashOff();
      return TEXTEDIT_RETURN_EXIT;
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
    } else if(edit->key == KEY_CTRL_UP) {
      text.shadowCursorY -= text.lineHeight;
      text.updateCursor = 1;
    } else if(edit->key == KEY_CTRL_F6) {
      return INPUT_RETURN_CONFIRM;
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
                        &edit->cursor, &edit->key, 1, 3*24-24, 1, 21 );
      // EditMBStringCtrl2 enables the OS cursor, we don't want it
      Cursor_SetFlashOff();
    }
  }
  Cursor_SetFlashOff();
  return TEXTEDIT_RETURN_CONFIRM;
}