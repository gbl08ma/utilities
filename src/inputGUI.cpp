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

#include "inputGUI.hpp"
#include "menuGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "settingsProvider.hpp"
#include "debugGUI.hpp"

short selectCharacterAux() {
  unsigned short VRAMbuffer[LCD_WIDTH_PX*(LCD_HEIGHT_PX-24)];
  MsgBoxMoveWB(VRAMbuffer, 0, 0, LCD_WIDTH_PX-1, LCD_HEIGHT_PX-24-1, 1);
  Bkey_ClrAllFlags();
  short character = CharacterSelectDialog();
  MsgBoxMoveWB(VRAMbuffer, 0, 0, LCD_WIDTH_PX-1, LCD_HEIGHT_PX-24-1, 0);
  return character;
}

int doTextInput(textInput* input) {
  if(input->type==INPUTTYPE_NORMAL) {
    drawFkeyLabels(-1, -1, -1, (input->symbols? 0x02A1 : -1), 0x0307); // CHAR, A<>a
  }
  int wasInClip=0;
  if (input->key) { input->cursor = EditMBStringChar((unsigned char*)input->buffer, input->charlimit, input->cursor, input->key); }
  int widthForSyscalls = input->width;
  if(input->width == 21) {
    widthForSyscalls = 20;
    clearLine(1, input->y); // remove aestethically unpleasing bit of background at the end of the field
  }
  while(1) {
    if(input->forcetext && strlen(input->buffer)==0) {
      input->buffer[0]='\xd8';
      input->buffer[1]='\x0';
    }
    DisplayMBString2( 0, (unsigned char*)input->buffer, input->start, input->cursor, 0, input->x, input->y*24-24, widthForSyscalls+input->x, (input->width==21? 0 : 1) );
    
    drawLine(input->x*18-18, input->y*24-1, (input->width==21?LCD_WIDTH_PX-1:input->width*18+input->x*18-18-1), input->y*24-1, COLOR_GRAY);
    drawLine(input->x*18-18, input->y*24+23, (input->width==21?LCD_WIDTH_PX-1:input->width*18+input->x*18-18-1), input->y*24+23, COLOR_GRAY);
    if(input->width != 21) {
      //vertical lines, start and end
      drawLine(input->x*18-18, input->y*24-1, input->x*18-18, input->y*24+23, COLOR_GRAY);
      drawLine((input->x*18-18)+18*input->width, input->y*24-1, (input->x*18-18)+18*input->width, input->y*24+23, COLOR_GRAY);
    }
    if(input->type==INPUTTYPE_DATE) {
      //vertical lines: dd, mm and yyyy separators
      switch(GetSetting(SETTING_DATEFORMAT)) {
        case 0:
        case 1:
          drawLine((input->x*18-18)+18*2, input->y*24-1, (input->x*18-18)+18*2, input->y*24+22, COLOR_GRAY);
          drawLine((input->x*18-18)+18*4+1, input->y*24-1, (input->x*18-18)+18*4+1, input->y*24+23, COLOR_GRAY);
          break;
        case 2:
          drawLine((input->x*18-18)+18*4, input->y*24-1, (input->x*18-18)+18*4, input->y*24+22, COLOR_GRAY);
          drawLine((input->x*18-18)+18*6+1, input->y*24-1, (input->x*18-18)+18*6+1, input->y*24+23, COLOR_GRAY);
          break;
      }
    } else if(input->type==INPUTTYPE_TIME) {
      //vertical lines: hh, mm and ss separators
      drawLine((input->x*18-18)+18*2, input->y*24-1, (input->x*18-18)+18*2, input->y*24+23, COLOR_GRAY);
      drawLine((input->x*18-18)+18*4, input->y*24-1, (input->x*18-18)+18*4, input->y*24+23, COLOR_GRAY);
    }
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    if(input->type==INPUTTYPE_NORMAL) {
      if(keyflag == 0x02) {
        // in clip mode
        wasInClip=1;
        drawFkeyLabels(0x0034, 0x0069); // COPY (white), CUT (white)
      } else if(wasInClip) {
        // clear, because we were in clip mode before
        wasInClip=0;
        drawFkeyLabels(0,0); // empty first two
      }
    }
    mGetKey(&input->key);
    if (GetSetupSetting( (unsigned int)0x14) == 0x01 || GetSetupSetting( (unsigned int)0x14) == 0x04 || GetSetupSetting( (unsigned int)0x14) == 0x84) {
      keyflag = GetSetupSetting( (unsigned int)0x14); //make sure the flag we're using is the updated one.
      //we can't update always because that way alpha-not-lock will cancel when F5 is pressed.
    }
    if(input->key == KEY_CTRL_EXE || (input->key == KEY_CTRL_F6 && input->acceptF6)) {
      // Next step
      if(!input->forcetext || (strlen((char*)input->buffer) > 0 && input->buffer[0]!='\xd8')) {
        // input can be empty, or it already has some text
        Cursor_SetFlashOff(); return INPUT_RETURN_CONFIRM;
      } else {
        mMsgBoxPush(4);
        mPrintXY(3, 2, (char*)"Field can't be", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(3, 3, (char*)"left blank.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        closeMsgBox();
      }
    } else if(input->key == KEY_CTRL_EXIT) {
      // Aborted
      Cursor_SetFlashOff(); return INPUT_RETURN_EXIT;
    } else if(input->key == KEY_CTRL_F1 || input->key == KEY_CTRL_F2) {  
      Cursor_SetFlashOff(); return INPUT_RETURN_KEYCODE;
    } else if(input->key == KEY_CTRL_F4 && input->type == INPUTTYPE_NORMAL) {
      short character = selectCharacterAux();
      if (character) input->cursor = EditMBStringChar((unsigned char*)input->buffer, input->charlimit, input->cursor, character);
    } else if(input->key == KEY_CTRL_F5 && input->type == INPUTTYPE_NORMAL) {
      // switch between lower and upper-case alpha
      switch(keyflag) {
        case 0x08:
        case 0x88:
          SetSetupSetting( (unsigned int)0x14, keyflag-0x04);
          continue; //do not process the key, because otherwise we will leave alpha status
        case 0x04:
        case 0x84:
          SetSetupSetting( (unsigned int)0x14, keyflag+0x04);
          continue; //do not process the key, because otherwise we will leave alpha status
      }
    } 
    if(input->key && input->key < 30000) {
      if(input->type == INPUTTYPE_NORMAL || (input->key >= KEY_CHAR_0 && input->key <= KEY_CHAR_9) ||
        (input->key == KEY_CHAR_DP && !input->symbols && input->type == INPUTTYPE_NUMERIC)) { // either a normal input, or only allow digits and eventually the decimal separator
        if ((GetSetupSetting( (unsigned int)0x14) == 0x08 || GetSetupSetting( (unsigned int)0x14) == 0x88) && input->key >= KEY_CHAR_A && input->key <= KEY_CHAR_Z) //if lowercase and key is char...
          input->key = input->key + 32; // to switch to lower-case characters

        input->cursor = EditMBStringChar((unsigned char*)input->buffer, input->charlimit, input->cursor, input->key);
      }
    } else EditMBStringCtrl2( (unsigned char*)input->buffer, input->charlimit+1, &input->start, &input->cursor, &input->key, input->x, input->y*24-24, 1, widthForSyscalls+input->x-1 );
    if(input->key == KEY_CTRL_PASTE) {
      // at this point it will have already pasted
      int pos = strlen(input->buffer)-1;
      if(input->forcetext && pos > 0 && input->buffer[pos]=='\xd8') {
        input->buffer[pos]='\x0';
      }
    }
  }
  Cursor_SetFlashOff();
  return INPUT_RETURN_CONFIRM;
}