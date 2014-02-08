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
#include "textGUI.hpp"
#include "menuGUI.hpp"

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

inline static double sine(double x)
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
inline static double cosine(double x) {
  return sine(M_PI/2-x);
}
inline static void drawAnalogClockFace(int cx, int cy, int radius, int colorbg, int colorfg) {
  // may be extended in the future
  drawFilledCircle(cx, cy, radius, colorfg);
  drawFilledCircle(cx, cy, radius-2, colorbg);
  int theta=0,i=0;
  double x,y,x1,y1;
  do{
    x=cx+(radius-radius/10.0)*cosine(theta*M_PI/180);
    x1=cx+(radius-radius/12.0)*cosine(theta*M_PI/180);
    y=cy+(radius-radius/10.0)*sine(theta*M_PI/180);
    y1=cy+(radius-radius/12.0)*sine(theta*M_PI/180);
    drawLine(x,y,x1,y1,COLOR_GRAY);
    // Increase angle by 30 degrees, which is the circular distance between each numeric point:
    theta+=30;
    i++;

  } while(i!=12); //LIMIT NUMERIC POINTS UPTO =12= Numbers.
}
inline static void drawAnalogClockSecondNeedle(int s, int cx, int cy, double radius, int colorfg) {
  double angle=-90.0;
  double sx,sy;
  double length = radius - radius/8.0;
  sx=cx+length*cosine((angle+s*6.0)*M_PI/180.0);
  sy=cy+length*sine((angle+s*6.0)*M_PI/180.0);
  drawLine(cx,cy,sx,sy,colorfg);
}

inline static void drawAnalogClockMinuteNeedle(int m, int s, int cx, int cy, double radius, int colorfg) {
  double angle=-90;
  double sx,sy;
  double length = radius - radius/5.0;
  sx=cx+length*cosine((angle+m*6+(s*6/60))*M_PI/180);
  sy=cy+length*sine((angle+m*6+(s*6/60))*M_PI/180);
  drawLine(cx,cy,sx,sy, colorfg);
}

inline static void drawAnalogClockHourNeedle(int h, int m, int s, int cx, int cy, double radius, int colorfg) {
  double angle=-90;
  double sx,sy;
  double length = radius - radius/2.5;
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
    year.min = 1970; //don't allow to set below 1970 so it is Unix-time compatible and always has 4 digits
    year.max = 9999;
    year.cycle = 0;
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
    day.max = ( month.value==2? (isLeap(year.value)? 29 : 28) : getMonthDays(month.value));
    int res = doSelector(&day);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop date adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  setDate(year.value, month.value, day.value); 
}


void RTCunadjustedWizard(int helpMessage) {
  //first check if RTC is unadjusted. if not, return.
  if(!getRTCisUnadjusted()) return;
  DisplayStatusArea();
  if(helpMessage) {
    textArea text;
    strcpy(text.title, (char*)"Clock unadjusted");
    
    textElement elem[5];
    text.elements = elem;
    text.allowEXE = 1; text.allowF1 = 1;
    text.scrollbar = 0;
    
    elem[0].text = (char*)"Looks like this calculator's clock isn't set correctly, because it is set to a date in the past.";
    elem[0].spaceAtEnd = 1;
    elem[1].text = (char*)"This may be because you took off the batteries or restarted the calculator.";
    elem[2].newLine = 1;
    elem[2].lineSpacing = 8;
    elem[2].text = (char*)"Press EXE or F1 to start setting the time and date, or EXIT to ignore and continue.";
    
    text.numelements = 3;
    int res = doTextArea(&text);
    if(res == 0) {
      return;
    } // else keep with program execution
  }
  
  textArea text;
  strcpy(text.title, (char*)"Clock unadjusted");
  
  textElement elem[5];
  text.elements = elem;
  text.allowEXE = 1; text.allowF1 = 1;
  text.scrollbar = 0;
  
  elem[0].text = (char*)"When setting the date and time, use the up/down keys to change the displayed value, and EXE to confirm.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)"Press EXE to start.";
  
  text.numelements = 2;
  doTextArea(&text);
  
  setTimeGUI(0);
  
  elem[0].text = (char*)"Now let's set the date.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)"Press EXE to continue.";
  
  text.numelements = 2;
  doTextArea(&text);
  
  setDateGUI(0);
  
  strcpy(text.title, (char*)"Clock adjusted");
  elem[0].text = (char*)"We're done!";
  elem[1].newLine = 1;
  elem[1].text = (char*)"If you ever need to adjust the clock again press Shift->Menu and choose the appropriate options.";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 8;
  elem[2].text = (char*)"Press EXE or EXIT to close.";
  
  text.numelements = 3;
  doTextArea(&text);
}

void currentTimeToBasicVar() {
  TBCD Src;
  double fhour=0.0,fminute=0.0,fsecond=0.0,fmillisecond=0.0,hourfraction = 0;
  
  fhour = getCurrentHour(); fminute = getCurrentMinute(); fsecond = getCurrentSecond(); fmillisecond = getCurrentMillisecond();
  hourfraction = fhour+fminute/60.0+fsecond/(60.0*60.0)+fmillisecond/(60.0*60.0*1000.0);
  Src.Set( hourfraction );
  Alpha_SetData( 'T', &Src );
  mMsgBoxPush(4);
  mPrintXY(3, 2, (char*)"Time fraction", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 3, (char*)"saved to alpha", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 4, (char*)"variable T.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
  closeMsgBox();
}

void drawHomeClock(int format, int theme) {
  char timeStr[14] = "";
  int fgcolor = COLOR_BLACK;
  int bgcolor = COLOR_WHITE;
  if(theme == 1) { 
    fgcolor = COLOR_WHITE;
    bgcolor = COLOR_BLACK;
  }
  switch(format) {
    case 0:
    case 1:
    case 2:
    case 6:
    case 7:
    case 8:
      currentTimeToString(timeStr,GetSetting(SETTING_TIMEFORMAT));
      break;
    case 4:
    case 10:
      currentDateToString(timeStr, GetSetting(SETTING_DATEFORMAT));
      break;
  }
  switch(format) {
    case 0:
      // show digital time and long date
      printCentered((unsigned char*)timeStr, 3*24, fgcolor, bgcolor);
      drawLongDate(90, NULL, fgcolor, bgcolor, theme);
      break;
    case 1:
      // show digital time only
      printCentered((unsigned char*)timeStr, 4*24, fgcolor, bgcolor);
      break;
    case 2:
      // show digital time and short date
      printCentered((unsigned char*)timeStr, 3*24, fgcolor, bgcolor);
      currentDateToString(timeStr, GetSetting(SETTING_DATEFORMAT));
      printCentered((unsigned char*)timeStr, 5*24, fgcolor, bgcolor);
      break;
    case 3:
      // show long date only
      drawLongDate(LCD_HEIGHT_PX/2-30, NULL, fgcolor, bgcolor, theme);
      break;
    case 4:
      // show short date only
      printCentered((unsigned char*)timeStr, 4*24, fgcolor, bgcolor);
      break;
    case 5:
      // show analog clock only
      drawAnalogClock(LCD_WIDTH_PX/2, LCD_HEIGHT_PX/2, 70, bgcolor, fgcolor);
      break;
    case 6:
      // show analog clock with digital time
      drawAnalogClock(80, LCD_HEIGHT_PX/2, 70, bgcolor, fgcolor);
      mPrintXY(10, 4, timeStr, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
      break;
    case 7:
      // show analog clock with digital time and long date
      drawAnalogClock(89, 58+24, 50, bgcolor, fgcolor);
      mPrintXY(10, 3, timeStr, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
      drawLongDate(120, NULL, fgcolor, bgcolor, theme);
      break;
    case 8:
      // show analog clock with digital time and short date
      drawAnalogClock(80, LCD_HEIGHT_PX/2, 70, bgcolor, fgcolor);
      mPrintXY(10, 3, timeStr, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
      currentDateToString(timeStr, GetSetting(SETTING_DATEFORMAT));
      mPrintXY(10, 5, timeStr, TEXT_MODE_TRANSPARENT_BACKGROUND, fgcolor);
      break;
    case 9:
      // show analog clock with long date
      drawAnalogClock(LCD_WIDTH_PX/2, 58+24, 50, bgcolor, fgcolor);
      drawLongDate(120, NULL, fgcolor, bgcolor, theme);
      break;
    case 10:
      // show analog clock with short date
      drawAnalogClock(LCD_WIDTH_PX/2, 58+24, 50, bgcolor, fgcolor);
      printCentered((unsigned char*)timeStr, 120+24, fgcolor, bgcolor);
      break;
    // 11 is for showing nothing at all... so put nothing in VRAM.
  }
}