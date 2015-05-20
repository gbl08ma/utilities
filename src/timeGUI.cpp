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

const char* getDayEnding(int day) {
  //st,nd,rd,th code:
  switch(day) {
    case 1:
    case 21:
    case 31:
      return "st";
    case 2:
    case 22:
      return "nd";
    case 3:
    case 23:
      return "rd";
    default:
      return "th";
      break;
  }
}

void drawLongDate(int textY, int format, int colorfg, int colorbg, int miniminiinvert) {
  if (format==NULL) format = getSetting(SETTING_LONGDATEFORMAT);
  // Draw long date as seen on the home screen, according to format.
  // textY is the y coordinate at which the date should start to be drawn.
  // x coordinate is set automatically (text is always centered on screen)
  // NOTE: does not update VRAM contents to screen
  int curYear = getCurrentYear();  
  int curDay = getCurrentDay();

  char buffer[50];
  switch(format) {
    case 0:
      sprintf(buffer, "%s, %s %d", getCurrentDOWAsString(), getCurrentMonthAsString(), curDay);
      break;
    case 1:
    case 3:
      sprintf(buffer, "%s, %d%s %s", getCurrentDOWAsString(), curDay, getDayEnding(curDay), getCurrentMonthAsString());
      break;
    case 2:
      sprintf(buffer, "%s, %d%s %s %d", getCurrentDOWAsString(), curDay, getDayEnding(curDay), getCurrentMonthAsString(), curYear);
      break;
    case 4:
    case 6: //it's exactly the same except minimini is not used, but that's not handled here
      sprintf(buffer, "%s %d", getCurrentMonthAsString(), curDay);
      break;
    case 5:
      sprintf(buffer, "%s %d, %d", getCurrentMonthAsString(), curDay, curYear);
      break;
    case 7:
    case 9: //exactly the same except minimini, not handled here
      sprintf(buffer, "%d%s %s", curDay, getDayEnding(curDay), getCurrentMonthAsString());
      break;
    case 8:
      sprintf(buffer, "%d%s %s %d", curDay, getDayEnding(curDay), getCurrentMonthAsString(), curYear);
      break;
  }

  int textX = 0;
  //Use PrintMini to get length in pixels of the string, then draw it on the middle of the screen
  PrintMini(&textX, &textY, buffer, 0, 0xFFFFFFFF, 0, 0, colorfg, colorbg, 0, 0); //get length
  textX = LCD_WIDTH_PX/2 - textX/2; //center
  PrintMini(&textX, &textY, buffer, 0, 0xFFFFFFFF, 0, 0, colorfg, colorbg, 1, 0); //draw
  
  if (format == 0 || format == 1 || format == 4 || format == 7) {
    // draw year in minimini font
    char buffer2[10];
    itoa(curYear, (unsigned char*)buffer2);
    int newTextX = 0;
    textY += 17;
    PrintMiniMini( &newTextX, &textY, buffer2, 0, TEXT_COLOR_BLACK, 1 ); //fake draw
    textX -= newTextX;
    PrintMiniMini( &textX, &textY, buffer2, (miniminiinvert == 1 ? 4 : 0), TEXT_COLOR_BLACK, 0 ); //draw
  }
  return;
}

void drawAnalogClockFace(int cx, int cy, int radius, int colorbg, int colorfg, int hourmarks) {
  // may be extended in the future
  drawFilledCircle(cx, cy, radius, colorfg);
  drawFilledCircle(cx, cy, radius-2, colorbg);
  int theta=0,i=0,increment=360/hourmarks;
  if(hourmarks) do{
    double x=cx+(radius-radius/10.0)*cosine(theta*M_PI/180);
    double x1=cx+(radius-radius/12.0)*cosine(theta*M_PI/180);
    double y=cy+(radius-radius/10.0)*sine(theta*M_PI/180);
    double y1=cy+(radius-radius/12.0)*sine(theta*M_PI/180);
    drawLine(x,y,x1,y1,COLOR_GRAY);
    // Increase angle by the circular distance between each numeric point:
    theta+=increment;
    i++;

  } while(i!=hourmarks); //LIMIT NUMERIC POINTS UPTO =12= Numbers.
}
void drawAnalogClockSecondNeedle(int s, int cx, int cy, int radius, int colorfg) {
  double angle=-90.0;
  double sx,sy;
  double length = radius - radius/8.0;
  sx=cx+length*cosine((angle+s*6)*M_PI/180.0);
  sy=cy+length*sine((angle+s*6)*M_PI/180.0);
  drawLine(cx,cy,sx,sy,colorfg);
}

void drawAnalogClockMinuteNeedle(int m, int s, int cx, int cy, int radius, int colorfg) {
  double angle=-90;
  double sx,sy;
  double length = radius - radius/5.0;
  sx=cx+length*cosine((angle+m*6+(s*6/60))*M_PI/180);
  sy=cy+length*sine((angle+m*6+(s*6/60))*M_PI/180);
  drawLine(cx,cy,sx,sy, colorfg);
}

void drawAnalogClockHourNeedle(int h, int m, int cx, int cy, int radius, int colorfg, int ischrono) {
  double angle=-90;
  double sx,sy;
  double length = radius - radius/(ischrono? 5.0 : 2.5);
  sx=cx+length*cosine((angle+h*(ischrono?15:30)+(m*(ischrono?15:30)/60))*M_PI/180);
  sy=cy+length*sine((angle+h*(ischrono?15:30)+(m*(ischrono?15:30)/60))*M_PI/180);
  drawLine(cx,cy,sx,sy, colorfg);
}


void drawAnalogClock(int cx, int cy, int radius, int colorbg, int colorfg) {
  int h = getCurrentHour();
  if(h>12) h=h-12;
  int m = getCurrentMinute();
  int s = getCurrentSecond();
  drawAnalogClockFace(cx, cy, radius, colorbg, colorfg, 12);
  if(getSetting(SETTING_CLOCK_SECONDS)) drawAnalogClockSecondNeedle(s, cx, cy, radius, colorfg ^ COLOR_INDIANRED);
  drawAnalogClockMinuteNeedle(m, s, cx, cy, radius, colorfg);
  drawAnalogClockHourNeedle(h, m, cx, cy, radius, colorfg, 0);
}

void drawAnalogChronometer(int cx, int cy, int radius, int colorbg, int colorfg, int d, int h, int m, int sPlusMs) {
  // draw bigger dial
  drawAnalogClockFace(cx, cy, radius, colorbg, colorfg, 60);
  // draw left dial (minutes)
  drawAnalogClockFace(cx-radius*2, cy, (radius*2)/3, colorbg, colorfg, 60);
  int textX = cx-radius*2-5, textY = cy-radius + 24 - 7;
  PrintMiniMini( &textX, &textY, (char*)"M", 0, TEXT_COLOR_BLACK, 0 );
  // draw right dial (hours)
  drawAnalogClockFace(cx+radius*2, cy, (radius*2)/3, colorbg, colorfg, 24);
  textX = cx+radius*2-4; textY = cy-radius + 24 - 7;
  PrintMiniMini( &textX, &textY, (char*)"H", 0, TEXT_COLOR_BLACK, 0 );
  // print days
  if(d) {
    textX = cx; textY = cy;
    if(d < 100) textX += radius - radius / 3; // (2/3)*radius hopefully with less rounding errors
    else if(d < 1000) textX += radius / 2;
    else if(d < 100000) textX += radius / 3;
    textY -= 4 + 24;
    char buffer[10];
    itoa(d, (unsigned char*)buffer);
    PrintMiniMini( &textX, &textY, buffer, 0, TEXT_COLOR_BLACK, 0 );
  }
  // seconds needle on bigger dial
  //drawAnalogClockSecondNeedle(s, cx, cy, radius, colorfg);
  double angle=-90.0;
  double length = radius - radius/8.0;
  drawLine(cx,cy,
    cx+length*cosine((angle+(double)sPlusMs*0.006)*M_PI/180.0),
    cy+length*sine((angle+(double)sPlusMs*0.006)*M_PI/180.0),
    colorfg);
  // minutes on top
  drawAnalogClockMinuteNeedle(m, sPlusMs/1000, cx-radius*2, cy, (radius*2)/3, colorfg);
  // hours on bottom
  drawAnalogClockHourNeedle(h, m, cx+radius*2, cy, (radius*2)/3, colorfg, 1);
}

void setTimeScreen(int canExit) {
  Selector hour;
  Selector minute;
  Selector second;
  while(1) {
    hour.title = (char*)"Set time";
    hour.subtitle = (char*)"Hour";
    hour.value = getCurrentHour();
    hour.min = 0;
    hour.max = 23;
    int res = doSelector(&hour);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop time adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  while(1) {
    minute.title = (char*)"Set time";
    minute.subtitle = (char*)"Minute";
    minute.value = getCurrentMinute();
    minute.min = 0;
    minute.max = 59;
    int res = doSelector(&minute);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop time adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }  
  while(1) {
    second.title = (char*)"Set time";
    second.subtitle = (char*)"Second";
    second.value = getCurrentSecond();
    second.min = 0;
    second.max = 59;
    int res = doSelector(&second);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop time adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }  
  setTime(hour.value, minute.value, second.value);
}

void setDateScreen(int canExit) {
  Selector year;
  Selector month;
  Selector day;
  while(1) {
    year.title = (char*)"Set date";
    year.subtitle = (char*)"Year";
    year.value = getCurrentYear();
    year.min = 1970; //don't allow to set below 1970 so it is Unix-time compatible and always has 4 digits
    year.max = HIGHEST_SUPPORTED_YEAR;
    year.cycle = 0;
    int res = doSelector(&year);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop date adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  while(1) {
    month.title = (char*)"Set date";
    month.subtitle = (char*)"Month";
    month.value = getCurrentMonth();
    month.min = 1;
    month.max = 12;
    month.type = SELECTORTYPE_MONTH;
    int res = doSelector(&month);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop date adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  while(1) {
    day.title = (char*)"Set date";
    day.subtitle = (char*)"Day";
    day.value = getCurrentDay();
    day.min = 1;
    day.max = getMonthDaysWithLeap(month.value, year.value);
    int res = doSelector(&day);
    if (res == SELECTOR_RETURN_EXIT && canExit) return; // stop date adjustment
    if (res == SELECTOR_RETURN_SELECTION) break;
  }
  setDate(year.value, month.value, day.value); 
}


void RTCunadjustedWizard(int helpMessage, int ignoreAdjusted) {
  //first check if RTC is unadjusted. if not, return.
  if(!ignoreAdjusted && !isRTCunadjusted()) return;
  if(helpMessage) {
    textArea text;
    text.title = (char*)"Clock unadjusted";
    
    textElement elem[3];
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
    if(!doTextArea(&text)) return;
    // else keep with program execution
  }
  
  textArea text;
  text.title = (char*)"Clock unadjusted";
  
  textElement elem[3];
  text.elements = elem;
  text.allowEXE = 1; text.allowF1 = 1;
  text.scrollbar = 0;
  
  elem[0].text = (char*)"When setting the date and time, use the up/down keys to change the displayed value, and EXE to confirm.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)"Press EXE to start.";
  
  text.numelements = 2;
  doTextArea(&text);
  
  setTimeScreen(0);
  
  elem[0].text = (char*)"Now let's set the date.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)"Press EXE to continue.";
  
  doTextArea(&text);
  
  setDateScreen(0);
  
  elem[0].text = (char*)"We're done!\nIf you ever need to adjust the clock again press Shift->Menu and choose the appropriate options.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)"Press EXE or EXIT to continue.";
  
  doTextArea(&text);
}

void currentTimeToBasicVar() {
  TBCD Src;
  double fhour=0.0,fminute=0.0,fsecond=0.0,fmillisecond=0.0,hourfraction = 0;
  
  fhour = getCurrentHour();
  fminute = getCurrentMinute();
  fsecond = getCurrentSecond();
  fmillisecond = getCurrentMillisecond();
  hourfraction = fhour+fminute/60.0+fsecond/(60.0*60.0)+fmillisecond/(60.0*60.0*1000.0);
  Src.Set(hourfraction);
  Alpha_SetData('T', &Src);
  mMsgBoxPush(4);
  multiPrintXY(3, 2, "Time fraction\nsaved to alpha\nvariable T.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
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
      currentTimeToString(timeStr);
      break;
    case 4:
    case 10:
      currentDateToString(timeStr);
      break;
  }
  switch(format) {
    case 0:
      // show digital time and long date
      printCentered(timeStr, 3*24, fgcolor, bgcolor);
      drawLongDate(90, NULL, fgcolor, bgcolor, theme);
      break;
    case 1:
      // show digital time only
      printCentered(timeStr, 4*24, fgcolor, bgcolor);
      break;
    case 2:
      // show digital time and short date
      printCentered(timeStr, 3*24, fgcolor, bgcolor);
      currentDateToString(timeStr);
      printCentered(timeStr, 5*24, fgcolor, bgcolor);
      break;
    case 3:
      // show long date only
      drawLongDate(LCD_HEIGHT_PX/2-30, NULL, fgcolor, bgcolor, theme);
      break;
    case 4:
      // show short date only
      printCentered(timeStr, 4*24, fgcolor, bgcolor);
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
      currentDateToString(timeStr);
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
      printCentered(timeStr, 120+24, fgcolor, bgcolor);
      break;
    // 11 is for showing nothing at all... so put nothing in VRAM.
  }
}

static const char *tzMinutes[] = {"00", "15", "30", "45"};

void setTimezone() {
  Bdisp_AllClr_VRAM();
  textArea text;
  text.y = 5*24+2;
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.scrollbar = 0;
  textElement elem[5];
  text.elements = elem;
  elem[0].text = (char*)"Adjust the setting so that the time on the status bar matches with the UTC time. The latter can be seen at http://time.is/UTC";
  text.numelements = 1;
  doTextArea(&text);

  Menu menu;
  menu.type = MENUTYPE_INSTANT_RETURN;
  menu.title = (char*)"Set Timezone";
  menu.scrollout = 1;
  menu.height = 5;
  MenuItem items[109];
  menu.items = items;
  // build list with possible offsets:
  // (from UTC-12:45 to UTC+14:00)
  // actually, the following code tries to build up to +14:45, but it stops as i reaches 108.
  char zonestr[109][15];
  int hour = -12;
  int mins = 0;
  char buffer[5];
  int hasRepeatedZero = 0;
  for(int i = 0; i < 109; i++) {
    strcpy(zonestr[i], (char*)"UTC");
    if(hasRepeatedZero) strcat(zonestr[i], (char*)"+");
    else strcat(zonestr[i], (char*)"-");
    if(abs(hour) < 10) strcat(zonestr[i], (char*)"0");
    itoa(abs(hour), (unsigned char*)buffer);
    strcat(zonestr[i], buffer);
    strcat(zonestr[i], (char*)":");
    if(!hasRepeatedZero) strcat(zonestr[i], tzMinutes[3-mins]);
    else strcat(zonestr[i], tzMinutes[mins]);
    items[i].text = zonestr[i];
    if(++mins == 4) {
      mins = 0;
      if(++hour == 15) break;
      if(hour == 1 && !hasRepeatedZero) {
        hour = 0;
        i--; // avoid having both +00:00 and -00:00
        hasRepeatedZero = 1;
      }
    }
  }
  menu.numitems = 108;
  menu.selection = getSetting(SETTING_TIMEZONE)+1;
  while(1) {
    long long int currentUTC = currentUEBT() - (long long int)(menu.selection - 52) * 15LL*60LL*1000LL;
    int hours, minutes;
    long long int seconds;
    seconds = currentUTC / 1000;
    currentUTC %= 1000;
    minutes = (int)(seconds / 60);
    seconds %= 60;
    hours = minutes / 60;
    minutes %= 60;
    hours %= 24;
    char buffer[50];
    strcpy(buffer, (char*)"UTC time: ");
    timeToString(buffer, hours, minutes, seconds, getSetting(SETTING_TIMEFORMAT), 0);
    DefineStatusMessage(buffer, 1, 0, 0);
    int r = doMenu(&menu);
    if(r == MENU_RETURN_EXIT) {
      DefineStatusMessage((char*)"", 1, 0, 0);
      return;
    }
    else if(r == MENU_RETURN_SELECTION) break;
  }
  setSetting(SETTING_TIMEZONE, menu.selection-1, 1);
  DefineStatusMessage((char*)"", 1, 0, 0);
}