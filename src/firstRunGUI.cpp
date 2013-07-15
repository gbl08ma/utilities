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

#include "powerGUI.hpp"
#include "menuGUI.hpp"
#include "selectorGUI.hpp"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "timeProvider.hpp"
#include "firstRunGUI.hpp"
#include "settingsProvider.hpp"

void firstRunWizard() {
  Bdisp_AllClr_VRAM();
  int curstep = 0;
  int textX=0, textY=0, key;

  drawtnyimLogo( LCD_WIDTH_PX/2-138/2, LCD_HEIGHT_PX/2-42/2, 0);
  PrintMini(&textX, &textY, (unsigned char*)"brought to you by", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  int textLen = textX;
  textY = LCD_HEIGHT_PX/2-42/2-24 - 20;
  textX = LCD_WIDTH_PX/2 - textLen/2;
  
  PrintMini(&textX, &textY, (unsigned char*)"brought to you by", 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);
  
  textX=0;
  PrintMini(&textX, &textY, (unsigned char*)"tny. internet media", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  textLen = textX;
  textY = LCD_HEIGHT_PX/2+42/2-24;
  textX = LCD_WIDTH_PX/2 - textLen/2;
  int orange = drawRGB24toRGB565(210, 68, 19);
  PrintMini(&textX, &textY, (unsigned char*)"tny. ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"i", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"nternet ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"m", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"edia", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  Bdisp_PutDisp_DD();
  blockForMilliseconds(3500);
  while(1) {
    Bdisp_AllClr_VRAM();
    DisplayStatusArea();
    PrintXY(1, 1, (char*)"  Welcome to Utilities", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    switch(curstep) {
      case 0:
        if(1) {
          int iresult;
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          
          textY = 24; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"This add-in provides functionality", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"not originally present on Casio Prizm", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"(fx-CG 10/20) calculators:", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 22; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)" - Clock and chronometer", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)" - Calendar with agenda and tasklist", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)" - Fine timeout&backlight adjustment", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)" - File manager", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);    
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)" - CPU clock adjustment", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 5; textX = textX + 40;
          PrintMiniMini( &textX, &textY, (unsigned char*)"...and more", 0, TEXT_COLOR_BLACK, 0 );
          textX = 0; textY = LCD_HEIGHT_PX-24-15;
          PrintMiniMini( &textX, &textY, (unsigned char*)"Press F6 or EXE for next page", 0, TEXT_COLOR_BLACK, 0 );
          GetKey(&key);
          switch(key)
          {
            case KEY_CTRL_EXE:
            case KEY_CTRL_F6:
              curstep = curstep+1;
              break;
          }
        }
        break;
      case 1:
        if(1) {
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          
          textY = 24; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"Important notes:", 0, 0xFFFFFFFF, 0, 0, COLOR_ORANGE, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"To set time and date, as well as other", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"settings, press Shift+Menu (Setup) at", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"almost any point in the add-in.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 27; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"The CPU clock adjustment tool is", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"hidden by default. To enable it, turn", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"on the \"Show advanced tools\" setting.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          
          GetKey(&key);
          switch(key)
          {
            case KEY_CTRL_F1:
              curstep = curstep-1;
              break;
            case KEY_CTRL_EXE:
            case KEY_CTRL_F6:
              curstep = curstep+1;
              break;
          }
        }
        break;
      case 2:
        if(1) {
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          
          textY = 24; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"Important notes:", 0, 0xFFFFFFFF, 0, 0, COLOR_ORANGE, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"There is a calculator lock function,", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"that allows for locking your", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"calculator with a password.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 20; textX = 0;
          PrintMiniMini( &textX, &textY, (unsigned char*)"Lock the calculator by pressing F5 on the home screen.", 0, TEXT_COLOR_BLACK, 0 );
          textY = textY + 12; textX = 0;
          PrintMiniMini( &textX, &textY, (unsigned char*)"You'll be prompted to set a password the first time you use", 0, TEXT_COLOR_BLACK, 0 );
          textY = textY + 12; textX = 0;
          PrintMiniMini( &textX, &textY, (unsigned char*)"this function. You can set a new password in the Settings", 0, TEXT_COLOR_BLACK, 0 );
          textY = textY + 12; textX = 0;
          PrintMiniMini( &textX, &textY, (unsigned char*)"screen (you'll learn how to get there after the jump).", 0, TEXT_COLOR_BLACK, 0 );
          
          GetKey(&key);
          switch(key)
          {
            case KEY_CTRL_F1:
              curstep = curstep-1;
              break;
            case KEY_CTRL_EXE:
            case KEY_CTRL_F6:
              curstep = curstep+1;
              break;
          }
        }
        break;
      case 3:
        if(1) {
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A4, &iresult); // Finish
          FKey_Display(5, (int*)iresult);
          
          PrintXY(1, 1, (char*)"  Welcome to Utilities", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
          textY = 24; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"Thanks for reading these notes.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"After pressing F6 or EXE, you will", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"probably be guided to adjust your", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"calculator's clock.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 20; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"In case you need help with this", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"software, contact info is on the", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"\"About\" screen, to which you can get", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          textY = textY + 17; textX = 0;
          PrintMini(&textX, &textY, (unsigned char*)"from the settings menu.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
          mGetKey(&key); //do a managed GetKey now so that users can go into Settings.
          switch(key)
          {
            case KEY_CTRL_F1:
              curstep = curstep-1;
              break;
            case KEY_CTRL_EXE:
            case KEY_CTRL_F6:
              SetSetting(SETTING_IS_FIRST_RUN, 0, 1);
              return;
              break;
          }
        }
        break;
    }
  }
}
 
