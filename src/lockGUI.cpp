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

#include "homeGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "lockProvider.hpp"
#include "constantsProvider.hpp"
#include "inputGUI.hpp"
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "hardwareProvider.hpp"
#include "setjmp.h"

int passwordInput(int x, int y, unsigned char* buffer) {
  //returns: 0 on user abort (EXIT), 1 on EXE
  int start = 0, cursor = 0, key;
  int charlimit = 256;
  unsigned char dispbuffer[256] = ""; //will hold asterisks instead of real characters...
  //clean buffer and display buffer
  strcpy((char*)buffer, "");
  strcpy((char*)dispbuffer, "");
  while(1)
  {   
    strcpy((char*)dispbuffer, "");
    int numchars = strlen((char*)buffer);
    if(GetSetting(SETTING_PASSWORD_PRIVACY)) {
      if(numchars > 0) {
        for (int k = 0; k < numchars-1; k++) {
          strcat((char*)dispbuffer, "*");
        }
        //show last character for easier typing
        strcat((char*)dispbuffer, " ");
        dispbuffer[(strlen((char*)dispbuffer)-1)] = buffer[(strlen((char*)buffer)-1)];
      }
    } else {
      if(numchars > 0) {
        for (int k = 0; k < numchars; k++) {
          strcat((char*)dispbuffer, "*");
        }
      }
    }
    int iresult;
    GetFKeyPtr(0x0307, &iresult); // A<>a
    FKey_Display(4, (int*)iresult);
    GetFKeyPtr(0x02A1, &iresult); // CHAR
    FKey_Display(5, (int*)iresult);
    Cursor_SetFlashOn(5);
    DisplayMBString((unsigned char*)dispbuffer, start, cursor, x,y);

    int keyflag = GetSetupSetting( (unsigned int)0x14);
    GetKey(&key);
    if (GetSetupSetting( (unsigned int)0x14) == 0x01 || GetSetupSetting( (unsigned int)0x14) == 0x04 || GetSetupSetting( (unsigned int)0x14) == 0x84) {
      keyflag = GetSetupSetting( (unsigned int)0x14); //make sure the flag we're using is the updated one.
      //we can't update always because that way alpha-not-lock will cancel when F5 is pressed.
    }
    if(key == KEY_CTRL_F5)
    {
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
    else if(key == KEY_CTRL_F6)
    {
      SaveVRAM_1();
      Bkey_ClrAllFlags();
      short character;
      character = CharacterSelectDialog();
      if (character) cursor = EditMBStringChar((unsigned char*)buffer, charlimit, cursor, character);
      LoadVRAM_1();
    }  
    if (key == KEY_CTRL_EXIT) { Cursor_SetFlashOff(); return 0; }
    if (key == KEY_CTRL_EXE) {
      if (strlen((char*)buffer) > 0) {
        Cursor_SetFlashOff(); return 1;
      } else {
        mMsgBoxPush(3);
        mPrintXY(3, 3, (char*)"Code can't be", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
        mPrintXY(3, 4, (char*)"empty.", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
        PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
        closeMsgBox(0);
      }
    }
    if(key && key < 30000)
    {
      if ((keyflag == 0x08 || keyflag == 0x88) && key >= KEY_CHAR_A && key <= KEY_CHAR_Z) //if lowercase and key is char...
      {
        key = key + 32; //so we switch to lowercase characters... Casio is smart
      }
      cursor = EditMBStringChar((unsigned char*)buffer, charlimit, cursor, key);
    }
    else
    {
      EditMBStringCtrl((unsigned char*)buffer, charlimit, &start, &cursor, &key, x, y);
    }
  }
  Cursor_SetFlashOff();
  return 0;
}

int setPassword() {
  unsigned char password[256] = "";
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  mPrintXY(1, 1, (char*)"Calculator lock", TEXT_MODE_NORMAL, TEXT_COLOR_BLUE);
  mPrintXY(1, 2, (char*)"Set new code:", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  int res = passwordInput(1, 3, password);
  if (res == 1) savePassword(password);
  return res; //which means this returns 1 if password was set, and 0 if user aborted
}

int unlockCalc() {
  //returns 1 on unlocked and 0 on still locked
  unsigned char password[256] = "";
  
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  mPrintXY(1, 1, (char*)"Calculator lock", TEXT_MODE_NORMAL, TEXT_COLOR_BLUE);
  mPrintXY(1, 2, (char*)"Input code:", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  int res = passwordInput(1, 3, password);
  int compareRes=0;
  if (res == 0) { return 0; }
  else if (res == 1) compareRes = comparePasswordHash(password);
  switch(compareRes)
  {
    case 0:
      return 1;
      break;
    case 1:
    case 2:
    case 3:
      mMsgBoxPush(3);
      mPrintXY(3, 3, (char*)"Data tampering", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      mPrintXY(3, 4, (char*)"detected!", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
      closeMsgBox(0);
      return 0;
      break;
    case 4:
      mMsgBoxPush(3);
      mPrintXY(3, 3, (char*)"Wrong code", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
      closeMsgBox(0);
      return 0;
      break;
    default: break;
  }
  return 0;
}

int lockCalc() {
  //returns 0 on "calculator was locked and now is unlocked"
  //returns 1 on "there was no lock code and one was now set, or user aborted before setting one"
  if(!isPasswordSet()) {
    textArea text;
    strcpy(text.title, (char*)"Calculator lock");
    
    textElement elem[10];
    text.elements = elem;
    
    elem[0].text = (char*)"This is probably the first time you are using the calculator lock function, which lets you lock your calculator with a password. You must set that password before you can use it. Later, you can set a new password in the Settings menu.";
    
    elem[1].newLine = 1;
    elem[1].lineSpacing = 8;
    elem[1].text = (char*)"Press EXE to close this message and proceed, or EXIT to cancel.";
    
    text.numelements = 2;
    text.allowEXE = 1;
    if(!doTextArea(&text)) return 1;
    setPassword();
    
    elem[0].text = (char*)"You successfully set a password for locking your calculator.";
    
    elem[1].newLine = 1;
    elem[1].lineSpacing = 8;
    elem[1].text = (char*)"Next time you press F5 on the home screen, your calculator will lock. You should then press the ALPHA key to be prompted for the code to unlock it.";
    
    elem[2].newLine = 1;
    elem[2].lineSpacing = 8;
    elem[2].text = (char*)"You can change settings relative to the calculator lock function on the settings (press Shift then Menu).";
    
    elem[3].newLine = 1;
    elem[3].lineSpacing = 8;
    elem[3].text = (char*)"Press EXIT to close this message and return to the home screen.";
    
    text.numelements = 4;
    doTextArea(&text);
    return 1;
  }
  int textX, textY;
  textY = LCD_HEIGHT_PX - 17 - 24;
  if(GetSetting(SETTING_LOCK_AUTOOFF)) PowerOff(1);
  SetGetkeyToMainFunctionReturnFlag(0); //Disable menu return
  while(1) {
    textX = 212;
    int key;
    Bdisp_AllClr_VRAM();
    Bdisp_EnableColor(1);
    DisplayStatusArea();
    PrintMini(&textX, &textY, (unsigned char*)"Calculator locked", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
    GetKey(&key); //oh, the pleasure of using GetKey and still have the Menu blocked
    setBrightnessToStartupSetting();
    if (key == KEY_CTRL_ALPHA) {
      SetSetupSetting( (unsigned int)0x14, 0); //avoid alpha still being triggered at start of text input
      if(1==unlockCalc()) {
        SetGetkeyToMainFunctionReturnFlag(1); //Enable menu return      
        return 0;
      }
    }
  }
}
extern jmp_buf utilities_return;
int rettimer;
int isKeyPressed(int basic_keycode)
{
  const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;
  unsigned short lastkey[8];
  memcpy(lastkey, keyboard_register, sizeof(unsigned short)*8);
  int row, col, word, bit;
  row = basic_keycode%10;
  col = basic_keycode/10-1;
  word = row>>1;
  bit = col + 8*(row&1);
  return (0 != (lastkey[word] & 1<<bit));
}
void returnToUtilitiesHandler() {
  if(isKeyPressed(KEY_PRGM_EXIT) && isKeyPressed(KEY_PRGM_SHIFT)) { // Shift+Exit (simultaneously)
    SetSetupSetting( (unsigned int)0x14, 0);
    Timer_Stop(rettimer);
    Timer_Deinstall(rettimer);
    longjmp(utilities_return, 1);
  }
}
void openRunMat() {
  rettimer = Timer_Install(0, returnToUtilitiesHandler, 100);
  if (rettimer > 0) { Timer_Start(rettimer); }
  APP_RUNMAT(0,0);
}
void lockApp() {
  if (GetSetting(SETTING_ENABLE_LOCK)) {
    int lockres = lockCalc();
    if(lockres == 0) {
      if (GetSetting(SETTING_UNLOCK_RUNMAT) == 1) {
        openRunMat();
      } else if (GetSetting(SETTING_UNLOCK_RUNMAT) == 2) {
        mMsgBoxPush(4);
        mPrintXY(3, 2, (char*)"Open Run-Mat?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(3, 4, (char*)"   Yes:[F1]/[1]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(3, 5, (char*)"   No :Other key", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        int key,inscreen=1;
        while(inscreen) {
          mGetKey(&key);
          switch(key)
          {
            case KEY_CTRL_F1:
            case KEY_CHAR_1:
              {
                mMsgBoxPop();
                openRunMat();
                break;
              }
            default:
              inscreen=0;
              break;
          }
        }
        mMsgBoxPop();
      }
    } 
  }
}