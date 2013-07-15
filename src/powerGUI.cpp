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
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "timeGUI.hpp"
#include "settingsProvider.hpp"
#include "settingsGUI.hpp" 
#include "selectorGUI.hpp"
#include "sprites.h"

void changePoweroffTimeout() {
  Selector sel;
  strcpy(sel.title, "Poweroff Timeout");
  strcpy(sel.subtitle, "");
  sel.value = GetAutoPowerOffTime();
  sel.min = 1;
  sel.max = 999;
  sel.cycle = 0;
  sel.type = SELECTORTYPE_TIMEOUT_MINUTES;
  int res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  SetAutoPowerOffTime(sel.value);
}

void changeBacklightTimeout() {
  Selector sel;
  strcpy(sel.title, "Backlight Duration");
  strcpy(sel.subtitle, "");
  sel.value = GetBacklightDuration();
  sel.min = 1;
  sel.max = 40;
  sel.cycle = 1;
  sel.type = SELECTORTYPE_BACKLIGHT_DURATION;
  int res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  SetBacklightDuration(sel.value);
}

void changeBacklightLevel() {
  int initValue = GetBacklightSubLevel_RAW();
  Selector sel;
  strcpy(sel.title, "Backlight Level");
  strcpy(sel.subtitle, "");
  sel.value = initValue;
  sel.min = 0;
  sel.max = 249;
  sel.allowMkey = 1;
  sel.cycle = 1;
  sel.type = SELECTORTYPE_INSTANT_RETURN;
  sel.clearVRAM = 0;
  
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  int textX=0; int textY=150;
  PrintMiniMini( &textX, &textY, (unsigned char*)"This setting is volatile because it is changed by the OS on", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"poweroff, on backlight timeout and when the user changes", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"the setting in the OS's System menu.", 0, TEXT_COLOR_BLACK, 0 );
  while(1) {
    int res = doSelector(&sel);
    if (res == SELECTOR_RETURN_EXIT) {
      SetBacklightSubLevel_RAW(initValue);
      return; 
    } else if (res == SELECTOR_RETURN_INSTANT) {
      SetBacklightSubLevel_RAW(sel.value);
    } else if (res == SELECTOR_RETURN_SELECTION) {
      return; // bl level was already set with the instant return
    }
  }
}

void powerInformation() {
  int key, textX=0, textY=24+4;
  unsigned int backlightlevel = GetBacklightSubLevel_RAW();
  volatile unsigned int*FRQCR = (unsigned int*) 0xA4150000;
  unsigned char voltbuffer[20];
  itoa(GetMainBatteryVoltage(1), voltbuffer);
  // We are gonna have fuuuuuun!
  memmove(voltbuffer+2, voltbuffer+1, 3);
  voltbuffer[1] = '.';
  strcat((char*)voltbuffer, "V");

  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Power information", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);

  PrintMini(&textX, &textY, (unsigned char*)"Main battery voltage: ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)voltbuffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

  textY=textY+17;
  textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"Note: battery voltage is inaccurate when the power source", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12;
  textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"is USB.", 0, TEXT_COLOR_BLACK, 0 );
  unsigned char hb[15];
  key = *(unsigned char*)P11DR;
  WordToHex( key & 0xFFFF, hb );
  textY=textY+12;
  textX=0;
  PrintMini(&textX, &textY, (unsigned char*)"Power source: ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  unsigned char powerSource[10];
  if (key==0x0000) {
    strcpy((char*)powerSource, "Emulated");
  } else if (key==0x0008) {
    strcpy((char*)powerSource, "Batteries");
  } else if (key==0x000A) {
    strcpy((char*)powerSource, "USB");
  } else {
    strcpy((char*)powerSource, "Unknown");
  }
  PrintMini(&textX, &textY, powerSource, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  
  textY=textY+17;
  textX=0;
  PrintMini(&textX, &textY, (unsigned char*)"Battery type setting: ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  unsigned char batteryType[15];
  if (GetBatteryType()==1) {
    strcpy((char*)batteryType, "Alkaline");
  } else if (GetBatteryType()==2) {
    strcpy((char*)batteryType, "Ni-MH");
  } else {
    strcpy((char*)batteryType, "Not defined");
  }
  PrintMini(&textX, &textY, batteryType, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

  textY=textY+17;
  textX=0;
  PrintMini(&textX, &textY, (unsigned char*)"Screen backlight level: ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  unsigned char blevel[3];
  itoa(backlightlevel, blevel);
  PrintMini(&textX, &textY, (unsigned char*)blevel, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  
  // Find CPU clock
  unsigned char* cfreq;
  switch((*FRQCR & 0x3F000000) >> 24) {
    case PLL_28x: cfreq = (unsigned char*)"101.5 MHz"; break;
    case PLL_26x: cfreq = (unsigned char*)"94.3 MHz"; break;
    case PLL_24x: cfreq = (unsigned char*)"87 MHz"; break;
    case PLL_20x: cfreq = (unsigned char*)"72.5 MHz"; break;
    case PLL_18x: cfreq = (unsigned char*)"65.3 MHz"; break;
    case PLL_16x: cfreq = (unsigned char*)"58 MHz"; break;
    case PLL_15x: cfreq = (unsigned char*)"54.4 MHz"; break;
    case PLL_12x: cfreq = (unsigned char*)"43.5 MHz"; break;
    case PLL_8x: cfreq = (unsigned char*)"29 MHz"; break;
    case PLL_6x: cfreq = (unsigned char*)"21.7 MHz"; break;
    case PLL_4x: cfreq = (unsigned char*)"14.5 MHz"; break;
    case PLL_3x: cfreq = (unsigned char*)"10.8 MHz"; break;
    case PLL_2x: cfreq = (unsigned char*)"7.25 MHz"; break;
    case PLL_1x: cfreq = (unsigned char*)"3.6 MHz"; break;
    default: cfreq = (unsigned char*)"Unknown"; break;
  }
  textY=textY+17;
  textX=0;
  PrintMini(&textX, &textY, (unsigned char*)"CPU clock: ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)cfreq, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  mGetKey(&key);
}
const unsigned int PLLs[] = {PLL_1x, PLL_2x, PLL_3x, PLL_4x, PLL_6x, PLL_8x, PLL_12x, PLL_15x, PLL_16x, PLL_18x, PLL_20x, PLL_24x, PLL_26x, PLL_28x};
void updateCurrentFreq() {
  // this does not draw the VRAM contents to screen, only changes them!
  char*cur = 0x00000000;
  char*desc = 0x00000000;
  volatile unsigned int*FRQCR = (unsigned int*) 0xA4150000;
  
  int arrowBottom = 84;
  
  //Clear area where the cursor arrow appears
  drawRectangle(0, 65, 384, 20, COLOR_WHITE);
  
  switch((*FRQCR & 0x3F000000) >> 24) {
      case PLL_28x:
        cur = (char*)"Running at 101.5 MHz";
        desc = (char*)"Overclocked";
        break;
      case PLL_26x:
        cur = (char*)"Running at 94.3 MHz";
        desc = (char*)"Overclocked";
        drawArrowDown(368, arrowBottom, COLOR_ORANGE);
        break;
      case PLL_24x:
        cur = (char*)"Running at 87 MHz";
        desc = (char*)"Overclocked";
        drawArrowDown(331, arrowBottom, COLOR_ORANGE);
        break;
      case PLL_20x:
        cur = (char*)"Running at 72.5 MHz";
        desc = (char*)"Overclocked";
        drawArrowDown(274, arrowBottom, COLOR_ORANGE);
        break;
      case PLL_18x:
        cur = (char*)"Running at 65.3 MHz";
        desc = (char*)"Overclocked";
        drawArrowDown(246, arrowBottom, COLOR_ORANGE);
        break;
      case PLL_16x:
        cur = (char*)"Running at 58 MHz";
        desc = (char*)"Normal speed";
        drawArrowDown(217, arrowBottom, COLOR_LIMEGREEN);
        break;
      case PLL_15x:
        cur = (char*)"Running at 54.4 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(203, arrowBottom, COLOR_LIGHTBLUE);
        break;
      case PLL_12x:
        cur = (char*)"Running at 43.5 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(169, arrowBottom, COLOR_LIGHTBLUE);
        break;
      case PLL_8x:
        cur = (char*)"Running at 29 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(112, arrowBottom, COLOR_LIGHTBLUE);
        break;
      case PLL_6x:
        cur = (char*)"Running at 21.7 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(84, arrowBottom, COLOR_LIGHTBLUE);
        break;
      case PLL_4x:
        cur = (char*)"Running at 14.5 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(56, arrowBottom, COLOR_LIGHTBLUE);
        break;
      case PLL_3x:
        cur = (char*)"Running at 10.8 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(41, arrowBottom, COLOR_LIGHTBLUE);
        break;
      case PLL_2x:
        cur = (char*)"Running at 7.25 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(27, arrowBottom, COLOR_LIGHTBLUE);
        break;
      case PLL_1x:
        cur = (char*)"Running at 3.6 MHz";
        desc = (char*)"Underclocked";
        drawArrowDown(13, arrowBottom, COLOR_LIGHTBLUE);
        break;
      default:
        cur = (char*)"INVALID CPU SPEED";
        desc = (char*)"Oops...";
        break;
    }
  int textX = 0; int textY = 145;
  PrintMini(&textX, &textY, (unsigned char*)cur, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  char descbuf[15];
  strcpy(descbuf, "  ");
  strcat(descbuf, desc);
  PrintXY(1, 8, descbuf, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
}

void setCPUclock() {
  int key; int textX; int textY;

  volatile unsigned int*FRQCR = (unsigned int*) 0xA4150000;
  while (1) {
    Bdisp_AllClr_VRAM();
    Bdisp_EnableColor(1); 
    DisplayStatusArea();

    PrintXY(1, 1, (char*)"  CPU speed", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    CopySpriteNbit(selector, 10, 85, 364, 43, selector_palette, 1);
    
    textX=0; textY=118;
    PrintMiniMini( &textX, &textY, (unsigned char*)"USE AT YOUR OWN RISK! NO WARRANTY PROVIDED.", 0, TEXT_COLOR_RED, 0 );
    textX=0; textY=130;
    PrintMiniMini( &textX, &textY, (unsigned char*)"Note: changes are applied instantly.", 0, TEXT_COLOR_BLACK, 0 );
    
    updateCurrentFreq();
    Bdisp_PutDisp_DD();
    mGetKey(&key);
    switch (key) {
      case KEY_CTRL_RIGHT:      
      case KEY_CTRL_UP:
         for(int i = 0; i < 12; i++) {
          if(((*FRQCR & 0x3F000000) >> 24) == PLLs[i]) {
            CPU_change_freq(PLLs[i+1]);
            break;
          }
        }
         break;
      case KEY_CTRL_LEFT:
      case KEY_CTRL_DOWN:
        for(int i = 1; i < 14; i++) {
          if(((*FRQCR & 0x3F000000) >> 24) == PLLs[i]) {
            CPU_change_freq(PLLs[i-1]);
            break;
          }
        }
        break;
      case KEY_CTRL_EXIT:
      case KEY_CTRL_EXE:
        return;
        break;
    }
  }
}