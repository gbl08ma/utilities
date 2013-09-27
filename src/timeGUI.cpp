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
#include "graphicsProvider.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "tbcdProvider.hpp"
#include "selectorGUI.hpp"

#include "debugGUI.hpp"

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

void drawAnalogClockFace(int cx, int cy, int radius, int colorbg, int colorfg) {
  // may be extended in the future
  drawFilledCircle(cx, cy, radius, colorfg);
  drawFilledCircle(cx, cy, radius-2, colorbg);
}
double sine(double x)
{
    // useful to pre-calculate
    double x2 = x*x;
    double x4 = x2*x2;

    // Calculate the terms
    // As long as abs(x) < sqrt(6), which is 2.45, all terms will be positive.
    // Values outside this range should be reduced to [-pi/2, pi/2] anyway for accuracy.
    // Some care has to be given to the factorials.
    // They can be pre-calculated by the compiler,
    // but the value for the higher ones will exceed the storage capacity of int.
    // so force the compiler to use unsigned long longs (if available) or doubles.
    double t1 = x * (1.0 - x2 / (2*3));
    double x5 = x * x4;
    double t2 = x5 * (1.0 - x2 / (6*7)) / (1.0* 2*3*4*5);
    double x9 = x5 * x4;
    double t3 = x9 * (1.0 - x2 / (10*11)) / (1.0* 2*3*4*5*6*7*8*9);
    double x13 = x9 * x4;
    double t4 = x13 * (1.0 - x2 / (14*15)) / (1.0* 2*3*4*5*6*7*8*9*10*11*12*13);
    
    double x17 = x13 * x4;
    double t5 = x17 * (1.0 - x2 / (18*19)) / (1.0* 2*3*4*5*6*7*8*9*10*11*12*13*17*15*16*17);
    // add some more if your accuracy requires them.
    // But remember that x is smaller than 2, and the factorial grows very fast
    // so I doubt that 2^17 / 17! will add anything.
    // Even t4 might already be too small to matter when compared with t1.

    // Sum backwards
    double result = t5;
    result += t4;
    result += t3;
    result += t2;
    result += t1;

    return result;
}
double cosine(double x) {
  return sine(M_PI/2-x);
}
void drawAnalogClockSecondNeedle(int s, int cx, int cy, double radius, int colorfg) {
  double angle=-90.0;
  double sx,sy;
  double length = radius - 5.0;
  sx=cx+length*cosine((angle+s*6.0)*M_PI/180.0);
  sy=cy+length*sine((angle+s*6.0)*M_PI/180.0);
  drawLine(cx,cy,sx,sy,colorfg);
}

void drawAnalogClockMinuteNeedle(int m, int s, int cx, int cy, double radius, int colorfg) {
  double angle=-90;
  double sx,sy;
  double length = radius - 10;
  sx=cx+length*cosine((angle+m*6)*M_PI/180);
  sy=cy+length*sine((angle+m*6)*M_PI/180);
  drawLine(cx,cy,sx,sy, colorfg);
}

void drawAnalogClockHourNeedle(int h, int m, int s, int cx, int cy, double radius, int colorfg) {
  double angle=-90;
  double sx,sy;
  double length = radius - 20;
  sx=cx+length*cosine((angle+h*30+(m*30/60))*M_PI/180);
  sy=cy+length*sine((angle+h*30+(m*30/60))*M_PI/180);
  drawLine(cx,cy,sx,sy, colorfg);
}


void drawAnalogClock(int cx, int cy, int radius, int colorbg, int colorfg) {
  int h = getCurrentHour();
  if(h>12) h=h-12;
  int m = getCurrentMinute();
  int s = getCurrentSecond();
  drawAnalogClockFace(cx, cy, radius, colorbg, colorfg);
  if(GetSetting(SETTING_CLOCK_SECONDS)) drawAnalogClockSecondNeedle(s, cx, cy, radius, colorfg);
  drawAnalogClockMinuteNeedle(m, s, cx, cy, radius, colorfg);
  drawAnalogClockHourNeedle(h, m, s, cx, cy, radius, colorfg);
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

void drawHomeClock(int format, int fgcolor, int bgcolor) {
  char timeStr[14] = "";
  if(format == 0) {
    // show digital time and long date
    currentTimeToString(timeStr,GetSetting(SETTING_TIMEFORMAT));
    printCentered((unsigned char*)timeStr, 3*24, fgcolor, bgcolor);
    if (GetSetting(SETTING_THEME) == 1) {
      drawLongDate(90, NULL, COLOR_WHITE, COLOR_BLACK, 1);
    } else {
      drawLongDate(90, NULL, COLOR_BLACK, COLOR_WHITE, NULL);
    }
  } else if (format == 1) {
    // show digital time only
    currentTimeToString(timeStr,GetSetting(SETTING_TIMEFORMAT));
    printCentered((unsigned char*)timeStr, 4*24, fgcolor, bgcolor);
  } else if (format == 2) {
    // show digital time and short date
    currentTimeToString(timeStr,GetSetting(SETTING_TIMEFORMAT));
    printCentered((unsigned char*)timeStr, 3*24, fgcolor, bgcolor);
    currentDateToString(timeStr, GetSetting(SETTING_DATEFORMAT));
    printCentered((unsigned char*)timeStr, 5*24, fgcolor, bgcolor);
  } else if (format == 3) {
    // show long date only
    if (GetSetting(SETTING_THEME) == 1) {
      drawLongDate(LCD_HEIGHT_PX/2-30, NULL, COLOR_WHITE, COLOR_BLACK, 1);
    } else {
      drawLongDate(LCD_HEIGHT_PX/2-30, NULL, COLOR_BLACK, COLOR_WHITE, NULL);
    }
  } else if (format == 4) {
    // show short date only
    currentDateToString(timeStr, GetSetting(SETTING_DATEFORMAT));
    printCentered((unsigned char*)timeStr, 4*24, fgcolor, bgcolor);
  } else if (format == 5) {
    // show analog clock only
    int cx = LCD_WIDTH_PX/2;
    int cy = LCD_HEIGHT_PX/2;
    int radius = 70;
    drawAnalogClock(cx, cy, radius, bgcolor, fgcolor);
  } else if (format == 6) {
    // show analog clock with digital time
    int cx = 80;
    int cy = LCD_HEIGHT_PX/2;
    int radius = 70;
    drawAnalogClock(cx, cy, radius, bgcolor, fgcolor);
    currentTimeToString(timeStr,GetSetting(SETTING_TIMEFORMAT));
    char buffer[20] = "";
    strcpy(buffer, "  ");
    strcat(buffer, timeStr);
    PrintXY(10, 4, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
  } else if (format == 7) {
    // show analog clock with digital time and long date
    int cx = 89;
    int cy = 58+24;
    int radius = 50;
    drawAnalogClock(cx, cy, radius, bgcolor, fgcolor);
    currentTimeToString(timeStr,GetSetting(SETTING_TIMEFORMAT));
    char buffer[20] = "";
    strcpy(buffer, "  ");
    strcat(buffer, timeStr);
    PrintXY(10, 3, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
    if (GetSetting(SETTING_THEME) == 1) {
      drawLongDate(120, NULL, COLOR_WHITE, COLOR_BLACK, 1);
    } else {
      drawLongDate(120, NULL, COLOR_BLACK, COLOR_WHITE, NULL);
    }
  } else if (format == 8) {
    // show analog clock with digital time and short date
    int cx = 80;
    int cy = LCD_HEIGHT_PX/2;
    int radius = 70;
    drawAnalogClock(cx, cy, radius, bgcolor, fgcolor);
    currentTimeToString(timeStr,GetSetting(SETTING_TIMEFORMAT));
    char buffer[20] = "";
    strcpy(buffer, "  ");
    strcat(buffer, timeStr);
    PrintXY(10, 3, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
    currentDateToString(timeStr, GetSetting(SETTING_DATEFORMAT));
    strcpy(buffer, "  ");
    strcat(buffer, timeStr);
    PrintXY(10, 5, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
  } else if (format == 9) {
    // show analog clock with long date
    int cx = LCD_WIDTH_PX/2;
    int cy = 58+24;
    int radius = 50;
    drawAnalogClock(cx, cy, radius, bgcolor, fgcolor);
    if (GetSetting(SETTING_THEME) == 1) {
      drawLongDate(120, NULL, COLOR_WHITE, COLOR_BLACK, 1);
    } else {
      drawLongDate(120, NULL, COLOR_BLACK, COLOR_WHITE, NULL);
    }
  } else if (format == 10) {
    // show analog clock with short date
    int cx = LCD_WIDTH_PX/2;
    int cy = 58+24;
    int radius = 50;
    drawAnalogClock(cx, cy, radius, bgcolor, fgcolor);
    currentDateToString(timeStr, GetSetting(SETTING_DATEFORMAT));
    printCentered((unsigned char*)timeStr, 120+24, fgcolor, bgcolor);
  }
  // 11 is for showing nothing at all... so put nothing in VRAM.
}