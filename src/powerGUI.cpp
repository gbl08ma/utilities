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
#include "textGUI.hpp"
#include "sprites.h"

void setPoweroffTimeout() {
  Selector sel;
  sel.title = (char*)"Poweroff Timeout";
  sel.value = GetAutoPowerOffTime();
  sel.min = 1;
  sel.max = 999;
  sel.cycle = 0;
  sel.type = SELECTORTYPE_TIMEOUT_MINUTES;
  int res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  SetAutoPowerOffTime(sel.value);
}

void setBacklightTimeout() {
  Selector sel;
  sel.title = (char*)"Backlight Duration";
  sel.value = GetBacklightDuration();
  sel.min = 1;
  sel.max = 40;
  sel.type = SELECTORTYPE_BACKLIGHT_DURATION;
  int res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  SetBacklightDuration(sel.value);
}

void setBacklightLevel() {
  int initValue = getRawBacklightSubLevel();
  Selector sel;
  sel.title = (char*)"Backlight Level";
  sel.value = initValue;
  sel.max = 249;
  sel.type = SELECTORTYPE_INSTANT_RETURN;
  sel.clearVRAM = 0;
  
  Bdisp_AllClr_VRAM();
  textArea text;
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.y = 150;
  text.lineHeight=19;
  textElement elem[2];
  text.elements = elem;
  text.scrollbar=0;
  
  elem[0].text = (char*)"This setting is volatile because it is changed by the OS on poweroff, on "
                        "backlight timeout and when the user changes the setting in the OS's "
                        "System menu.";
  elem[0].minimini = 1;
  text.numelements = 1;
  doTextArea(&text);
  while(1) {
    switch(doSelector(&sel)) {
      case SELECTOR_RETURN_EXIT:
        setRawBacklightSubLevel(initValue);
        // deliberate fallthrough:
        // bl level was already set with the instant return, so if user is confirming, just return.
      case SELECTOR_RETURN_SELECTION:
        return;
      case SELECTOR_RETURN_INSTANT:
        setRawBacklightSubLevel(sel.value);
        break;
    }
  }
}

void powerInformation() {
  unsigned int backlightlevel = getRawBacklightSubLevel();
  volatile unsigned int*FRQCR = (unsigned int*) 0xA4150000;
  unsigned char voltbuffer[20];
  itoa(GetMainBatteryVoltage(1), voltbuffer);
  // We are gonna have fuuuuuun!
  memmove(voltbuffer+2, voltbuffer+1, 3);
  voltbuffer[1] = '.';
  strcat((char*)voltbuffer, "V");
  
  textArea text;
  text.title = (char*)"Power Information";

  textElement elem[16];
  text.elements = elem;
  text.scrollbar=0;
  
  elem[0].text = (char*)"Battery voltage:";
  elem[0].spaceAtEnd = 1;
  elem[1].text = (char*)voltbuffer;
  
  elem[2].newLine = 1;
  elem[2].color = COLOR_GRAY;
  elem[2].text = (char*)"Note: battery voltage is inaccurate when the power source is USB.";
  
  elem[3].newLine = 1;
  elem[3].text = (char*)"Power source:";
  elem[3].spaceAtEnd = 1;
  int key = *(unsigned char*)P11DR;
  switch(key) {
    case 0x00:
      elem[4].text = (char*)"Emulated";
      break;
    case 0x08:
      elem[4].text = (char*)"Batteries";
      break;
    case 0x0A:
      elem[4].text = (char*)"USB";
      break;
    default:
      elem[4].text = (char*)"Unknown";
      break;
  }
  
  elem[5].newLine = 1;
  elem[5].text = (char*)"Battery type setting:";
  elem[5].spaceAtEnd = 1;
  
  switch(GetBatteryType()) {
    case 1:
      elem[6].text = (char*)"Alkaline";
      break;
    case 2:
      elem[6].text = (char*)"Ni-MH";
      break;
    default:
      elem[6].text = (char*)"Not defined";
      break;
  }
  
  elem[7].newLine = 1;
  elem[7].text = (char*)"Screen backlight level:";
  elem[7].spaceAtEnd = 1;
  
  unsigned char blevel[5];
  itoa(backlightlevel, blevel);
  elem[8].text = (char*)blevel;

  elem[9].newLine = 1;
  elem[9].text = (char*)"Backlight timeout:";
  elem[9].spaceAtEnd = 1;
  char btimeout[20];
  int timeoutval = GetBacklightDuration();

  if(timeoutval % 2 == 0) { //even, so timeout is X min 0 sec.
    itoa(timeoutval/2, (unsigned char*)btimeout);
  } else { // timeout is X min 30 sec.
    itoa((timeoutval-1)/2, (unsigned char*)btimeout);
  }
  strcat(btimeout, " Minute");
  if(timeoutval != 2 && timeoutval != 3) strcat(btimeout, "s");
  if(timeoutval % 2 != 0) strcat(btimeout, " 30 Sec.");
  elem[10].text = btimeout;

  elem[11].newLine = 1;
  elem[11].text = (char*)"Power off timeout:";
  elem[11].spaceAtEnd = 1;
  char ptimeout[20];
  timeoutval = GetAutoPowerOffTime();
  itoa(timeoutval, (unsigned char*)ptimeout);
  strcat(ptimeout, " Minute");
  if(timeoutval != 1) strcat(ptimeout, "s");
  elem[12].text = ptimeout;
  
  elem[13].newLine = 1;
  elem[13].text = (char*)"CPU clock:";
  elem[13].spaceAtEnd = 1;
  char* desc; int c;
  getPLLinfo((*FRQCR & 0x3F000000) >> 24, &elem[14].text, &desc, &c);
  elem[14].spaceAtEnd = 1;
  elem[15].text = (char*)"MHz";
  text.numelements = 16;
  doTextArea(&text);
}
const unsigned int PLLs[] = {PLL_1x, PLL_2x, PLL_3x, PLL_4x, PLL_6x, PLL_8x, PLL_12x, PLL_15x,
                             PLL_16x, PLL_18x, PLL_20x, PLL_24x, PLL_26x, PLL_28x};
static const unsigned short markerspos[] = {13, 27, 41, 56, 84, 112, 169, 203, 217, 246, 274,
                                            331, 368, 0};
static const char* freqstrings[] = {"3.6", "7.25", "10.8", "14.5", "21.7", "29", "43.5", "54.4",
                                    "58", "65.3", "72.5", "87", "94.3", "101.5"};

int getPLLinfo(unsigned int PLL, char** freqstr, char** statusstr, int* color) {
  // gets frequency as string, status (over/underclocked/normal) and returns the X position for the
  // arrow. freqstr and statusstr receive pointers to static strings
  if(PLL > PLL_16x) {
    *statusstr = (char*)"Overclocked";
    *color = COLOR_ORANGE;
  } else if(PLL < PLL_16x) {
    *statusstr = (char*)"Underclocked";
    *color = COLOR_LIGHTBLUE;
  } else if(PLL == PLL_16x) {
    *statusstr = (char*)"Normal speed";
    *color = COLOR_LIMEGREEN;
  } else {
    *statusstr = (char*)"???";
    *color = 0;
  }
  for(int i = 0; i < 14; i++) {
    if(PLLs[i] == PLL) {
      *freqstr = (char*)freqstrings[i];
      return markerspos[i];
    }
  }
  *freqstr = (char*)"Unknown";
  return 0;
}
#define FREQ_ARROW_BOTTOM 84
void updateCurrentFreq() {
  // this does not draw the VRAM contents to screen, only changes them!
  char* cur;
  char* desc;
  int color;
  volatile unsigned int*FRQCR = (unsigned int*) 0xA4150000;

  int xpos = getPLLinfo((*FRQCR & 0x3F000000) >> 24, &cur, &desc, &color);
  if(xpos) {
    drawArrowDown(xpos, FREQ_ARROW_BOTTOM, color);
  }
  int textX = 0; int textY = 147;
  char buffer[50];
  sprintf(buffer, "Running at %s MHz", cur);
  PrintMini(&textX, &textY, buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  mPrintXY(1, 8, desc, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
}

void setCPUclock() {
  int key;

  volatile unsigned int*FRQCR = (unsigned int*) 0xA4150000;
  while (1) {
    Bdisp_AllClr_VRAM();

    drawScreenTitle("CPU speed");

    drawRectangle(13, 88, 357, 2, COLOR_BLACK);
    
    // draw vertical ticks on the possible CPU frequencies:
    for(int i = 0; i < 13; i++)
      drawRectangle(markerspos[i], 85, 2, 19, COLOR_BLACK);

    CopySpriteNbitMasked(selector, 10, 105, 364, 22, selector_palette, 0xffff, 1);
    
    textArea text;
    text.type = TEXTAREATYPE_INSTANT_RETURN;
    text.y = 118;
    text.lineHeight=14;
    textElement elem[2];
    text.elements = elem;
    text.scrollbar=0;
    
    elem[0].text = (char*)"USE AT YOUR OWN RISK! NO WARRANTY PROVIDED.";
    elem[0].color = TEXT_COLOR_RED;
    elem[0].minimini = 1;
    elem[1].text = (char*)"Note: changes are applied instantly.";
    elem[1].newLine = 1;
    elem[1].minimini = 1;
    text.numelements = 2;
    doTextArea(&text);
    
    updateCurrentFreq();
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