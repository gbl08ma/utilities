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
#include "debugGUI.hpp"

void firstRunWizard() {
  setmGetKeyMode(MGETKEY_MODE_RESTRICT_SETTINGS);
  Bdisp_AllClr_VRAM();
  DrawFrame(COLOR_WHITE);
  drawtnyimLogo( LCD_WIDTH_PX/2-138/2, LCD_HEIGHT_PX/2-42/2);
  int textY = LCD_HEIGHT_PX/2-42/2-24 - 20;
  int textX = 104;
  PrintMini(&textX, &textY, (char*)"brought to you by", 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);

  textY = LCD_HEIGHT_PX/2+42/2-24;
  textX = 94;
  PrintMini(&textX, &textY, (char*)"tny. ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (char*)"i", 0, 0xFFFFFFFF, 0, 0, TNYIM_ORANGE, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (char*)"nternet ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (char*)"m", 0, 0xFFFFFFFF, 0, 0, TNYIM_ORANGE, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (char*)"edia", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  Bdisp_PutDisp_DD();
  blockForMilliseconds(3500);
  
  textArea text;
  text.title = (char*)"Welcome to " ADDIN_FRIENDLYNAME;
  
  textElement elem[14];
  text.elements = elem;
  text.allowEXE = 1;
  
  elem[0].text = (char*)"This add-in provides functionality not originally present on Casio Prizm (fx-CG 10/20) calculators:";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 5;
  elem[1].text = (char*)"- Clock and chronometer";
  elem[2].newLine = 1;
  elem[2].text = (char*)"- Calendar with agenda and tasklist";
  elem[3].newLine = 1;
  elem[3].text = (char*)"- Fine timeout&backlight adjustment";
  elem[4].newLine = 1;
  elem[4].text = (char*)"- File manager with file search";
  elem[5].newLine = 1;
  elem[5].text = (char*)"- CPU clock adjustment";
  elem[6].newLine = 1;
  elem[6].text = (char*)"...and more";
  
  elem[7].newLine = 1;
  elem[7].lineSpacing = 8;
  elem[7].color = COLOR_ORANGE;
  elem[7].text = (char*)"Important notes:";
  
  elem[8].newLine = 1;
  elem[8].text = (char*)"To set time and date, as well as other settings, press Shift+Menu (Setup) at almost any point in the add-in.";
  
  elem[9].newLine = 1;
  elem[9].lineSpacing = 5;
  elem[9].text = (char*)"The CPU clock adjustment tool is hidden by default. To enable it, turn on the \"Show advanced tools\" setting.";
  
  elem[10].newLine = 1;
  elem[10].lineSpacing = 8;
  elem[10].text = (char*)"Thanks for reading these notes.";
  elem[11].newLine = 1;
  elem[11].text = (char*)"After pressing EXIT or EXE, you may be guided to adjust your calculator's clock.";
  
  elem[12].newLine = 1;
  elem[12].lineSpacing = 3;
  elem[12].text = (char*)"In case you need help with this software, contact info is on the \"About\" screen, to which you can get from the Settings menu.";
  
  text.numelements = 13;
  doTextArea(&text);
  SetSetting(SETTING_IS_FIRST_RUN, 0, 1);
  setmGetKeyMode(MGETKEY_MODE_NORMAL);
}
 
