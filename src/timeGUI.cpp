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

#include "timeProvider.hpp"
#include "timeGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "tbcdProvider.hpp"
#include "selectorGUI.hpp"

void drawLongDate(int textY, int format, int colorfg, int colorbg, int miniminiinvert) {
  if (format==NULL) format = GetSetting(SETTING_LONGDATEFORMAT);
  if (miniminiinvert==NULL) miniminiinvert = 0;
  // Draw long date as seen on the home screen, according to format.
  // textY is the y coordinate at which the date should start to be drawn.
  // x coordinate is set automatically (text is always centered on screen)
  // NOTE: does not update VRAM contents to screen
  int curYear = getCurrentYear();  
  int curDay = getCurrentDay();

  char buffer[50];
  char buffer2[10];

  int textX = 0;
  switch(format)
  {
    case 0:
      strcpy(buffer, getCurrentDOWAsString());
      strcat(buffer, ", ");
      strcat(buffer, getCurrentMonthAsString());
      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, " ");
      strcat(buffer, buffer2);
      break;
    case 1:
      strcpy(buffer, getCurrentDOWAsString());
      strcat(buffer, ", ");

      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getCurrentMonthAsString());
      break;

    case 2:
      strcpy(buffer, getCurrentDOWAsString());
      strcat(buffer, ", ");

      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getCurrentMonthAsString());

      strcat(buffer, " ");
      itoa(curYear, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      break;

    case 3:
      strcpy(buffer, getCurrentDOWAsString());
      strcat(buffer, ", ");

      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getCurrentMonthAsString());
      break;

    case 4:
    case 6: //it's exactly the same except minimini is not used, but that's not handled here
      strcpy(buffer, getCurrentMonthAsString());
      strcat(buffer, " ");
      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      break;

    case 5:
      strcpy(buffer, getCurrentMonthAsString());
      strcat(buffer, " ");
      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);

      strcat(buffer, ", ");
      itoa(curYear, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      break;

    case 7:
    case 9: //exactly the same except minimini, not handled here
      itoa(curDay, (unsigned char*)buffer2);
      strcpy(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getCurrentMonthAsString());
      break;

    case 8:
      itoa(curDay, (unsigned char*)buffer2);
      strcpy(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getCurrentMonthAsString());

      strcat(buffer, " ");
      itoa(curYear, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      break;
  }

  //Use PrintMini to get length in pixels of the string, then draw it on the middle of the screen
  PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, colorfg, colorbg, 0, 0); //get length
  textX = LCD_WIDTH_PX/2 - textX/2; //center
  PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, colorfg, colorbg, 1, 0); //draw
  
  if (format == 0 || format == 1 || format == 4 || format == 7) {
    // draw year in minimini font
    itoa(curYear, (unsigned char*)buffer2);
    int newTextX = 0;
    textY = textY+17;
    PrintMiniMini( &newTextX, &textY, (unsigned char*)buffer2, 0, TEXT_COLOR_BLACK, 1 ); //fake draw
    textX = textX - newTextX;
    PrintMiniMini( &textX, &textY, (unsigned char*)buffer2, (miniminiinvert == 1 ? 4 : 0), TEXT_COLOR_BLACK, 0 ); //draw
  }
  return;
}

void setTimeGUI(int canExit) {
  Selector hour;
  Selector minute;
  Selector second;
  while(1) {
    strcpy(hour.title, "Set time");
    strcpy(hour.subtitle, "Hour");
    hour.value = getCurrentHour();
    hour.min = 0;
    hour.max = 23;
    hour.allowMkey = 0;
    hour.cycle = 1;
    hour.type = SELECTORTYPE_NORMAL;
    int res = doSelector(&hour);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop time adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  while(1) {
    strcpy(minute.title, "Set time");
    strcpy(minute.subtitle, "Minute");
    minute.value = getCurrentMinute();
    minute.min = 0;
    minute.max = 59;
    minute.allowMkey = 0;
    minute.cycle = 1;
    minute.type = SELECTORTYPE_NORMAL;
    int res = doSelector(&minute);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop time adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }  
  while(1) {
    strcpy(second.title, "Set time");
    strcpy(second.subtitle, "Second");
    second.value = getCurrentSecond();
    second.min = 0;
    second.max = 59;
    second.allowMkey = 0;
    second.cycle = 1;
    second.type = SELECTORTYPE_NORMAL;
    int res = doSelector(&second);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop time adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }  
  setTime(hour.value, minute.value, second.value);
}

void setDateGUI(int canExit) {
  Selector year;
  Selector month;
  Selector day;
  while(1) {
    strcpy(year.title, "Set date");
    strcpy(year.subtitle, "Year");
    year.value = getCurrentYear();
    year.min = 1970;
    year.max = 9999;
    year.allowMkey = 0;
    year.cycle = 0;
    year.type = SELECTORTYPE_NORMAL;
    int res = doSelector(&year);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop date adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  while(1) {
    strcpy(month.title, "Set date");
    strcpy(month.subtitle, "Month");
    month.value = getCurrentMonth();
    month.min = 1;
    month.max = 12;
    month.allowMkey = 0;
    month.cycle = 1;
    month.type = SELECTORTYPE_MONTH;
    int res = doSelector(&month);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop date adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  while(1) {
    strcpy(day.title, "Set date");
    strcpy(day.subtitle, "Day");
    day.value = getCurrentDay();
    day.min = 1;
    day.max = getMonthDays(month.value);
    day.allowMkey = 0;
    day.cycle = 1;
    day.type = SELECTORTYPE_NORMAL;
    int res = doSelector(&day);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop date adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  setDate(year.value, month.value, day.value); 
}


void RTCunadjustedWizard(int helpMessage) {
  //first check if RTC is unadjusted. if not, return.
  if(!getRTCisUnadjusted()) return;
  int textX = 0;
  int textY = 0;
  int key;
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  if(helpMessage) {
    PrintXY(1, 1, (char*)"  Clock unadjusted", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    textY = 24; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"Looks like this calculator's clock isn't", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"set correctly, because it is set to a", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"date in the past. This may be because", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"you never set this clock before or", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"because you took off the batteries.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    
    textY = textY + 34; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"Press EXE or F1 to start setting the", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"time and date, or EXIT to ignore and", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"continue.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    int inscreen = 1;
    while(inscreen) {
      mGetKey(&key);
      switch(key) {
        case KEY_CTRL_EXE:
        case KEY_CTRL_F1:
          inscreen=0;
          break;
        case KEY_CTRL_EXIT:
          return;
      }
    }
  }
  
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Clock unadjusted", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  textY = 24+17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"When setting the date and time, use", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"the up/down keys to change the", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"displayed value, and EXE to confirm.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 17+17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"Press any key to start.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  mGetKey(&key);
  setTimeGUI(0);
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Clock unadjusted", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  textY = 24+17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"Now let's set the date.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 17+17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"Press any key to start.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  mGetKey(&key);
  setDateGUI(0);
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Clock adjusted", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  textY = 24+17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"We're done!", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"If you ever need to adjust the clock", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"again press Shift->Menu and choose", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 17; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"the appropriate options.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = textY + 30; textX = 0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"You'll need to set the clock every time you take off the", 0, TEXT_COLOR_BLACK, 0 );
  textY = textY + 12; textX = 0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"batteries.", 0, TEXT_COLOR_BLACK, 0 );
  textY = LCD_HEIGHT_PX-17-24; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"Press any key to continue.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  mGetKey(&key);
}

void currentTimeToBasicVar() {
  TBCD Src;
  int ihour = getCurrentHour();
  int iminute = getCurrentMinute();
  int isecond = getCurrentSecond();
  int imillisecond = getCurrentMillisecond();
  double fhour=0.0,fminute=0.0,fsecond=0.0,fmillisecond=0.0,hourfraction = 0;
  
  fhour = ihour; fminute = iminute; fsecond = isecond; fmillisecond = imillisecond;
  hourfraction = fhour+fminute/60.0+fsecond/(60.0*60.0)+fmillisecond/(60.0*60.0*1000.0);
  Src.Set( hourfraction );
  Alpha_SetData( 'T', &Src );
  MsgBoxPush(4);
  PrintXY(3, 2, (char*)"  Time fraction", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(3, 3, (char*)"  saved to alpha", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(3, 4, (char*)"  variable T.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
  int gkey;
  while(1) {
    mGetKey(&gkey);
    switch(gkey)
    {
      case KEY_CTRL_EXIT:
      case KEY_CTRL_AC:
        MsgBoxPop();
        return;
        break;
    }
  }
}