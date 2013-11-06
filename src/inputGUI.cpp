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
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "settingsProvider.hpp" 

int doTextInput(textInput* input) {
  int iresult;
  if(input->symbols && input->type==INPUTTYPE_NORMAL) {
    GetFKeyPtr(0x02A1, &iresult); // CHAR
    FKey_Display(3, (int*)iresult);
  }
  if(input->type==INPUTTYPE_NORMAL) {
    GetFKeyPtr(0x0307, &iresult); // A<>a
    FKey_Display(4, (int*)iresult);
  }

  if (input->key) { input->cursor = EditMBStringChar((unsigned char*)input->buffer, input->charlimit, input->cursor, input->key); }
  
  while(1)
  {
    DisplayMBString2( 0, (unsigned char*)input->buffer, input->start, input->cursor, 0, input->x, input->y*24-24, input->width+input->x, 0 );
    
    drawLine(input->x*18-18, input->y*24-1, (input->width==21?LCD_WIDTH_PX-1:input->width*18+input->x*18-18-1), input->y*24-1, COLOR_GRAY);
    drawLine(input->x*18-18, input->y*24+23, (input->width==21?LCD_WIDTH_PX-1:input->width*18+input->x*18-18-1), input->y*24+23, COLOR_GRAY);
    if(input->type==INPUTTYPE_DATE) {
      //vertical lines, start and end
      drawLine(input->x*18-18, input->y*24-1, input->x*18-18, input->y*24+23, COLOR_GRAY);
      drawLine((input->x*18-18)+18*input->width, input->y*24-1, (input->x*18-18)+18*input->width, input->y*24+23, COLOR_GRAY);
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
    }
    if(input->type==INPUTTYPE_TIME) {
      //vertical lines, start and end
      drawLine(input->x*18-18, input->y*24-1, input->x*18-18, input->y*24+23, COLOR_GRAY);
      drawLine((input->x*18-18)+18*input->width, input->y*24-1, (input->x*18-18)+18*input->width, input->y*24+23, COLOR_GRAY);
      //vertical lines: hh, mm and ss separators
      drawLine((input->x*18-18)+18*2, input->y*24-1, (input->x*18-18)+18*2, input->y*24+23, COLOR_GRAY);
      drawLine((input->x*18-18)+18*4, input->y*24-1, (input->x*18-18)+18*4, input->y*24+23, COLOR_GRAY);
    }
  
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    if(input->allowMkey) mGetKey(&input->key); else GetKey(&input->key);
    if (GetSetupSetting( (unsigned int)0x14) == 0x01 || GetSetupSetting( (unsigned int)0x14) == 0x04 || GetSetupSetting( (unsigned int)0x14) == 0x84) {
      keyflag = GetSetupSetting( (unsigned int)0x14); //make sure the flag we're using is the updated one.
      //we can't update always because that way alpha-not-lock will cancel when F5 is pressed.
    }
    if(input->key == KEY_CTRL_EXE || (input->key == KEY_CTRL_F6 && input->acceptF6))
    {
      // Next step
      if(input->forcetext) {
        if (strlen((char*)input->buffer) > 0) {
          Cursor_SetFlashOff(); return INPUT_RETURN_CONFIRM;
        } else {
          MsgBoxPush(3);
          mPrintXY(3, 3, (char*)"Field can't be", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 4, (char*)"left blank.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
          int inscreen=1;
          while(inscreen) {
            if(input->allowMkey) mGetKey(&input->key); else GetKey(&input->key);
            switch(input->key)
            {
              case KEY_CTRL_EXIT:
              case KEY_CTRL_AC:
                inscreen=0;
                break;
            }
          }
          MsgBoxPop(); 
        }
      } else {
        Cursor_SetFlashOff(); return INPUT_RETURN_CONFIRM;
      }
    } else if(input->key == KEY_CTRL_EXIT) {
      // Aborted
      Cursor_SetFlashOff(); return INPUT_RETURN_EXIT;
    } else if(input->key == KEY_CTRL_F1 || input->key == KEY_CTRL_F2) {  
      Cursor_SetFlashOff(); return INPUT_RETURN_KEYCODE;
    } else if(input->key == KEY_CTRL_F4 && input->type == INPUTTYPE_NORMAL) {
      SaveVRAM_1();
      Bkey_ClrAllFlags();
      short character;
      character = CharacterSelectDialog();
      if (character) input->cursor = EditMBStringChar((unsigned char*)input->buffer, input->charlimit, input->cursor, character);
      LoadVRAM_1();
    } else if(input->key == KEY_CTRL_F5 && input->type == INPUTTYPE_NORMAL) {
      if (keyflag == 0x04 || keyflag == 0x08 || keyflag == 0x84 || keyflag == 0x88) {
        // ^only applies if some sort of alpha (not locked) is already on
        if (keyflag == 0x08 || keyflag == 0x88) { //if lowercase
          SetSetupSetting( (unsigned int)0x14, keyflag-0x04);
          DisplayStatusArea();
          continue; //do not process the key, because otherwise we will leave alpha status
        } else {
          SetSetupSetting( (unsigned int)0x14, keyflag+0x04);
          DisplayStatusArea();
          continue; //do not process the key, because otherwise we will leave alpha status
        }
      }
    }

    DisplayStatusArea(); 
    if(input->key && input->key < 30000)
    {
      if(input->type == INPUTTYPE_NORMAL) {
        if ((GetSetupSetting( (unsigned int)0x14) == 0x08 || GetSetupSetting( (unsigned int)0x14) == 0x88) && input->key >= KEY_CHAR_A && input->key <= KEY_CHAR_Z) //if lowercase and key is char...
        {
          input->key = input->key + 32; //so we switch to lowercase characters... Casio is smart
        }
        //1st and 5th parameter are, respectively, whether selection is enabled and its starting point. 
        input->cursor = EditMBStringChar((unsigned char*)input->buffer, input->charlimit, input->cursor, input->key);
      } else if (input->type == INPUTTYPE_DATE || input->type == INPUTTYPE_TIME) {
        if (input->key >= KEY_CHAR_0 && input->key <= KEY_CHAR_9) {
          //don't allow for typing non-digits
          input->cursor = EditMBStringChar((unsigned char*)input->buffer, input->charlimit, input->cursor, input->key);          
        }
      }
      DisplayMBString2( 0, (unsigned char*)input->buffer, input->start, input->cursor, 0, input->x, input->y*24-24, input->width+input->x, 0 );
    }
    else
    {
      EditMBStringCtrl2( (unsigned char*)input->buffer, input->charlimit+1, &input->start, &input->cursor, &input->key, input->x, input->y*24-24, 1, input->width+input->x-1 );
    }
  }
  Cursor_SetFlashOff();
  return INPUT_RETURN_CONFIRM;
}