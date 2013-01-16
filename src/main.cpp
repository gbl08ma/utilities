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
#include "unixtime.hpp"
#include "events.hpp"
#include "lock.hpp"
#include "toksplit.h"
#include "ftoa.c"
#include "sprites.h"
#include "version.h"
#define CALENDAR_FOLDER (const char*)"@CALNDAR"
#define SMEM_CALENDAR_FOLDER (const char*)"@CALNDAR"

// part of cpu clocking code
void change_freq(int mult) { 
   __asm__( 
      "mov r4, r0\n\t" 
      "and #0x3F, r0\n\t"  
      "shll16 r0\n\t"  
      "shll8 r0\n\t"  
      "mov.l frqcr, r1\n\t"   
      "mov.l pll_mask, r3\n\t"   
      "mov.l @r1, r2\n\t"   
      "and r3, r2\n\t"   
      "or r0, r2\n\t"  
      "mov.l r2, @r1\n\t"  
      "mov.l frqcr_kick_bit, r0\n\t"  
      "mov.l @r1, r2\n\t" 
      "or r0, r2\n\t" 
      "rts\n\t" 
      "mov.l r2, @r1\n\t" 
      ".align 4\n\t" 
      "frqcr_kick_bit: .long 0x80000000\n\t" 
      "pll_mask: .long 0xC0FFFFFF\n\t"  
      "frqcr: .long 0xA4150000\n\t" 
   ); 
}
// end of cpu clocking code
// SETTINGS engine
/*App settings.*/
int setting_timeformat = 0; // 0 = 24-hour HH:MM:SS ; 1 = 12-hour HH:MM:SS AM/PM

int setting_longdateformat = 0;
/* 0 = "Weekday, Month 12"&mini 4-digits year below
   1 = "Weekday, 12th Month"&mini 4-digits year below
   2 = "Weekday, 12th Month 2121" (year not mini)
   3 = "Weekday, 12th Month" (no year)
   4 = "Month 12"&mini 4-digits year below
   5 = "Month 12, 2012" (year not mini)
   6 = "Month 12" (no year)
   7 = "12th Month"&mini 4-digits year below
   8 = "12th Month 2012" (year not mini)
   9 = "12th Month" (no year)
*/

int setting_dateformat = 0;
/* 0 = DD/MM/YYYY
   1 = MM/DD/YYYY
   2 = YYYY/MM/DD
*/

int setting_debug_on = 0; //NOT saved or loaded from settings files (meaning it's volatile), and can only be toggled with master control. Defines whether Test Mode can be accessed with Shift+OPTN.
int setting_black_theme = 0; //whether the home screen (and later, possibly other things too) should use a dark/inverted theme.
int setting_display_statusbar = 1; //whether the status bar should be displayed
unsigned int setting_startup_brightness = 250; //screen brightness level to force on add-in startup, 0~249. 250 for no forcing
int setting_show_advanced = 0; //whether to show advanced things like CPU clock selection
int setting_display_fkeys = 1;
int setting_password_show_last_char = 1; //whether to unhide the last input character when entering passwords
int setting_show_events_count_on_calendar = 1;
int setting_is_first_run = 1; //should be 1 for the first time the add-in is run, then it's set to zero and never changed
int setting_enable_lock_func = 1; //whether lock functionality is available or not. Should always be on and not changeable on the settings menu, except when a specific code is entered on master-control in order to disable it to make people who don't want the lock, because they accidentally lock their calculators, happier (I'm thinking of you Catarina...)
int setting_lock_auto_turnoff = 0; //whether to turn off automatically after locking the calc
int setting_lock_on_exe = 0; //when enabled, calculator is locked when EXE is pressed on the home screen (i.e. legacy support for people used to the old lock add-in).
int setting_unlock_runmat = 0; //whether to jump to Run-Mat when calculator is unlocked
/*End of settings*/

#define DIRNAME (unsigned char*)"@UTILS"
#define SETTINGSFILE (unsigned char*)"Set"
// Routines for saving and loading settings

void LoadSettings(unsigned char* file) {
  int size;
  MCSGetDlen2(DIRNAME, file, &size);
  if (size == 0) { return; }
  
  unsigned char buffer[15];
  MCSGetData1(0, 11, buffer);
  setting_timeformat = buffer[0];
  setting_longdateformat = buffer[1];
  setting_dateformat = buffer[2];
  setting_black_theme = buffer[3];
  setting_display_statusbar = buffer[4];
  setting_startup_brightness = buffer[5];
  setting_show_advanced = buffer[6];
  setting_display_fkeys = buffer[7];
  setting_password_show_last_char = buffer[8];
  setting_show_events_count_on_calendar = buffer[9];
  setting_is_first_run = buffer[10];
  setting_enable_lock_func = buffer[11];
  setting_lock_auto_turnoff = buffer[12];
  setting_lock_on_exe = buffer[13];
  setting_unlock_runmat = buffer[14];
}

void SaveSettings(unsigned char* file, int nofirstrun=1) {

  int createResult = MCS_CreateDirectory( DIRNAME );

  if(createResult != 0) // Check directory existence
  { // directory already exists, so delete the exiting file that may be there
    MCSDelVar2(DIRNAME, file);
  }
  if(nofirstrun) setting_is_first_run = 0; //if we're saving settings, then for sure we have completed the first run wizard.
  //...except if we really want to say it's still the first run...
  unsigned char buffer[15] = {
    setting_timeformat,
    setting_longdateformat,
    setting_dateformat,
    setting_black_theme,
    setting_display_statusbar,
    setting_startup_brightness,
    setting_show_advanced,
    setting_display_fkeys,
    setting_password_show_last_char,
    setting_show_events_count_on_calendar,
    setting_is_first_run,
    setting_enable_lock_func,
    setting_lock_auto_turnoff,
    setting_lock_on_exe,
    setting_unlock_runmat
  };
  MCSPutVar2(DIRNAME, file, 15, buffer);
}
unsigned int halfSeconds = 0;
int halfSecondTimer = 0;
void halfSecondTimerHandler() {
  halfSeconds++;
}
// end of settings-related code
// RTC, CLOCK and CALENDAR CODE
// Thanks to AHelper and http://prizm.cemetech.net/index.php?title=RTC_Unit for these
#define RYRCNT  (unsigned short*)0xA413FECE
#define RMONCNT (unsigned char*)0xA413FECC
#define RDAYCNT (unsigned char*)0xA413FECA
 
#define RHRCNT  (unsigned char*)0xA413FEC6
#define RMINCNT (unsigned char*)0xA413FEC4
#define RSECCNT (unsigned char*)0xA413FEC2
#define RWKCNT (unsigned char*)0xA413FEC8
#define RCR2 (unsigned char*)0xA413FEDE

const char *dayofweek[] = {"Sunday",
                           "Monday",
                           "Tuesday",
                           "Wednesday",
                           "Thursday",
                           "Friday",
                           "Saturday"
                          };
const char *dayofweekshort[] = {"SUN",
                                "MON",
                                "TUE",
                                "WED",
                                "THU",
                                "FRI",
                                "SAT"
                               };
const char *monthNames[] = {"January",
                            "February",
                            "March",
                            "April",
                            "May",
                            "June",
                            "July",
                            "August",
                            "September",
                            "October",
                            "November",
                            "December"
                           }; 
const char monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

unsigned int bcd_to_2digit(unsigned char* bcd) {
        return ((((*(bcd))&0xf0)>>4)*10) + ((*(bcd))&0x0f);
}
unsigned int bcd_to_4digit(unsigned char* bcd) {
        return bcd_to_2digit(bcd)*100 + bcd_to_2digit(bcd+1);
}
const char* getDOW()
{
    return dayofweek[(*RWKCNT & 0b111)];
}

const char* getMonth()
{
    return monthNames[((*RMONCNT & 0b10000)>>4)*10 + (*RMONCNT & 0b1111) - 1];
}
bool isLeap(int y)
{
    return ( ((y % 4) == 0) && ((y % 100) != 0) ) || ((y % 400) == 0);
}
// From wikipedia for the Keith and Craver method
int dow(int y, int m, int d)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + (y / 4) - (y / 100) + (y / 400) + t[m-1] + d) % 7;
}

void blockForTicks(int ticks) { //stop program execution for n ticks (1 tick = 1/128 s)
  int ot = RTC_GetTicks();
  while (RTC_GetTicks()-ot <= ticks) {}
}
void fillDate(char *buffer, int format=setting_dateformat)
{
  switch(format)
  {
    case 0: // DD/MM/YYYY
      buffer[0] = '0' + ((*RDAYCNT >> 4) & 0b11);
      buffer[1] = '0' + (*RDAYCNT & 0b1111);
      buffer[2] = '/';
      buffer[3] = '0' + ((*RMONCNT >> 4) & 0b1);
      buffer[4] = '0' + (*RMONCNT & 0b1111);
      buffer[5] = '/';
      buffer[6] = '0' + ((*RYRCNT >> 12) & 0b1111);
      buffer[7] = '0' + ((*RYRCNT >>  8) & 0b1111);
      buffer[8] = '0' + ((*RYRCNT >>  4) & 0b1111);
      buffer[9] = '0' + (*RYRCNT & 0b1111);
      buffer[10] = '\0';
      break;
    case 1: // MM/DD/YYYY
      buffer[0] = '0' + ((*RMONCNT >> 4) & 0b1);
      buffer[1] = '0' + (*RMONCNT & 0b1111);
      buffer[2] = '/';
      buffer[3] = '0' + ((*RDAYCNT >> 4) & 0b11);
      buffer[4] = '0' + (*RDAYCNT & 0b1111);
      buffer[5] = '/';
      buffer[6] = '0' + ((*RYRCNT >> 12) & 0b1111);
      buffer[7] = '0' + ((*RYRCNT >>  8) & 0b1111);
      buffer[8] = '0' + ((*RYRCNT >>  4) & 0b1111);
      buffer[9] = '0' + (*RYRCNT & 0b1111);
      buffer[10] = '\0';
      break;
    case 2: // YYYY/MM/DD
      buffer[0] = '0' + ((*RYRCNT >> 12) & 0b1111);
      buffer[1] = '0' + ((*RYRCNT >>  8) & 0b1111);
      buffer[2] = '0' + ((*RYRCNT >>  4) & 0b1111);
      buffer[3] = '0' + (*RYRCNT & 0b1111);
      buffer[4] = '/';
      buffer[5] = '0' + ((*RMONCNT >> 4) & 0b1);
      buffer[6] = '0' + (*RMONCNT & 0b1111);
      buffer[7] = '/';
      buffer[8] = '0' + ((*RDAYCNT >> 4) & 0b11);
      buffer[9] = '0' + (*RDAYCNT & 0b1111);
      buffer[10] = '\0';
      break;
  }
}

void fillCustomDate(char *buffer, int y, int m, int d, int format=setting_dateformat)
{
  char day[2];
  char month[2];
  char year[6];
  itoa(y, (unsigned char*)year);
  itoa(m, (unsigned char*)month);
  itoa(d, (unsigned char*)day);
  switch(format)
  {
    case 0: // DD/MM/YYYY
    
      if (d < 10) { strcat(buffer, "0"); }
      strcat(buffer, day);
      strcat(buffer, "/");
      
      if (m < 10) { strcat(buffer, "0"); }
      strcat(buffer, month);
      strcat(buffer, "/");
      
      strcat(buffer, year);
      break;
    case 1: // MM/DD/YYYY
      if (m < 10) { strcat(buffer, "0"); }
      strcat(buffer, month);
      strcat(buffer, "/");
      if (d < 10) { strcat(buffer, "0"); }
      strcat(buffer, day);
      strcat(buffer, "/");
      strcat(buffer, year);
      break;
    case 2: // YYYY/MM/DD
      strcat(buffer, year);
      strcat(buffer, "/");
      if (m < 10) { strcat(buffer, "0"); }
      strcat(buffer, month);
      strcat(buffer, "/");
      if (d < 10) { strcat(buffer, "0"); }
      strcat(buffer, day);

      break;
  }
}
void fillCustomTime(char *buffer, int h, int min, int sec, int format=setting_timeformat)
{
  char hour[2];
  char minute[2];
  char second[2];
  itoa(h, (unsigned char*)hour);
  itoa(min, (unsigned char*)minute);
  itoa(sec, (unsigned char*)second);
  switch(format)
  {
    case 0: // HH:MM:SS 24-hour
    
      if (h < 10) { strcat(buffer, "0"); }
      strcat(buffer, hour);
      strcat(buffer, ":");
      
      if (min < 10) { strcat(buffer, "0"); }
      strcat(buffer, minute);
      strcat(buffer, ":");
      
      if (sec < 10) { strcat(buffer, "0"); }
      strcat(buffer, second);
      break;
    case 1: // HH:MM:SS AM/PM 12-hour
      int pm = 0;
      if (h >= 12) {
        pm = 1;
        h = h - 12;
        if (h == 0) h = 12;
        itoa(h, (unsigned char*)hour);
      }
      if (h < 10) { strcat(buffer, "0"); }
      strcat(buffer, hour);
      strcat(buffer, ":");
      
      if (min < 10) { strcat(buffer, "0"); }
      strcat(buffer, minute);
      strcat(buffer, ":");
      
      if (sec < 10) { strcat(buffer, "0"); }
      strcat(buffer, second);
      
      if(pm) {
        strcat(buffer, " PM");
      } else {
        strcat(buffer, " AM");
      }
      break;
  }
}

void drawLongDate(int textY, int format=setting_longdateformat, int colorfg=COLOR_BLACK, int colorbg=COLOR_WHITE, int miniminiinvert=0) {
  // Draw long date as seen on the home screen, according to settings (or override a format).
  // textY is the y coordinate at which the date should start to be drawn.
  // x coordinate is set automatically (text is always centered on screen)
  // NOTE: does not update VRAM contents to screen
  int curYear = ((*RYRCNT >> 12) & 0b1111)*1000 + ((*RYRCNT >> 8) & 0b1111)*100 + ((*RYRCNT >> 4) & 0b1111)*10 + (*RYRCNT & 0b1111);
  //curMonth = ((*RMONCNT >> 4) & 0b1)*10 + (*RMONCNT & 0b1111);
  int curDay = ((*RDAYCNT >> 4) & 0b11)*10 + (*RDAYCNT & 0b1111);

  char buffer[50];
  char buffer2[10];

  int textX = 0;
  switch(format)
  {
    case 0:
      strcpy(buffer, getDOW());
      strcat(buffer, ", ");
      strcat(buffer, getMonth());
      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, " ");
      strcat(buffer, buffer2);
      break;
    case 1:
      strcpy(buffer, getDOW());
      strcat(buffer, ", ");

      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getMonth());
      break;

    case 2:
      strcpy(buffer, getDOW());
      strcat(buffer, ", ");

      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getMonth());

      strcat(buffer, " ");
      itoa(curYear, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      break;

    case 3:
      strcpy(buffer, getDOW());
      strcat(buffer, ", ");

      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      //st,nd,rd,th code:
      if (curDay == 1 || curDay == 21 || curDay == 31) strcat(buffer, "st");
      else if (curDay == 2 || curDay == 22) strcat(buffer, "nd");
      else if (curDay == 3 || curDay == 23) strcat(buffer, "rd");
      else strcat(buffer, "th");

      strcat(buffer, " ");
      strcat(buffer, getMonth());
      break;

    case 4:
    case 6: //it's exactly the same except minimini is not used, but that's not handled here
      strcpy(buffer, getMonth());
      strcat(buffer, " ");
      itoa(curDay, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      break;

    case 5:
      strcpy(buffer, getMonth());
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
      strcat(buffer, getMonth());
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
      strcat(buffer, getMonth());

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

void fillTime(char *buffer, bool ampm)
{
    unsigned char s1 = '0' + (*RSECCNT & 0b1111);
    unsigned char s2 = '0' + ((*RSECCNT >> 4) & 0b111);
    unsigned char m1 = '0' + (*RMINCNT & 0b1111);
    unsigned char m2 = '0' + ((*RMINCNT >> 4) & 0b111);
    unsigned char h1 = '0' + (*RHRCNT & 0b1111);
    unsigned char h2 = '0' + ((*RHRCNT >> 4) & 0b11);
    if (ampm)
    {
        int pm = 0;
        int hour = (h2 - '0') * 10 + (h1 - '0');
        if (hour >= 12)
        {
            pm = 1;
            h2 -= 1;
            h1 -= 2;
            if (h1 < '0')
            {
                h1 += ('9' - '0') + 1;
                h2--;
            }
        }
        hour = (h2 - '0') * 10 + (h1 - '0');
        if (hour == 0)
        {
            h2 = '1';
            h1 = '2';
        }
        if (h2 == '0')
            h2 = ' ';

        buffer[8] = ' ';
        buffer[10] = 'M';
        buffer[11] = '\0';
        if (pm)
            buffer[9] = 'P';
        else
            buffer[9] = 'A';

    }
    else
    {
        buffer[8] = '\0';
    }
    buffer[0] = h2;
    buffer[1] = h1;
    buffer[2] = ':';
    buffer[3] = m2;
    buffer[4] = m1;
    buffer[5] = ':';
    buffer[6] = s2;
    buffer[7] = s1;
}

// END OF RTC, CLOCK and CALENDAR code

// START OF POWER MANAGEMENT CODE
#define LCDC *(unsigned int*)(0xB4000000)
int GetBacklightSubLevel_RAW()
{
  Bdisp_DDRegisterSelect(0x5a1);
  return (LCDC & 0xFF) - 6;
}
void SetBacklightSubLevel_RAW(int level)
{
  Bdisp_DDRegisterSelect(0x5a1);
  LCDC = (level & 0xFF) + 6;
}
// END OF POWER MANAGEMENT CODE

// START OF GRAPHICS CODE
/*
 * The below function was authored by Merth
 */

const short empty[18] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int PrintMiniFix( int x, int y, const char*Msg, const int flags, const short color, const short bcolor )
{
  int i = 0, dx;
  unsigned short width;
  void*p;

  while ( Msg[ i ] )
  {
    p = GetMiniGlyphPtr( Msg[ i ], &width );
    dx = ( 12 - width ) / 2;
    if ( dx > 0 )
    {
      PrintMiniGlyph( x, y, (void*)empty, flags, dx, 0, 0, 0, 0, color, bcolor, 0 );
    }
   
    else dx = 0;
    PrintMiniGlyph( x + dx, y, p, flags, width, 0, 0, 0, 0, color, bcolor, 0 );
    if ( width + dx < 12 )
    {
      PrintMiniGlyph( x + width + dx, y, (void*)empty, flags, 12 - width - dx, 0, 0, 0, 0, color, bcolor, 0 );
    }
    x += 12;
    i++;
  }
  return x;
}
//draws a point of color color at (x0, y0) 
void plot(int x0, int y0, int color) { 
   char* VRAM = (char*)0xA8000000; 
   VRAM += 2*(y0*LCD_WIDTH_PX + x0); 
   *(VRAM++) = (color&0x0000FF00)>>8; 
   *(VRAM++) = (color&0x000000FF); 
   return; 
}

void drawRectangle(int x, int y, int width, int height, unsigned short color) {
  unsigned short*VRAM = (unsigned short*)0xA8000000;
  for(int j = y; j < y+height; j++) {
    for(int i = x; i < x+width; i++) {
      *(j*LCD_WIDTH_PX+i+VRAM) = color;      
    }
  }
}
//Uses the Bresenham line algorithm 
void drawLine(int x1, int y1, int x2, int y2, int color) { 
    signed char ix; 
    signed char iy; 
  
    // if x1 == x2 or y1 == y2, then it does not matter what we set here 
    int delta_x = (x2 > x1?(ix = 1, x2 - x1):(ix = -1, x1 - x2)) << 1; 
    int delta_y = (y2 > y1?(iy = 1, y2 - y1):(iy = -1, y1 - y2)) << 1; 
  
   plot(x1, y1, color);  
    if (delta_x >= delta_y) { 
        int error = delta_y - (delta_x >> 1);        // error may go below zero 
        while (x1 != x2) { 
            if (error >= 0) { 
                if (error || (ix > 0)) { 
                    y1 += iy; 
                    error -= delta_x; 
                }                           // else do nothing 
         }                              // else do nothing 
            x1 += ix; 
            error += delta_y; 
            plot(x1, y1, color); 
        } 
    } else { 
        int error = delta_x - (delta_y >> 1);      // error may go below zero 
        while (y1 != y2) { 
            if (error >= 0) { 
                if (error || (iy > 0)) { 
                    x1 += ix; 
                    error -= delta_y; 
                }                           // else do nothing 
            }                              // else do nothing 
            y1 += iy; 
            error += delta_x;  
            plot(x1, y1, color); 
        } 
    } 
}
//ReplaceColor By Kerm:
void VRAMReplaceColorInRect(int x, int y, int width, int height, color_t color_old, color_t color_new) { 
   //color_t* VRAM = GetVRAMAddress();
   color_t* VRAM = (color_t*)0xA8000000; 
   VRAM += (y*LCD_WIDTH_PX)+x; 
   for(int j=0; j<height; VRAM += (LCD_WIDTH_PX-width), j++) { 
      for(int i=0; i<width; VRAM++, i++) { 
         if (*VRAM == color_old) *VRAM = color_new; 
      } 
   } 
}
void darkenStatusbar() {  
  VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_BLACK, COLOR_CYAN);
  VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_WHITE, COLOR_BLACK);
  VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_CYAN, COLOR_GRAY);
  VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_BLUE, COLOR_ORANGE);
}
void darkenFkeys(int numkeys) {
  VRAMReplaceColorInRect(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_BLACK, COLOR_CYAN);
  VRAMReplaceColorInRect(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_WHITE, COLOR_BLACK);
  VRAMReplaceColorInRect(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_CYAN, COLOR_GRAY);  
}
void drawArrowDown(int bottomX, int bottomY, int color) {
  drawLine(bottomX-7,bottomY-7,bottomX,bottomY,color);
  drawLine(bottomX-8,bottomY-7,bottomX,bottomY+1,color); //double thickness of line
  drawLine(bottomX+1,bottomY,bottomX+8,bottomY-7,color);
  drawLine(bottomX+1,bottomY+1,bottomX+9,bottomY-7,color); //double thickness of line
}
void drawFkeyPopup(int Fkey, int darktheme=0, int showclosemessage=1) {
//draws a big popup pointing to a Fkey (index zero based, F1 = 0, F6 = 5) with the selected color scheme.
//showclosemessage - select to show a minimini message on the right left saying "...or press: [EXIT]"
// PrintXY text inside the popup starts at X=2 and Y=2
  int fgcolor = COLOR_BLACK;
  int bgcolor = COLOR_WHITE;
  if (darktheme) {
    fgcolor = COLOR_WHITE;
    bgcolor = COLOR_BLACK;
  }
  int c1x = 18; int c1y = 24*2;
  int c2x = 18*20; int c2y = 24*2;
  int c3x = 18*20; int c3y = 24*8-12;
  int c4x = 18; int c4y = 24*8-12;
  drawRectangle(c1x, c1y, c3x-c1x, c3y-c1x-24, bgcolor);
  drawLine(c1x-1, c1y-1, c2x, c2y-1, fgcolor);
  drawLine(c1x-1, c1y-1, c4x-1, c4y+1, fgcolor);
  drawLine(c2x, c2y-1, c3x, c3y+1, fgcolor);
  drawLine(c4x-1, c4y+1, c3x, c3y+1, fgcolor);

  drawLine(c1x-2, c1y-2, c2x+1, c2y-2, fgcolor);
  drawLine(c1x-2, c1y-2, c4x-2, c4y+2, fgcolor);
  drawLine(c2x+1, c2y-2, c3x+1, c3y+2, fgcolor);
  drawLine(c4x-2, c4y+2, c3x+1, c3y+2, fgcolor);

  drawLine(c1x, c1y+23, c2x-1, c2y+23, COLOR_GRAY);
  drawRectangle(c1x, c1y, c2x-c1x, 23, COLOR_LIGHTGRAY);

  drawArrowDown(31+Fkey*64, 190, fgcolor);
  drawLine(23+Fkey*64, 181, 40+Fkey*64, 181, bgcolor);
  drawLine(24+Fkey*64, 182, 39+Fkey*64, 182, bgcolor);


  if (showclosemessage) {
    int textX = 0;
    int textY = c3y-14-20;
    PrintMiniMini( &textX, &textY, (unsigned char*)"...or press: [EXIT]", 0, TEXT_COLOR_BLACK, 1 ); //fake draw
    textX = c3x-textX-4;
    PrintMiniMini( &textX, &textY, (unsigned char*)"...or press: [EXIT]", 0, TEXT_COLOR_BLACK, 0 ); //draw
  }
}
void CopySprite(const void* datar, int x, int y, int width, int height) { 
   color_t*data = (color_t*) datar; 
   color_t* VRAM = (color_t*)0xA8000000; 
   VRAM += LCD_WIDTH_PX*y + x; 
   for(int j=y; j<y+height; j++) { 
      for(int i=x; i<x+width; i++) { 
         *(VRAM++) = *(data++); 
     } 
     VRAM += LCD_WIDTH_PX-width; 
   } 
} 
void CopySpriteMasked(const unsigned char* data, int x, int y, int width, int height, int maskcolor) { 
   char* VRAM = (char*)0xA8000000; 
   VRAM += 2*(LCD_WIDTH_PX*y + x); 
   for(int j=y; j<y+height; j++) { 
      for(int i=x; i<x+width;  i++) { 
         if ((((((int)(*data))&0x000000FF)<<8) | ((((int)(*(data+1))))&0x000000FF)) != maskcolor) { 
            *(VRAM++) = *(data++); 
            *(VRAM++) = *(data++); 
         } else { VRAM += 2; data += 2; } 
      } 
      VRAM += 2*(LCD_WIDTH_PX-width); 
   } 
}
int drawRGB24toRGB565(int r, int g, int b)  
{  
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);  
}
//the following does not update the screen automatically; it will draw the tny.im logo starting at screen coordinates x,y
//the tny.im logo is great enough not to require any sprites! yay!
//w:138
//h:42
int alphaBlend(int newcc, int oldcc, float alpha) {
  return alpha*newcc+(1-alpha)*oldcc;
}
/* commented because isn't needed
void alphaRGB(int r1, int g1, int b1, int r2, int g2, int b2, int* nr, int* ng, int* nb, float alpha) {
  *nr = alphaBlend(r1, r2, alpha);
  *ng = alphaBlend(g1, g2, alpha);
  *nb = alphaBlend(b1, b2, alpha);
}*/
void drawtnyimLogo( int x, int y, float alpha=0) {
  //draw t
  int black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
  drawRectangle(x, y+6, 6, 24, black);
  drawRectangle(x+6, y+12, 6, 6, black);
  drawRectangle(x+6, y+30, 6, 6, black);
  //draw n
  drawRectangle(x+18, y+12, 6, 24, black);
  drawRectangle(x+24, y+12, 12, 6, black);
  drawRectangle(x+36, y+18, 6, 18, black);
  //draw y
  drawRectangle(x+48, y+12, 6, 18, black);
  drawRectangle(x+60, y+12, 6, 18, black);
  drawRectangle(x+54, y+30, 6, 6, black);
  drawRectangle(x+48, y+36, 6, 6, black);
  //draw dot
  drawRectangle(x+72, y+30, 6, 6, black);
  int orange = drawRGB24toRGB565(alphaBlend(255, 210, alpha), alphaBlend(255, 68, alpha), alphaBlend(255, 19, alpha));
  //draw i (orange)
  drawRectangle(x+84, y, 6, 6, orange);
  drawRectangle(x+84, y+12, 6, 24, orange);
  //draw m (orange)
  drawRectangle(x+96, y+12, 6, 24, orange);
  drawRectangle(x+102, y+12, 12, 6, orange);
  drawRectangle(x+114, y+18, 6, 18, orange);
  drawRectangle(x+120, y+12, 12, 6, orange);
  drawRectangle(x+132, y+18, 6, 18, orange);
}
// END OF GRAPHICS CODE

// CPU CLOCKING CODE

#define PLL_28x 0b011011 // 101.5 MHz
#define PLL_26x 0b011001 // 94.3 MHz 
#define PLL_24x 0b010111 // 87 MHz 
#define PLL_20x 0b010011 // 72.5 MHz 
#define PLL_18x 0b010001 // 65.25 MHz 
#define PLL_16x 0b001111 // 58 MHz (NORMAL SPEED) 
#define PLL_15x 0b001110 // 54.37 MHz 
#define PLL_12x 0b001011 // 43.5 MHz 
#define PLL_8x  0b000111 // 29 MHz 
#define PLL_6x  0b000101 // 21.75 MHz 
#define PLL_4x  0b000011 // 14.5 MHz 
#define PLL_3x  0b000010 // 10.8 MHz 
#define PLL_2x  0b000001 // 7.25 MHz 
#define PLL_1x  0b000000 // 3.6 MHz

const int PLLs[] = {PLL_1x, PLL_2x, PLL_3x, PLL_4x, PLL_6x, PLL_8x, PLL_12x, PLL_15x, PLL_16x, PLL_18x, PLL_20x, PLL_24x, PLL_26x, PLL_28x};

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
void settingsMenu();
// KEYBOARD HANDLING
void mGetKey(int* key) {
  //managed GetKey. allows for entering the settings menu from most points in the add-in.
  while (1) {
    GetKey(key);
    if (*key == KEY_CTRL_SETUP) {
      SaveVRAM_1();
      settingsMenu();
      LoadVRAM_1();
      break;
    } else if (*key == KEY_SHIFT_OPTN && setting_debug_on) {
      SaveVRAM_1();
    	TestMode( 1 );
    	const unsigned int default_fkeys[] = { 0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0 };
    	Set_FKeys1( 0, (unsigned int*)default_fkeys );
    	Set_FKeys2( 0 );
      LoadVRAM_1();
    } else {
      break; 
    }
  }
}
// END OF KEYBOARD HANDLING
void setCPUclock() {
  int key; int textX; int textY;

  volatile unsigned int*FRQCR = (unsigned int*) 0xA4150000;
  while (1) {
    Bdisp_AllClr_VRAM();
    Bdisp_EnableColor(1); 
    if (setting_display_statusbar == 1) DisplayStatusArea();

    PrintXY(1, 1, (char*)"  CPU speed", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    CopySprite(selector, 10, 85, 364, 43);
    
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
            change_freq(PLLs[i+1]);
            break;
          }
        }
         break;
      case KEY_CTRL_LEFT:
      case KEY_CTRL_DOWN:
        for(int i = 1; i < 14; i++) {
          if(((*FRQCR & 0x3F000000) >> 24) == PLLs[i]) {
            change_freq(PLLs[i-1]);
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
// END OF CPU CLOCKING CODE

//////////////////////////////////////////
// ABOUT SCREEN
//////////////////////////////////////////

void showAbout() {
 int key;
 Bdisp_AllClr_VRAM();
 Bdisp_EnableColor(1); 
 DefineStatusMessage((char*)"About Utilities", 1, 0, 0);
 DisplayStatusArea();
 //y increment between each line: 17; between paragraphs: 20
 int orange = drawRGB24toRGB565(210, 68, 19);
 int textX = 0;
 int textY = 5;
 char verBuffer[100] = "";
 getVersion(verBuffer);
 PrintMini(&textX, &textY, (unsigned char*)"Version ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
 PrintMini(&textX, &textY, (unsigned char*)verBuffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
 textY = textY + 17; textX = 0;
 getTimestamp(verBuffer);
 PrintMini(&textX, &textY, (unsigned char*)verBuffer, 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);
 textY = 42;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"Developed by gbl08ma at", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 drawtnyimLogo(10, 59+24); //24 pixels for the status bar
 textY = 101;
 textX = 0;
 // PrintMini and its x,y pointers allow for awesome easy color formatting... let's try
 PrintMini(&textX, &textY, (unsigned char*)"tny. ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 PrintMini(&textX, &textY, (unsigned char*)"i", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
 PrintMini(&textX, &textY, (unsigned char*)"nternet ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 PrintMini(&textX, &textY, (unsigned char*)"m", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
 PrintMini(&textX, &textY, (unsigned char*)"edia", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 125;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"http://i.tny.im | http://gbl08ma.com", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 142;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"gbl08ma@gmail.com", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 
 textX = 0; textY = 175;
 PrintMini(&textX, &textY, (unsigned char*)"Press any key", 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
 GetKey(&key);

 Bdisp_AllClr_VRAM();
 Bdisp_EnableColor(1);
 DisplayStatusArea();
 textY = 5;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"Contains code by AHelper,", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 22;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"merthsoft and KermMartian at", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 39;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"Cemetech - http://cemetech.net", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 67;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"This Utilities add-in is licensed", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 84;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"under the GNU GPL v2, or at your", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 101;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"choice, any later version.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

 textX = 0; textY = 175;
 PrintMini(&textX, &textY, (unsigned char*)"Press any key", 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
 GetKey(&key);

 Bdisp_AllClr_VRAM();
 Bdisp_EnableColor(1);
 DisplayStatusArea();
 textY = 5;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"USE AT YOUR OWN RISK!", 0, 0xFFFFFFFF, 0, 0, COLOR_RED, COLOR_WHITE, 1, 0);
 textY = 22;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"PROVIDED WITHOUT WARRANTY!", 0, 0xFFFFFFFF, 0, 0, COLOR_RED, COLOR_WHITE, 1, 0);
 textY = 52;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"In no event will the authors be held", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 69;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"liable for any damages arising from", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 86;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"the use of this software.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

 textX = 0; textY = 175;
 PrintMini(&textX, &textY, (unsigned char*)"Press any key", 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
 GetKey(&key);
 DefineStatusMessage((char*)"", 1, 0, 0);
}


//////////////////////////////////////////
// SETTING SETTINGS
//////////////////////////////////////////
void setDate() {
  int key, inscreen = 1;
  unsigned int selyear = ((*RYRCNT >> 12) & 0b1111)*1000 + ((*RYRCNT >> 8) & 0b1111)*100 + ((*RYRCNT >> 4) & 0b1111)*10 + (*RYRCNT & 0b1111);
  unsigned int selmonth = ((*RMONCNT & 0b10000)>>4)*10 + (*RMONCNT & 0b1111);
  unsigned int selday = ((*RDAYCNT >> 4) & 0b11)*10 + (*RDAYCNT & 0b1111);
  char buffer1[50] = "";
  char buffer2[50] = "";
  // YEAR SELECTION SCREEN:
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set date", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Year", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(selyear, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //year
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selyear != 1970) { //don't allow to set below 1970 so it is Unix-time compatible and always has 4 digits
          selyear--;
        }
        break;
      case KEY_CTRL_UP:
        if (selyear != 9999) {
          selyear++;
        }
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }

  // MONTH SELECTION SCREEN:
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set date", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Month", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    strcat(buffer1, monthNames[selmonth-1]);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //month name
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selmonth == 1) {
          selmonth = 12;
        } else {
          selmonth--;
        }
        break;
      case KEY_CTRL_UP:
        if (selmonth == 12) {
          selmonth = 1;
        } else {
          selmonth++;
        }
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }
  
  // DAY SELECTION SCREEN:
  char temp = *RCR2; //for rtc stopping/starting
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set date", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Day", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(selday, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //day
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selday == 1) {
          selday = monthDays[selmonth-1];
        } else {
          selday--;
        }
        break;
      case KEY_CTRL_UP:
        if (selday == (unsigned)monthDays[selmonth-1]) { //it is zero-based...
          selday = 1;
        } else {
          selday++;
        }
        break;
      case KEY_CTRL_EXE:
        //All set, set date
        //convert to char for easier setting
        char day[2];
        char month[2];
        char year[4];
        itoa(selyear, (unsigned char*) year); //we already know this has four digits

        itoa(selmonth, (unsigned char*) buffer2);
        if (selmonth < 10)
        { strcpy(month, "0"); strcat(month, buffer2); }
        else { strcpy(month, buffer2);}

        itoa(selday, (unsigned char*) buffer2);
        if (selday < 10)
        { strcpy(day, "0"); strcat(day, buffer2); }
        else { strcpy(day, buffer2);}

        //stop RTC
        temp |= 0b10;
        temp &= 0b11111110;
        *RCR2 = temp;

        // set year
        *RYRCNT  = ((year[0] - '0') << 12) | ((year[1] - '0') << 8) | ((year[2] - '0') << 4) | (year[3] - '0');
        // set month
        *RMONCNT = ((month[0] - '0') << 4) | (month[1] - '0');
        // set day
        *RDAYCNT = ((day[0] - '0') << 4) | (day[1] - '0');
        // set day of week
        *RWKCNT = dow(selyear, selmonth, selday) & 0b111;
        // start RTC
        *RCR2 |= 1;
        return;
      case KEY_CTRL_EXIT: return;
    }
  }
}

void setTime() {
  int key, inscreen = 1;
  unsigned int selhour = bcd_to_2digit(RHRCNT);
  unsigned int selmin = bcd_to_2digit(RMINCNT);
  unsigned int selsec = bcd_to_2digit(RSECCNT);

  char buffer1[50] = "";
  char buffer2[50] = "";
  // HOUR SELECTION SCREEN:
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set time", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Hour", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(selhour, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //hour
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selhour == 0) {
          selhour = 23;
        } else {
          selhour--;
        }
        break;
      case KEY_CTRL_UP:
        if (selhour == 23) {
          selhour = 0;
        } else {
          selhour++;
        }
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }

  // MINUTE SELECTION SCREEN:
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set time", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Minute", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(selmin, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //minute
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selmin == 0) {
          selmin = 59;
        } else {
          selmin--;
        }
        break;
      case KEY_CTRL_UP:
        if (selmin == 59) {
          selmin = 0;
        } else {
          selmin++;
        }
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }
  
  // SECOND SELECTION SCREEN:
  char temp = *RCR2; //for rtc stopping/starting
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set time", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Second", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(selsec, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //day
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selsec == 0) {
          selsec = 59;
        } else {
          selsec--;
        }
        break;
      case KEY_CTRL_UP:
        if (selsec == 59) {
          selsec = 0;
        } else {
          selsec++;
        }
        break;
      case KEY_CTRL_EXE:
        //All set, set date
        //convert to char for easier setting
        char hour[2];
        char min[2];
        char sec[2];

        itoa(selhour, (unsigned char*) buffer2);
        if (selhour < 10)
        { strcpy(hour, "0"); strcat(hour, buffer2); }
        else { strcpy(hour, buffer2);}

        itoa(selmin, (unsigned char*) buffer2);
        if (selmin < 10)
        { strcpy(min, "0"); strcat(min, buffer2); }
        else { strcpy(min, buffer2);}

        itoa(selsec, (unsigned char*) buffer2);
        if (selsec < 10)
        { strcpy(sec, "0"); strcat(sec, buffer2); }
        else { strcpy(sec, buffer2);}

        //stop RTC
        temp |= 0b10;
        temp &= 0b11111110;
        *RCR2 = temp;

        // hour
        *RHRCNT  = ((hour[0] - '0') << 4) | (hour[1] - '0');
        // minute
        *RMINCNT = ((min[0] - '0') << 4) | (min[1] - '0');
        // second
        *RSECCNT = ((sec[0] - '0') << 4) | (sec[1] - '0');


        // start RTC
        *RCR2 |= 1;
        return;
      case KEY_CTRL_EXIT: return;
    }
  }
}

void setTimeFormat() {
  int key, inscreen = 1;
  unsigned int selformat = setting_timeformat;
  
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set time format", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  //PrintXY(3, 2, (char*)"  Second", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    // draw time sample accordingly
    char timeStr[14] = "  "; //two spaces for printxy...
    if (selformat == 0) { //24 hour
      fillTime(timeStr + 2,0);
      PrintXY(5,5,timeStr, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else { //12 hour
      fillTime(timeStr + 2,1);
      PrintXY(5,5,timeStr, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
      case KEY_CTRL_UP:
        if (selformat == 1) selformat = 0; else selformat = 1;
        break;
      case KEY_CTRL_EXE:
        setting_timeformat = selformat;
        SaveSettings(SETTINGSFILE);
        return;
      case KEY_CTRL_EXIT: return;
    }
  }
}

void setLongDateFormat() {
  int key, inscreen = 1;
  unsigned int selformat = setting_longdateformat;
  
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set long date format", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  //PrintXY(3, 2, (char*)"  Second", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 3, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(1, 4, (char*)"                           ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    PrintXY(1, 5, (char*)"                           ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    // draw date sample accordingly
    drawLongDate(82,selformat);
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selformat == 0) {
          selformat = 9;
        } else {
          selformat--;
        }
        break;
      case KEY_CTRL_UP:
        if (selformat == 9) {
          selformat = 0;
        } else {
          selformat++;
        }
        break;
      case KEY_CTRL_EXE:
        setting_longdateformat = selformat;
        SaveSettings(SETTINGSFILE);
        return;
      case KEY_CTRL_EXIT: return;
    }
  }
}

void setDateFormat() {
  int key, inscreen = 1;
  unsigned int selformat = setting_dateformat;
  char dateStr[13] = "  ";
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set date format", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  //PrintXY(3, 2, (char*)"  Second", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    // draw date sample accordingly
    fillDate(dateStr + 2,selformat);
    PrintXY(5,5,dateStr, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);

    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selformat == 0) {
          selformat = 2;
        } else {
          selformat--;
        }
        break;
      case KEY_CTRL_UP:
        if (selformat == 2) {
          selformat = 0;
        } else {
          selformat++;
        }
        break;
      case KEY_CTRL_EXE:
        setting_dateformat = selformat;
        SaveSettings(SETTINGSFILE);
        return;
      case KEY_CTRL_EXIT: return;
    }
  }
}

void setStartupBrightness() {
  int key, inscreen = 1, textX=0, textY=130;
  unsigned int selbrightness = setting_startup_brightness;
  char buffer1[50] = "";
  char buffer2[50] = "";

  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set start brightness", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(5, 3, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 5, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 4, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    if (selbrightness == 250) {
      PrintXY(5, 4, (char*)"  Do not force", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else {
      strcpy(buffer1, "  ");
      itoa(selbrightness, (unsigned char*)buffer2);
      strcat(buffer1, buffer2);
      PrintXY(5, 4, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }

    textX=0; textY=130;
    PrintMiniMini( &textX, &textY, (unsigned char*)"Set the screen backlight level to be set when the add-in", 0, TEXT_COLOR_BLACK, 0 );
    textY=textY+12; textX=0;
    PrintMiniMini( &textX, &textY, (unsigned char*)"starts. Select 'Do not force' to use the system brightness.", 0, TEXT_COLOR_BLACK, 0 );
    textY=textY+12; textX=0;
    PrintMiniMini( &textX, &textY, (unsigned char*)"Valid values vary between 0 (minimum) and 244 (brightest).", 0, TEXT_COLOR_BLACK, 0 );
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selbrightness != 0) {
          selbrightness--;
        }
        break;
      case KEY_CTRL_UP:
        if (selbrightness != 250) {
          selbrightness++;
        }
        break;
      case KEY_CTRL_EXE:
        setting_startup_brightness = selbrightness;
        SaveSettings(SETTINGSFILE);
        return;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }
}
void drawLockSettingsMenu(int pos, int scroll, int numitems)
{
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  if(scroll < 1) PrintXY(1,1,(char*)"  Set lock code        ", (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 2) PrintXY(1,2-scroll,(setting_password_show_last_char == 1 ? (char*)"  Show last code char \xe6\xa9" : (char*)"  Show last code char \xe6\xa5"), (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 3) PrintXY(1,3-scroll,(setting_lock_auto_turnoff == 1 ?       (char*)"  Off after locking   \xe6\xa9" : (char*)"  Off after locking   \xe6\xa5"), (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 4) PrintXY(1,4-scroll,(setting_lock_on_exe == 1 ?             (char*)"  Lock on [EXE]       \xe6\xa9" : (char*)"  Lock on [EXE]       \xe6\xa5"), (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 5) switch(setting_unlock_runmat) {
    default:
      PrintXY(1,5-scroll,(char*)"  Run-Mat on unlock   \xe6\xa5", (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
      break;
    case 1:
      PrintXY(1,5-scroll,(char*)"  Run-Mat on unlock   \xe6\xa9", (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
      break;  
    case 2:
      PrintXY(1,5-scroll,(char*)"  Run-Mat on unlock     ", (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
      int textX=LCD_WIDTH_PX-7-21; int textY=(5-scroll)*24-24+7;
      PrintMiniMini( &textX, &textY, (unsigned char*)"Ask", (pos == 5 ? 4 : 0), TEXT_COLOR_BLACK, 0 );
      break; 
  }
  
  //hide 8th item
  PrintXY(1,8,(char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  TScrollbar sb;
  sb.I1 = 0;
  sb.I5 = 0;
  sb.indicatormaximum = numitems;
  sb.indicatorheight = 7;
  sb.indicatorpos = scroll;
  sb.barheight = LCD_HEIGHT_PX - 24 - 24;
  sb.bartop = 0;
  sb.barleft = LCD_WIDTH_PX - 6;
  sb.barwidth = 6;
  Scrollbar(&sb);

}
void lockSettingsMenu()
{
  int key, pos = 1, scroll = 0, inscreen = 1;
  int numitems = 5; //total number of items in menu
  while(inscreen)
  {
    drawLockSettingsMenu(pos, scroll, numitems);
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(pos == numitems)
        {
          pos = 1;
          scroll = 0;
        }
        else
        {
          pos++;
          if(pos > scroll+5)
            scroll = pos -5;
        }
        break;
      case KEY_CTRL_UP:
        if(pos == 1)
        {
          pos = numitems;
          scroll = pos-5;
        }
        else
        {
          pos--;
          if(pos-1 < scroll)
            scroll = pos -1;
        }
        break;
      case KEY_CTRL_EXE:
        switch(pos)
        {
          case 1:
            setPassword(setting_display_statusbar, setting_password_show_last_char);
            break;
          case 2:
            if (setting_password_show_last_char == 1) setting_password_show_last_char = 0; else setting_password_show_last_char = 1;
            SaveSettings(SETTINGSFILE);
            break;
          case 3:
            if (setting_lock_auto_turnoff == 1) setting_lock_auto_turnoff = 0; else setting_lock_auto_turnoff = 1;
            SaveSettings(SETTINGSFILE);
            break;
          case 4:
            if (setting_lock_on_exe == 1) setting_lock_on_exe = 0; else setting_lock_on_exe = 1;
            SaveSettings(SETTINGSFILE);
            break;
          case 5:
            if(setting_unlock_runmat == 0) {
              setting_unlock_runmat = 1;
            } else if (setting_unlock_runmat == 1) {
              setting_unlock_runmat = 2;
            } else {
              setting_unlock_runmat = 0;
            }
            SaveSettings(SETTINGSFILE);
            break;
          default: break;
        }
        break;
      case KEY_CTRL_EXIT: inscreen = 0;
    }
  }
}
void drawSettingsMenu(int pos, int scroll, int numitems)
{  
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  if(scroll < 1) PrintXY(1,1,(char*)"  Set time             ", (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 2) PrintXY(1,2-scroll,(char*)"  Set date             ", (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 3) PrintXY(1,3-scroll,(char*)"  Time format          ", (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 4) PrintXY(1,4-scroll,(char*)"  Long date format     ", (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 5) PrintXY(1,5-scroll,(char*)"  Date format          ", (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 6) PrintXY(1,6-scroll,(setting_black_theme == 1 ? (char*)"  Dark theme          \xe6\xa9" : (char*)"  Dark theme          \xe6\xa5"), (pos == 6 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 7) PrintXY(1,7-scroll,(setting_display_statusbar == 1 ? (char*)"  Display statusbar   \xe6\xa9" : (char*)"  Display statusbar   \xe6\xa5"), (pos == 7 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 8) PrintXY(1,8-scroll,(setting_show_advanced == 1 ? (char*)"  Show advanced tools \xe6\xa9" : (char*)"  Show advanced tools \xe6\xa5"), (pos == 8 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 9) PrintXY(1,9-scroll,(setting_display_fkeys == 1 ? (char*)"  Show F. keys labels \xe6\xa9" : (char*)"  Show F. keys labels \xe6\xa5"), (pos == 9 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 10) PrintXY(1,10-scroll,(char*)"  Startup brightness   ", (pos == 10 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 11) PrintXY(1,11-scroll,(char*)"  Calc. lock settings  ", (pos == 11 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 12) PrintXY(1,12-scroll,(setting_show_events_count_on_calendar == 1 ? (char*)"  Show events count   \xe6\xa9" : (char*)"  Show events count   \xe6\xa5"), (pos == 12 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 13) PrintXY(1,13-scroll,(char*)"  About this add-in    ", (pos == 13 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  
  //hide 8th item
  PrintXY(1,8,(char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  TScrollbar sb;
  sb.I1 = 0;
  sb.I5 = 0;
  sb.indicatormaximum = numitems;
  sb.indicatorheight = 7;
  sb.indicatorpos = scroll;
  sb.barheight = LCD_HEIGHT_PX - 24 - 24;
  sb.bartop = 0;
  sb.barleft = LCD_WIDTH_PX - 6;
  sb.barwidth = 6;
  Scrollbar(&sb);

}
void settingsMenu()
{
  int key, pos = 1, scroll = 0, inscreen = 1;
  int numitems = 13; //total number of items in menu
  while(inscreen)
  {
    drawSettingsMenu(pos, scroll, numitems);
    GetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(pos == numitems)
        {
          pos = 1;
          scroll = 0;
        }
        else
        {
          pos++;
          if(pos > scroll+7)
            scroll = pos -7;
        }
        break;
      case KEY_CTRL_UP:
        if(pos == 1)
        {
          pos = numitems;
          scroll = pos-7;
        }
        else
        {
          pos--;
          if(pos-1 < scroll)
            scroll = pos -1;
        }
        break;
      case KEY_CTRL_EXE:
        switch(pos)
        {
          case 1:
            setTime();
            break;
          case 2:
            setDate();
            break;
          case 3:
            setTimeFormat();
            break;
          case 4:
            setLongDateFormat();
            break;
          case 5:
            setDateFormat();
            break;
          case 6:
            if (setting_black_theme == 1) setting_black_theme = 0; else setting_black_theme = 1;
            SaveSettings(SETTINGSFILE);
            break;
          case 7:
            if (setting_display_statusbar == 1) setting_display_statusbar = 0; else setting_display_statusbar = 1;
            SaveSettings(SETTINGSFILE);
            break;
          case 8:
            if (setting_show_advanced == 1) setting_show_advanced = 0; else setting_show_advanced = 1;
            SaveSettings(SETTINGSFILE);
            break;
          case 9:
            if (setting_display_fkeys == 1) setting_display_fkeys = 0; else setting_display_fkeys = 1;
            SaveSettings(SETTINGSFILE);
            break;
          case 10:
            setStartupBrightness();
            break;
          case 11:
            lockSettingsMenu();
            break;
          case 12:
            if (setting_show_events_count_on_calendar == 1) setting_show_events_count_on_calendar = 0; else setting_show_events_count_on_calendar = 1;
            SaveSettings(SETTINGSFILE);
            break;     
          case 13:
            showAbout();
            break;
          default: break;
        }
        break;
      case KEY_CTRL_EXIT: inscreen = 0;
    }
  }
}

//////////////////////////////////////////
// POWER OPTIONS
//////////////////////////////////////////
void changePowerTimeout() {
  int key, inscreen = 1;
  unsigned int seltimeout = GetAutoPowerOffTime();
  char buffer1[50] = "";
  char buffer2[50] = "";

  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Poweroff Timeout", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(5, 3, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 5, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 4, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    itoa(seltimeout, (unsigned char*)buffer2);
    strcpy(buffer1, "  ");
    strcat(buffer1, buffer2);
    strcat(buffer1, " Minutes");
    PrintXY(5, 4, (char*)buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);

    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (seltimeout != 1) {
          seltimeout--;
        }
        break;
      case KEY_CTRL_UP:
        if (seltimeout != 999) {
          seltimeout++;
        }
        break;
      case KEY_CTRL_EXE:
        SetAutoPowerOffTime(seltimeout);
        return;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }
}

void changeBacklightTimeout() {
  int key, inscreen = 1;
  unsigned int seltimeout = GetBacklightDuration();
  char buffer1[50] = "";
  char buffer2[50] = "";

  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Backlight Duration", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(5, 3, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 5, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 4, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    if(seltimeout % 2 == 0) { //even, so timeout is X min 0 sec.
      itoa(seltimeout/2, (unsigned char*)buffer2);
      strcpy(buffer1, "  ");
      strcat(buffer1, buffer2);
      strcat(buffer1, " Minutes ");
      PrintXY(5, 4, (char*)buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else { // timeout is X min 30 sec.
      itoa((seltimeout-1)/2, (unsigned char*)buffer2);
      strcpy(buffer1, "  ");
      strcat(buffer1, buffer2);
      strcat(buffer1, " Minutes 30 Sec.");
      PrintXY(5, 4, (char*)buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);

    }
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (seltimeout != 1) {
          seltimeout--;
        }
        break;
      case KEY_CTRL_UP:
        if (seltimeout != 40) {
          seltimeout++;
        }
        break;
      case KEY_CTRL_EXE:
        SetBacklightDuration(seltimeout);
        return;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }
}
void changeBacklightLevel() {
  int key, inscreen = 1, textX=0, textY=130;
  unsigned int selbrightness = GetBacklightSubLevel_RAW();
  unsigned int initlevel = selbrightness; //so we can revert from live preview if user presses exit
  char buffer1[50] = "";
  char buffer2[50] = "";

  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Backlight Level", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(5, 3, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 5, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 4, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(selbrightness, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 4, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);

    SetBacklightSubLevel_RAW(selbrightness); //live preview!

    textX=0; textY=130;
    PrintMiniMini( &textX, &textY, (unsigned char*)"This setting is volatile because it is changed by the OS on", 0, TEXT_COLOR_BLACK, 0 );
    textY=textY+12; textX=0;
    PrintMiniMini( &textX, &textY, (unsigned char*)"poweroff, on backlight timeout and when the user changes", 0, TEXT_COLOR_BLACK, 0 );
    textY=textY+12; textX=0;
    PrintMiniMini( &textX, &textY, (unsigned char*)"the setting in the OS's System menu.", 0, TEXT_COLOR_BLACK, 0 );
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (selbrightness != 0) {
          selbrightness--;
        }
        break;
      case KEY_CTRL_UP:
        if (selbrightness != 249) {
          selbrightness++;
        }
        break;
      case KEY_CTRL_EXE:
        SetBacklightSubLevel_RAW(selbrightness);
        return;
        break;
      case KEY_CTRL_EXIT:
        SetBacklightSubLevel_RAW(initlevel); //recover from live preview
        return;
        break;
    }
  }
}

#define P10CR 0xA4050140
#define P11CR (P10CR+2)
#define P11DR (P11CR+0x20)
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
  if (setting_display_statusbar == 1) DisplayStatusArea();
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
void drawPowerMenu(int pos, int scroll, int numitems)
{  
  drawFkeyPopup(0, setting_black_theme);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  if(setting_black_theme) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Power options", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);

  if (numitems > 4) { //show advanced
    if(scroll < 1) PrintXY(2,3,(char*)"  Auto Power Off     ", (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
    if(scroll < 2) PrintXY(2,4-scroll,(char*)"  Backlight Duration ", (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
    if(scroll < 3) PrintXY(2,5-scroll,(char*)"  Backlight Level    ", (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
    if(scroll < 4) PrintXY(2,6-scroll,(char*)"  CPU speed          ", (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
    if(scroll < 5 && scroll > 0) PrintXY(2,7-scroll,(char*)"  Power Information  ", (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);

    TScrollbar sb;
    sb.I1 = 0;
    sb.I5 = 0;
    sb.indicatormaximum = numitems;
    sb.indicatorheight = 4;
    sb.indicatorpos = scroll;
    sb.barheight = LCD_HEIGHT_PX - 24*5;
    sb.bartop = 24*2;
    sb.barleft = LCD_WIDTH_PX - 6 - 18 - 5;
    sb.barwidth = 6;
    Scrollbar(&sb);
  } else {
    if(scroll < 1) PrintXY(2,3,(char*)"  Auto Power Off     ", (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
    if(scroll < 2) PrintXY(2,4-scroll,(char*)"  Backlight Duration ", (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
    if(scroll < 3) PrintXY(2,5-scroll,(char*)"  Backlight Level    ", (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
    if(scroll < 4) PrintXY(2,6-scroll,(char*)"  Power Information  ", (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  }

}

void powerMenu() {
  int key, pos = 1, scroll = 0, inscreen = 1;
  int numitems = 4; //total number of items in menu

  while(inscreen)
  {
    if (setting_show_advanced) numitems = 5;
    drawPowerMenu(pos, scroll, numitems);
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(pos == numitems)
        {
          pos = 1;
          scroll = 0;
        }
        else
        {
          pos++;
          if(pos > scroll+4)
            scroll = pos -4;
        }
        break;
      case KEY_CTRL_UP:
        if(pos == 1)
        {
          pos = numitems;
          scroll = pos-4;
        }
        else
        {
          pos--;
          if(pos-1 < scroll)
            scroll = pos -1;
        }
        break;
      case KEY_CTRL_EXE:
        switch(pos)
        {
          case 1:
            changePowerTimeout();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 2:
            changeBacklightTimeout();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 3:
            changeBacklightLevel();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 4:
            if (setting_show_advanced) { setCPUclock(); } else powerInformation();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 5: //only shows when advanced tools are on
            powerInformation();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          default: break;
        }
        break;
      case KEY_CHAR_1: changePowerTimeout(); inscreen = 0; break;
      case KEY_CHAR_2: changeBacklightTimeout(); inscreen = 0; break;
      case KEY_CHAR_3: changeBacklightLevel(); inscreen = 0; break;
      case KEY_CHAR_4: if (setting_show_advanced) { setCPUclock(); } else { powerInformation(); } inscreen = 0; break;
      case KEY_CHAR_5: if (setting_show_advanced) { powerInformation(); inscreen = 0; } break;
      case KEY_CTRL_EXIT: inscreen = 0; break;
    }
  }
}
//////////////////////////////////////////
// END OF POWER OPTIONS
//////////////////////////////////////////
// LIGHT TOOLS
//////////////////////////////////////////
void lantern() {
  int key;  
  unsigned int prevlevel = GetBacklightSubLevel_RAW();
  SetBacklightSubLevel_RAW(249);
  Bdisp_AllClr_VRAM();
  GetKey(&key);
  SetBacklightSubLevel_RAW(prevlevel);
  return;
}
void flashLight() {
  unsigned short key; 
  int keyCol, keyRow; 
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  unsigned int prevlevel = 249;
  Bdisp_AllClr_VRAM();
  if (halfSecondTimer > 0) { halfSeconds = 0; Timer_Start(halfSecondTimer); } else { return; }
  unsigned int prevhalfsecond = halfSeconds;
  while (1) {
    Bdisp_PutDisp_DD();
    //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
    if (0 != GetKeyWait_OS(&keyCol,&keyRow,KEYWAIT_HALTOFF_TIMEROFF,0,1, &key) ) {
      if(keyCol == 4 && keyRow == 8) {
        SetBacklightSubLevel_RAW(initlevel); DrawFrame( COLOR_WHITE );
        if (halfSecondTimer > 0) { Timer_Stop(halfSecondTimer); }
        return;
      }
    } 
    if(halfSeconds > prevhalfsecond) { //half second has passed
      if (prevlevel == 249) { SetBacklightSubLevel_RAW(0); prevlevel = 0; Bdisp_Fill_VRAM( COLOR_BLACK, 3 ); DrawFrame( COLOR_BLACK ); }
      else { SetBacklightSubLevel_RAW(249); prevlevel = 249; Bdisp_Fill_VRAM( COLOR_WHITE, 3 ); DrawFrame( COLOR_WHITE );}
      prevhalfsecond = halfSeconds;
    }
  }
}
void morseLight() {
  unsigned short key; 
  int keyCol, keyRow; 
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  Bdisp_AllClr_VRAM();
  while (1) {
    Bdisp_PutDisp_DD();
    //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
    if (0 != GetKeyWait_OS(&keyCol,&keyRow,KEYWAIT_HALTOFF_TIMEROFF,0,1, &key) ) {
      if (keyCol == 4 && keyRow == 8) { SetBacklightSubLevel_RAW(initlevel); return; }
      SetBacklightSubLevel_RAW(249);
    } else {
      SetBacklightSubLevel_RAW(0);
    }
    
  }
}
void networkLight() {
  unsigned short key; 
  int gkey;
  int keyCol, keyRow; 
  int master = 0;
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  PrintXY(1, 1, (char*)"  Serial opening", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); Bdisp_PutDisp_DD();
  if (Serial_IsOpen() != 1) {
    unsigned char mode[6] = {0, 5, 0, 0, 0, 0};    // 9600 bps 8n1
    Serial_Open(mode);
  }
  PrintXY(1, 1, (char*)"  Serial open   ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); Bdisp_PutDisp_DD();
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Network light", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Select mode:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(1, 3, (char*)"  F1: Master", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(1, 4, (char*)"  F2: Slave", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int inscreen = 1;
  while(inscreen) {
    mGetKey(&gkey);
    switch(gkey) {
      case KEY_CTRL_F1:
        master = 1; inscreen = 0;
        break;
      case KEY_CTRL_F2:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT:
        Serial_Close(1);
        return;
        break;
    }
  }
  Bdisp_AllClr_VRAM();
  while (1) {
    Bdisp_PutDisp_DD();
    if (master) {
      //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
      if (0 != GetKeyWait_OS(&keyCol,&keyRow,KEYWAIT_HALTOFF_TIMEROFF,0,1, &key) ) {
        if (keyCol == 4 && keyRow == 8) { Serial_Close(1); SetBacklightSubLevel_RAW(initlevel); return; }
        SetBacklightSubLevel_RAW(249);
        Serial_WriteUnbuffered('1'); 
      } else {
        Serial_WriteUnbuffered('0'); 
        SetBacklightSubLevel_RAW(0);
      }
    } else {
      unsigned char out;
      short count;
      Serial_Read(&out, 1, &count);
      if (out == '1') SetBacklightSubLevel_RAW(249);
      if (out == '0') SetBacklightSubLevel_RAW(0);
      //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
      if (0 != GetKeyWait_OS(&keyCol,&keyRow,KEYWAIT_HALTOFF_TIMEROFF,0,1, &key) ) {
        if (keyCol == 4 && keyRow == 8) { Serial_Close(1); SetBacklightSubLevel_RAW(initlevel); return; }
      }
    }
  }
}
void colorLight() {
  int gkey;
  int inscreen = 1;
  int selcolor = 0;
  char buffer1[50] = "";
  unsigned int initlevel = GetBacklightSubLevel_RAW();
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Color light", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(5, 3, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 5, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 4, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    switch (selcolor) {
      case 0: strcat(buffer1, "Blue"); break;
      case 1: strcat(buffer1, "Green"); break;
      case 2: strcat(buffer1, "Red"); break;
      case 3: strcat(buffer1, "Orange"); break;
      case 4: strcat(buffer1, "Yellow"); break;
      case 5: strcat(buffer1, "Cyan"); break;
      case 6: strcat(buffer1, "Brown"); break;
      case 7: strcat(buffer1, "Magenta"); break;
    }
    PrintXY(5, 4, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mGetKey(&gkey);
    switch(gkey) {
      case KEY_CTRL_DOWN:
        if (selcolor != 0) {
          selcolor--;
        } else { selcolor = 7; }
        break;
      case KEY_CTRL_UP:
        if (selcolor != 7) {
          selcolor++;
        } else { selcolor = 0; }
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT:
        return;
        break;
    }
  }
  Bdisp_AllClr_VRAM();
  SetBacklightSubLevel_RAW(249);
  switch (selcolor) {
    case 0: Bdisp_Fill_VRAM( COLOR_BLUE, 3 ); DrawFrame( COLOR_BLUE  ); break;
    case 1: Bdisp_Fill_VRAM( COLOR_GREEN, 3 ); DrawFrame( COLOR_GREEN  ); break;
    case 2: Bdisp_Fill_VRAM( COLOR_RED, 3 ); DrawFrame( COLOR_RED  ); break;
    case 3: Bdisp_Fill_VRAM( COLOR_ORANGE, 3 ); DrawFrame( COLOR_ORANGE  ); break;
    case 4: Bdisp_Fill_VRAM( COLOR_YELLOW, 3 ); DrawFrame( COLOR_YELLOW  ); break;
    case 5: Bdisp_Fill_VRAM( COLOR_CYAN, 3 ); DrawFrame( COLOR_CYAN  ); break;
    case 6: Bdisp_Fill_VRAM( COLOR_BROWN, 3 ); DrawFrame( COLOR_BROWN  ); break;
    case 7: Bdisp_Fill_VRAM( COLOR_MAGENTA, 3 ); DrawFrame( COLOR_MAGENTA  ); break;
  }

  Bdisp_PutDisp_DD();
  GetKey(&gkey);
  SetBacklightSubLevel_RAW(initlevel);
}
void drawLightMenu(int pos, int scroll, int numitems)
{  
  drawFkeyPopup(1, setting_black_theme);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  if(setting_black_theme) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Light tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);

  if(scroll < 1) PrintXY(2,3,(char*)"  Lantern            ", (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 2) PrintXY(2,4-scroll,(char*)"  Flashlight         ", (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 3) PrintXY(2,5-scroll,(char*)"  Morse light        ", (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 4) PrintXY(2,6-scroll,(char*)"  Network light      ", (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 5 && scroll > 0) PrintXY(2,7-scroll,(char*)"  Color light        ", (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);

  TScrollbar sb;
  sb.I1 = 0;
  sb.I5 = 0;
  sb.indicatormaximum = numitems;
  sb.indicatorheight = 4;
  sb.indicatorpos = scroll;
  sb.barheight = LCD_HEIGHT_PX - 24*5;
  sb.bartop = 24*2;
  sb.barleft = LCD_WIDTH_PX - 6 - 18 - 5;
  sb.barwidth = 6;
  Scrollbar(&sb);
}

void lightMenu() {
  int key, pos = 1, scroll = 0, inscreen = 1;
  int numitems = 5; //total number of items in menu

  while(inscreen)
  {
    drawLightMenu(pos, scroll, numitems);
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(pos == numitems)
        {
          pos = 1;
          scroll = 0;
        }
        else
        {
          pos++;
          if(pos > scroll+4)
            scroll = pos -4;
        }
        break;
      case KEY_CTRL_UP:
        if(pos == 1)
        {
          pos = numitems;
          scroll = pos-4;
        }
        else
        {
          pos--;
          if(pos-1 < scroll)
            scroll = pos -1;
        }
        break;
      case KEY_CTRL_EXE:
        switch(pos)
        {
          case 1:
            lantern();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 2:
            flashLight();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 3:
            morseLight();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 4:
            networkLight();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 5:
            colorLight();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          default: break;
        }
        break;
      case KEY_CHAR_1: lantern(); inscreen = 0; break;
      case KEY_CHAR_2: flashLight(); inscreen = 0; break;
      case KEY_CHAR_3: morseLight(); inscreen = 0; break;
      case KEY_CHAR_4: networkLight(); inscreen = 0; break;
      case KEY_CHAR_5: colorLight(); inscreen = 0; break;
      case KEY_CTRL_EXIT: inscreen = 0; break;
    }
  }
}
//////////////////////////////////////////
// END OF LIGHT TOOLS
//////////////////////////////////////////
// TIME TOOLS
//////////////////////////////////////////
int eventsfordayofcurmonth[31]; //buffer for telling which days in the currently-being-worked-on month have events
int curbufmonth = 0; //buffer for telling which is the month that is in the buffer, so that the buffer can be updated when needed.
                 //it's not zero-based but its initial value is zero so that an update is forced.
int curbufyear = 0;
void loadEventsForMonth(int year, int month) {
  int day = 1;
  while (day <= monthDays[month-1] + (month == 2 && isLeap(year)) ? 1 : 0)
  {
    EventDate thisday;
    thisday.day = day; thisday.month = month; thisday.year = year;  
    eventsfordayofcurmonth[day] = GetSMEMeventsForDate(thisday, SMEM_CALENDAR_FOLDER, NULL); //NULL means it will only count and not parse
    day++;
  }
}
void drawCalendar(int year, int month, int d, int show_event_count=1)
{
    Bdisp_AllClr_VRAM();
    if (setting_display_statusbar == 1) DisplayStatusArea();
    int textX = 0;
    int textY = 0;
#define TOP 25
#define BOTTOM (LCD_HEIGHT_PX - TOP-2)
#define LEFT 25
#define RIGHT (LCD_WIDTH_PX - 26)
#define THICKNESS 20
#define TOPOFFSET 22
#define WIDTH 47
    drawLine(LEFT,TOP,RIGHT-1,TOP,COLOR_BLACK);
    drawLine(LEFT,TOP,LEFT,BOTTOM-1,COLOR_BLACK);
    drawLine(LEFT,BOTTOM-1,RIGHT-1,BOTTOM-1,COLOR_BLACK);
    drawLine(RIGHT-1,BOTTOM-1,RIGHT-1,TOP,COLOR_BLACK);
    drawRectangle(LEFT+2,TOP+2,RIGHT-2-2-LEFT,THICKNESS,COLOR_BLACK);
    textX=LEFT+5; textY= TOP+2-TOPOFFSET;
    PrintMini(&textX, &textY, (unsigned char*)monthNames[month-1], 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
    int x,y,k = 0;
    drawRectangle(LEFT+2,TOP+2+THICKNESS,RIGHT-2-2-LEFT,THICKNESS,COLOR_LIME);
    drawRectangle(RIGHT-2-WIDTH,TOP+2+2*THICKNESS,WIDTH,THICKNESS*6,COLOR_AQUA);
    drawRectangle(LEFT+2,TOP+2+2*THICKNESS,WIDTH,THICKNESS*6,COLOR_AQUA);
    drawLine(LEFT+2,TOP+2+THICKNESS,RIGHT-3,TOP+2+THICKNESS,COLOR_BLACK);
    for (x = LEFT + 2+WIDTH; x < RIGHT - WIDTH;x+=WIDTH)
    {
        drawLine(x,TOP+2+THICKNESS,x,BOTTOM-3, COLOR_BLACK);
    }
    for (y = TOP+2+2*THICKNESS; y < BOTTOM - THICKNESS; y+=THICKNESS)
    {
        drawLine(LEFT+2,y,RIGHT-3,y,COLOR_BLACK);
    }
    for (x = LEFT+2; x < RIGHT - WIDTH; x+= WIDTH)
    {
        PrintMiniFix(x+2,TOP+2+THICKNESS-TOPOFFSET,dayofweekshort[k++],0,COLOR_BLUE, COLOR_LIME);
    }

    int startingday = dow(year,month,1),day = 1;
    int prevstart = monthDays[(month == 1 ? 11 : month - 2)] - (startingday == 0 ? 7 : startingday) + ((month == 3 && isLeap(year)) ? 2 : 1);
    char buffer[10];
    if (startingday != 0) { //solve "overlapping days on months started on a sunday" bug
      for (x = 0; x < (startingday == 0 ? 7 : startingday); x++)
      {
          itoa(prevstart++,(unsigned char*)buffer);
          PrintMiniFix(LEFT+2+x*WIDTH+2,TOP+2+2*THICKNESS-TOPOFFSET,buffer,0,(x == 0 ? COLOR_LIGHTSLATEGRAY : COLOR_LIGHTGRAY),(x == 0 ? COLOR_AQUA : COLOR_WHITE));
      }
    }
    x = startingday;
    y = 2;
    while (day <= monthDays[month-1] + (month == 2 && isLeap(year)) ? 1 : 0)
    {
        itoa(day,(unsigned char*)buffer);
        if (day == d) {
          drawRectangle(LEFT+2+WIDTH*x+1,TOP+1+2+y*THICKNESS,WIDTH-1,THICKNESS-1,COLOR_RED);
          if(x == 0) { drawRectangle(LEFT+2+WIDTH*x,TOP+1+2+y*THICKNESS,WIDTH,THICKNESS-1,COLOR_RED); } //make sure the little pixels row on sundays is filled
        }
        //events indicator:            
        if (show_event_count) {
          if (curbufmonth!=month || curbufyear!=year) { //events in buffer are not for this month, refresh.
            loadEventsForMonth(year, month);
            curbufmonth = month; //update which month is now in buffer
            curbufyear = year; //update which year is now in buffer
          }
          if(eventsfordayofcurmonth[day] > 0) {
            int textX = LEFT+2+WIDTH*x+2+12*2+2; //12+2 to have space to write the day and some padding
            int textY = TOP+2+y*THICKNESS-TOPOFFSET+2; //+2 to have some padding
            unsigned char eventstr[10] = "";
            if (eventsfordayofcurmonth[day] < 100) {
              itoa(eventsfordayofcurmonth[day], (unsigned char*)eventstr); 
            } else {
              strcpy((char*)eventstr, (char*)"++");
            }
            PrintMiniMini( &textX, &textY, (unsigned char*)eventstr, 0, TEXT_COLOR_BLACK, 0 );            
            if(day == d) { VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+2+24, 8*2, 12, COLOR_WHITE, COLOR_RED); VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+2+24, 8*2, 12, COLOR_BLACK, COLOR_WHITE);}
            else if(x == 0 || x == 6) { VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+2+24, 8*2, 12, COLOR_WHITE, COLOR_AQUA);}            
          }
        }
        //end of events indicator
        PrintMiniFix(LEFT+2+WIDTH*x+2,TOP+2+y*THICKNESS-TOPOFFSET,buffer,0,(day == d ? COLOR_WHITE : COLOR_BLACK),(day == d ? COLOR_RED : (x == 0 || x == 6) ? COLOR_AQUA : COLOR_WHITE));
        
        x++;
        day++;
        if (x == 7)
        {
            x = 0;
            y++;
        }
    }
    day = 1;
    while (y != 8)
    {
        itoa(day++,(unsigned char*)buffer);
        PrintMiniFix(LEFT+2+WIDTH*x+2,TOP+2+y*THICKNESS-TOPOFFSET,buffer,0,((x == 0) | (x == 6)) ? COLOR_LIGHTSLATEGRAY : COLOR_LIGHTGRAY,(x == 0 || x == 6) ? COLOR_AQUA : COLOR_WHITE);
        x++;
        if (x == 7)
        {
            x = 0;
            y++;
        }
    }
    itoa(year,(unsigned char*)buffer);
    textX = 0; //RIGHT-5-50;
    textY = TOP+2-TOPOFFSET;
    PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 0, 0);
    textX = RIGHT-4-textX;
    PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
}

int chooseCalendarDate(int *yr, int *m, int *d, char* message, char* message2, int inwizard=0, int allowF1=0, int laststep=0, int showF2msg=0, int newFileBackground=0)
{ //returns 0 on success, 1 on error, 2 on user abort
  char buffer[8] = ""; // Some string length
  int start = 0; // Used for scrolling left and right
  int cursor = 0; // Cursor position

  buffer[0] = '\0'; // This sets the first character to \0, also represented by "", an empty string
  char buffer2[8] = "";
  if(*yr != 0 || *m != 0 || *d != 0) {
    switch(setting_dateformat) {
      case 0:
        if (*d < 10) { strcat(buffer, "0"); }
        itoa(*d, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (*m < 10) { strcat(buffer, "0"); }
        itoa(*m, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (*yr < 1000) { strcat(buffer, "0"); }
        if (*yr < 100) { strcat(buffer, "0"); }
        if (*yr < 10) { strcat(buffer, "0"); }
        itoa(*yr, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        break;
      case 1:
        if (*m < 10) { strcat(buffer, "0"); }
        itoa(*m, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (*d < 10) { strcat(buffer, "0"); }
        itoa(*d, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (*yr < 1000) { strcat(buffer, "0"); }
        if (*yr < 100) { strcat(buffer, "0"); }
        if (*yr < 10) { strcat(buffer, "0"); }
        itoa(*yr, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        break;
      case 2:
        if (*yr < 1000) { strcat(buffer, "0"); }
        if (*yr < 100) { strcat(buffer, "0"); }
        if (*yr < 10) { strcat(buffer, "0"); }
        itoa(*yr, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
    
        if (*m < 10) { strcat(buffer, "0"); }
        itoa(*m, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (*d < 10) { strcat(buffer, "0"); }
        itoa(*d, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        break;
    }
    
    cursor = 8;
  }
  Bdisp_AllClr_VRAM();
  if (newFileBackground == 1) SetBackGround(0x0A);
  if (newFileBackground == 2) SetBackGround(6);
  SetSetupSetting( (unsigned int)0x14, 0); //we only accept numbers, so switch off alpha/shift
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)message, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)message2, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(1, 3, (char*)"  Date: ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int x = 6; int y = 3; int fieldwidth = 8;
  DisplayMBString2( 0, (unsigned char*)buffer, start, cursor, 0, x, y*24-24, fieldwidth+x, 0 );
  switch(setting_dateformat) {
    case 0:
      PrintXY(6, 4, (char*)"  DDMMYYYY", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      break;
    case 1:
      PrintXY(6, 4, (char*)"  MMDDYYYY", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      break;
    case 2:
      PrintXY(6, 4, (char*)"  YYYYMMDD", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      break;
  }
  if (showF2msg) { PrintXY(1, 5, (char*)"  F2:Same as start date", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); }
  if (inwizard) {
    int iresult;
    if (allowF1) {
      GetFKeyPtr(0x036F, &iresult); // <
      FKey_Display(0, (int*)iresult);
    }
    if (laststep) {
      GetFKeyPtr(0x04A4, &iresult); // Finish
      FKey_Display(5, (int*)iresult);
    } else {
      GetFKeyPtr(0x04A3, &iresult); // Next
      FKey_Display(5, (int*)iresult);
    }
  }  
  
  Cursor_SetFlashOn(0);
  int key;
  while(1)
  {
    //top, bottom lines
    drawLine(x*18-18, y*24-1, (x*18-18)+18*fieldwidth, y*24-1, COLOR_GRAY);
    drawLine(x*18-18, y*24+22, (x*18-18)+18*fieldwidth, y*24+22, COLOR_GRAY);
    //vertical lines, start and end
    drawLine(x*18-18-1, y*24-1, x*18-18-1, y*24+22, COLOR_GRAY);
    drawLine((x*18-18)+18*fieldwidth+1, y*24-1, (x*18-18)+18*fieldwidth+1, y*24+22, COLOR_GRAY);
    //vertical lines: dd, mm and yyyy separators
    switch(setting_dateformat) {
      case 0:
      case 1:
        drawLine((x*18-18)+18*2, y*24-1, (x*18-18)+18*2, y*24+22, COLOR_GRAY);
        drawLine((x*18-18)+18*4+1, y*24-1, (x*18-18)+18*4+1, y*24+22, COLOR_GRAY);
        break;
      case 2:
        drawLine((x*18-18)+18*4, y*24-1, (x*18-18)+18*4, y*24+22, COLOR_GRAY);
        drawLine((x*18-18)+18*6+1, y*24-1, (x*18-18)+18*6+1, y*24+22, COLOR_GRAY);
        break;
    }
    mGetKey(&key);
    if (inwizard) {
      if(key == KEY_CTRL_F1 && allowF1) {
        Cursor_SetFlashOff();
        return 3;
      }
    }
    if (inwizard && showF2msg && key == KEY_CTRL_F2) {
      Cursor_SetFlashOff();
      return 4;
    }
    if(key == KEY_CTRL_EXIT) {
      Cursor_SetFlashOff();
      return 2;
    }
    if(key == KEY_CTRL_EXE && cursor == 8) {
      break;
    }
    if(inwizard && key == KEY_CTRL_F6 && cursor == 8) {
      break;
    }
    if(key && key < 30000) {
      if (key >= KEY_CHAR_0 && key <= KEY_CHAR_9) {
        //don't allow for typing non-digits
        cursor = EditMBStringChar((unsigned char*)buffer, 8, cursor, key);
        DisplayMBString2( 0, (unsigned char*)buffer, start, cursor, 0, x, y*24-24, fieldwidth+x, 0 );
      }
    } else {
      EditMBStringCtrl2( (unsigned char*)buffer, 8+1, &start, &cursor, &key, x, y*24-24, 1, fieldwidth+x-1 );
    }
  }
  Cursor_SetFlashOff();
  char year[6] = "";
  char month[2] = "";
  char day[2] = "";
  switch(setting_dateformat) {
    case 0:
      day[0] = buffer[0]; day[1] = buffer[1];
      month[0] = buffer[2]; month[1] = buffer[3];
      year[0] = buffer[4]; year[1] = buffer[5]; year[2] = buffer[6]; year[3] = buffer[7];
      break;
    case 1:
      day[0] = buffer[2]; day[1] = buffer[3];
      month[0] = buffer[0]; month[1] = buffer[1];
      year[0] = buffer[4]; year[1] = buffer[5]; year[2] = buffer[6]; year[3] = buffer[7];
      break;
    case 2:
      day[0] = buffer[6]; day[1] = buffer[7];
      month[0] = buffer[4]; month[1] = buffer[5];
      year[0] = buffer[0]; year[1] = buffer[1]; year[2] = buffer[2]; year[3] = buffer[3];
      break;
  }

  *yr = sys_atoi(year);
  *m = sys_atoi(month);
  *d = sys_atoi(day);
  if (*m > 12 || *m <= 0 || *d > monthDays[*m-1] || *m <= 0) {
    MsgBoxPush(3);
    PrintXY(3, 3, (char*)"  Invalid date.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mGetKey(&key);
    MsgBoxPop();
    return 1;
  }
  return 0;
}
int chooseTime(int *h, int *m, int *s, char* message, char* message2, int inwizard=0, int allowF1=0, int laststep=0, int showalldaymsg=0, int newFileBackground=0)
{ //returns 0 on success, 1 on error, 2 on user abort, 3 on previous (when inwizard), 4 on F2 (all-day event preference)
  char buffer[8] = ""; // Some string length
  int start = 0; // Used for scrolling left and right
  int cursor = 0; // Cursor position

  buffer[0] = '\0'; // This sets the first character to \0, also represented by "", an empty string
  char buffer2[8] = ""; // Some string length
  if(*h != 0 || *m != 0 || *s != 0) {
    if (*h < 10) { strcat(buffer, "0"); }
    itoa(*h, (unsigned char*) buffer2);
    strcat(buffer, buffer2);

    if (*m < 10) { strcat(buffer, "0"); }
    itoa(*m, (unsigned char*) buffer2);
    strcat(buffer, buffer2);
    
    if (*s < 10) { strcat(buffer, "0"); }
    itoa(*s, (unsigned char*) buffer2);
    strcat(buffer, buffer2);
    cursor = 6;
  }
  Bdisp_AllClr_VRAM();
  if (newFileBackground == 1) { SetBackGround(0x0A); }
  if (newFileBackground == 2) { SetBackGround(6); }
  SetSetupSetting( (unsigned int)0x14, 0); //we only accept numbers, so switch off alpha/shift
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)message, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)message2, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(1, 3, (char*)"  Time: ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int x = 6; int y = 3; int fieldwidth = 6;
  DisplayMBString2( 0, (unsigned char*)buffer, start, cursor, 0, x, y*24-24, fieldwidth+x, 0 );
  PrintXY(6, 4, (char*)"  HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);

  if (inwizard) {
    int iresult;
    if (allowF1) {
      GetFKeyPtr(0x036F, &iresult); // <
      FKey_Display(0, (int*)iresult);
    }
    if (laststep) {
      GetFKeyPtr(0x04A4, &iresult); // Finish
      FKey_Display(5, (int*)iresult);
    } else {
      GetFKeyPtr(0x04A3, &iresult); // Next
      FKey_Display(5, (int*)iresult);
    }
    if (showalldaymsg) { PrintXY(1, 5, (char*)"  F2: All-day event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); }
  }  
  
  Cursor_SetFlashOn(0);
  int key;
  while(1)
  {
    //top, bottom lines
    drawLine(x*18-18, y*24-1, (x*18-18)+18*fieldwidth, y*24-1, COLOR_GRAY);
    drawLine(x*18-18, y*24+22, (x*18-18)+18*fieldwidth, y*24+22, COLOR_GRAY);
    //vertical lines, start and end
    drawLine(x*18-18-1, y*24-1, x*18-18-1, y*24+22, COLOR_GRAY);
    drawLine((x*18-18)+18*fieldwidth+1, y*24-1, (x*18-18)+18*fieldwidth+1, y*24+22, COLOR_GRAY);
    //vertical lines: hh, mm and ss separators
    drawLine((x*18-18)+18*2, y*24-1, (x*18-18)+18*2, y*24+22, COLOR_GRAY);
    drawLine((x*18-18)+18*4+1, y*24-1, (x*18-18)+18*4+1, y*24+22, COLOR_GRAY);
    mGetKey(&key);
    if (inwizard) {
      if(key == KEY_CTRL_F1 && allowF1) {
        Cursor_SetFlashOff();
        return 3;
      }
      if(key == KEY_CTRL_F2 && showalldaymsg) {
        return 4;
      }
    }
    if(key == KEY_CTRL_EXIT) {
      Cursor_SetFlashOff();
      return 2;
    }
    if(key == KEY_CTRL_EXE && cursor == 6) {
      break;
    }
    if(inwizard && key == KEY_CTRL_F6 && cursor == 6) {
      break;
    }
    if(key && key < 30000) {
      if (key >= KEY_CHAR_0 && key <= KEY_CHAR_9) {
        //don't allow for typing non-digits
        cursor = EditMBStringChar((unsigned char*)buffer, 6, cursor, key);
        DisplayMBString2( 0, (unsigned char*)buffer, start, cursor, 0, x, y*24-24, fieldwidth+x, 0 );
      }
    } else {
      EditMBStringCtrl2( (unsigned char*)buffer, 6+1, &start, &cursor, &key, x, y*24-24, 1, fieldwidth+x-1 );
    }
  }
  Cursor_SetFlashOff();
  char hour[2] = "";
  char minute[2] = "";
  char second[2] = "";
  hour[0] = buffer[0]; hour[1] = buffer[1];
  minute[0] = buffer[2]; minute[1] = buffer[3];
  second[0] = buffer[4]; second[1] = buffer[5];

  *h = sys_atoi(hour);
  *m = sys_atoi(minute);
  *s = sys_atoi(second);
  if (*h > 23 || *h < 0 || *m > 59 || *m < 0 || *s > 59 || *s < 0) {
    MsgBoxPush(3);
    PrintXY(3, 3, (char*)"  Invalid time.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mGetKey(&key);
    MsgBoxPop();
    return 1;
  }
  return 0;
}

void delAllEventUI(int y, int m, int d, int istask=0) {
  EventDate date; date.day = d; date.month = m; date.year = y;
  MsgBoxPush(4);
  if (istask) {
    PrintXY(3, 2, (char*)"  Delete All Tasks?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    PrintXY(3, 2, (char*)"  Delete All Events", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY(3, 3, (char*)"  on Selected Day?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  PrintXY(3, 4, (char*)"     Yes:[F1]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(3, 5, (char*)"     No :[F6]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int key,inscreen=1;
  while(inscreen) {
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_F1:
        RemoveSMEMDay(date, CALENDAR_FOLDER);
        curbufmonth = 0; //force calendar event counts to refresh
        inscreen=0;
        break;
      case KEY_CTRL_F6:
      case KEY_CTRL_EXIT:
        inscreen=0;
        break;
    }
  }
  MsgBoxPop();
}
void dbgPrint(unsigned char*msg) {
  int key;
  locate_OS(1,1);
  Print_OS((unsigned char*)"                     ", 0, 0);
  locate_OS(1,1);
	Print_OS(msg, 0, 0);
	GetKey(&key);
}
void delEventUI(int y, int m, int d, int pos, int istask=0) {
  EventDate date; date.day = d; date.month = m; date.year = y;
  MsgBoxPush(4);
  PrintXY(3, 2, (char*)"  Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  if(istask) {
    PrintXY(3, 3, (char*)"  Selected Task?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    PrintXY(3, 3, (char*)"  Selected Event?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  PrintXY(3, 4, (char*)"     Yes:[F1]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(3, 5, (char*)"     No :[F6]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int key,inscreen=1;
  int delres;
  while(inscreen) {
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_F1:
        delres = RemoveSMEMEvent(date, pos, CALENDAR_FOLDER);
        if(delres!=0) {
          unsigned char res[10] = "";
          itoa(delres, (unsigned char*)res);
          dbgPrint((unsigned char*)res);
        }
        curbufmonth = 0; //force calendar event counts to refresh
        inscreen=0;
        break;
      case KEY_CTRL_F6:
      case KEY_CTRL_EXIT:
        inscreen=0;
        break;
    }
  }
  MsgBoxPop();
}
long long int currentUnixTime();

int newEventTextInput(int x, int y, int charlimit, unsigned char* buffer, int allowF1=0, int laststep=0, int forcetext=0, int key=0) {
  //returns: 0 on success, 1 on user abort (EXIT), 2 on EXE, 3 on F1 (when allowF1 is 1)
  int start = 0, cursor = 0;
  int fieldwidth = 21; //something new on mbstring2
  int iresult;
  
  if (allowF1) {
    GetFKeyPtr(0x036F, &iresult); // <
    FKey_Display(0, (int*)iresult);
  }
  GetFKeyPtr(0x02A1, &iresult); // CHAR
  FKey_Display(3, (int*)iresult);
  GetFKeyPtr(0x0307, &iresult); // A<>a
  FKey_Display(4, (int*)iresult);
  if (laststep) {
    GetFKeyPtr(0x04A4, &iresult); // Finish
    FKey_Display(5, (int*)iresult);
  } else {
    GetFKeyPtr(0x04A3, &iresult); // Next
    FKey_Display(5, (int*)iresult);
  }
  drawLine(0, y*24-1, LCD_WIDTH_PX-1, y*24-1, COLOR_GRAY);
  drawLine(0, y*24+24, LCD_WIDTH_PX-1, y*24+24, COLOR_GRAY);
  if (key) { cursor = EditMBStringChar((unsigned char*)buffer, charlimit, cursor, key); }
  while(1)
  {
    //DisplayMBString((unsigned char*)buffer, start, cursor, x, y);
    DisplayMBString2( 0, (unsigned char*)buffer, start, cursor, 0, x, y*24-24, fieldwidth+x-1, 0 );
    
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    mGetKey(&key);
    if (GetSetupSetting( (unsigned int)0x14) == 0x01 || GetSetupSetting( (unsigned int)0x14) == 0x04 || GetSetupSetting( (unsigned int)0x14) == 0x84) {
      keyflag = GetSetupSetting( (unsigned int)0x14); //make sure the flag we're using is the updated one.
      //we can't update always because that way alpha-not-lock will cancel when F5 is pressed.
    }
    if(key == KEY_CTRL_F1)
    {
      if(allowF1) { Cursor_SetFlashOff(); return 3; }
    }
    if(key == KEY_CTRL_EXE || key == KEY_CTRL_F6)
    {
      // Next step
      if(forcetext) {
        if (strlen((char*)buffer) > 0) {
          Cursor_SetFlashOff(); return 2;
        } else {
          MsgBoxPush(3);
          PrintXY(3, 3, (char*)"  Field can't be", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(3, 4, (char*)"  empty.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mGetKey(&key);
          MsgBoxPop();
        }
      } else {
        Cursor_SetFlashOff(); return 2;
      }
    }
    else if(key == KEY_CTRL_EXIT)
    {
      // Aborted
      Cursor_SetFlashOff(); return 1;
    }
    else if(key == KEY_CTRL_F4)
    {
      SaveVRAM_1();
      Bkey_ClrAllFlags();
      short character;
      character = CharacterSelectDialog();
      if (character) cursor = EditMBStringChar((unsigned char*)buffer, charlimit, cursor, character);
      LoadVRAM_1();
    }  
    else if(key == KEY_CTRL_F5)
    {
      if (keyflag == 0x04 || keyflag == 0x08 || keyflag == 0x84 || keyflag == 0x88) {
        // ^only applies if some sort of alpha (not locked) is already on
        if (keyflag == 0x08 || keyflag == 0x88) { //if lowercase
          SetSetupSetting( (unsigned int)0x14, keyflag-0x04);
          if (setting_display_statusbar == 1) DisplayStatusArea();
          continue; //do not process the key, because otherwise we will leave alpha status
        } else {
          SetSetupSetting( (unsigned int)0x14, keyflag+0x04);
          if (setting_display_statusbar == 1) DisplayStatusArea();
          continue; //do not process the key, because otherwise we will leave alpha status
        }
      }

    } 
    if (setting_display_statusbar == 1) DisplayStatusArea(); 
    if(key && key < 30000)
    {
      if ((GetSetupSetting( (unsigned int)0x14) == 0x08 || GetSetupSetting( (unsigned int)0x14) == 0x88) && key >= KEY_CHAR_A && key <= KEY_CHAR_Z) //if lowercase and key is char...
      {
        key = key + 32; //so we switch to lowercase characters... Casio is smart
      }
      cursor = EditMBStringChar((unsigned char*)buffer, charlimit, cursor, key);
      //DisplayMBString((unsigned char*)buffer, start, cursor, x,y);
      //1st and 5th parameter are, respectively, whether selection is enabled and its starting point. 
      DisplayMBString2( 0, (unsigned char*)buffer, start, cursor, 0, x, y*24-24, fieldwidth+x-1, 0 );
    }
    else
    {
      //EditMBStringCtrl((unsigned char*)buffer, charlimit, &start, &cursor, &key, x, y);
      EditMBStringCtrl2( (unsigned char*)buffer, charlimit+1, &start, &cursor, &key, x, y*24-24, 1, fieldwidth+x-1 );
    }
  }
  Cursor_SetFlashOff();
  return 0;
}
void addEventUI(int y, int m, int d, int key=0) {
  CalendarEvent event;
  EventDate startdate;
  EventDate enddate;
  EventTime starttime;
  EventTime endtime;
  
  startdate.day = d;
  startdate.month = m;
  startdate.year = y;
  event.startdate = startdate;
  int chooseres;
  int h=0, min=0, s=0;
  int ey=0, em=0, ed=0;
  
  //clean buffers:
  strcpy((char*)event.title, "");
  strcpy((char*)event.location, "");
  strcpy((char*)event.description, "");
addEventTitleScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Title:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  
  int res = newEventTextInput(1, 3, 21, event.title, 0, 0, 1, key); //force text so title must be at least one char
  if (res == 1) { return; }
addEventLocationScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Location:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 128, event.location, 1);
  if (res == 3) { goto addEventTitleScreen; }
  else if (res == 1) { return; }
addEventDescriptionScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Description:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 1024, event.description, 1);
  if (res == 3) { goto addEventLocationScreen; }
  else if (res == 1) { return; }   
addEventStartTimeScreen:
  h=0, min=0, s=0;
  chooseres = chooseTime(&h, &min, &s, (char*) "  Add New Event", (char*)"  Start time:", 1, 1, 0, 1, 1);
  switch(chooseres) {
    case 0: //success
      starttime.hour = h;
      starttime.minute = min;
      starttime.second = s;
      event.starttime = starttime;
      event.timed = 1;
      break;
    case 1: //invalid time, restart
      goto addEventStartTimeScreen;
      break;
    case 2: //user abort on EXIT
      return;
      break;
    case 3: //user pressed F1 - previous
      goto addEventDescriptionScreen;
      break;
    case 4: //user pressed F2 - all-day event (no start time)
      starttime.hour = 0;
      starttime.minute = 0;
      starttime.second = 0;
      event.starttime = starttime;
      event.timed = 0;
      break;
  }

addEventEndDateScreen:
  ey=0, em=0, ed=0;
  chooseres = chooseCalendarDate(&ey, &em, &ed, (char*) "  Add New Event", (char*)"  End date:", 1, 1, 0, 1, 1 );
  switch(chooseres) {
    case 0: //success
        enddate.day = ed;
        enddate.month = em;
        enddate.year = ey;
        event.enddate = enddate;
      break;
    case 1: //invalid date, restart
      goto addEventEndDateScreen;
      break;
    case 2: //user abort on EXIT
      return;
      break;
    case 3: //user pressed F1 - previous
      goto addEventStartTimeScreen;
      break;
    case 4: //user pressed F2 - date is same as start date
      event.enddate = startdate;
      break;
  }

addEventEndTimeScreen:
  if(event.timed) {
    h=0, min=0, s=0;
    chooseres = chooseTime(&h, &min, &s, (char*) "  Add New Event", (char*)"  End time:", 1, 1, 0, 0, 1);
    switch(chooseres) {
      case 0: //success
        endtime.hour = h;
        endtime.minute = min;
        endtime.second = s;
        event.endtime = endtime;
        break;
      case 1: //invalid time, restart
        goto addEventEndTimeScreen;
        break;
      case 2: //user abort on EXIT
        return;
        break;
      case 3: //user pressed F1 - previous
        goto addEventEndDateScreen;
        break;
    }
  } else {
    endtime.hour = 0;
    endtime.minute = 0;
    endtime.second = 0;
    event.endtime = endtime;
  }

  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Select category", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  int iresult, inscreen=1; key = 0;
  GetFKeyPtr(0x036F, &iresult); // <
  FKey_Display(0, (int*)iresult);
  GetFKeyPtr(0x04A4, &iresult); // Finish
  FKey_Display(5, (int*)iresult);
  char buffer1[20];
  char buffer2[20];
  event.category = 1;
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"   ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(event.category, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, (event.category <= 6 ? event.category-1 : TEXT_COLOR_YELLOW));
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (event.category == 0) {
          event.category = 7;
        } else {
          event.category--;
        }
        break;
      case KEY_CTRL_UP:
        if (event.category == 7) {
          event.category = 0;
        } else {
          event.category++;
        }
        break;
      case KEY_CTRL_F1:
        if (event.timed) { goto addEventEndTimeScreen; }
        else { goto addEventEndDateScreen; }
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_F6:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }

  event.daterange = 0;
  event.repeat = 0;
  event.dayofweek = dow(startdate.day, startdate.month, startdate.year);
  res = AddSMEMEvent(event, CALENDAR_FOLDER);
  if(res > 0) {
    MsgBoxPush(4);
    if (res == 4) {
      PrintXY(3, 2, (char*)"  Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else {
      PrintXY(3, 2, (char*)"  Event add. ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
    PrintXY(3, 3, (char*)"  Event could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY(3, 4, (char*)"  be added.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY(3, 5, (char*)"     Press:[EXIT]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    int key,inscreen=1;
    while(inscreen) {
      mGetKey(&key);
      switch(key)
      {
        case KEY_CTRL_EXIT:
          inscreen=0;
          break;
      }
    }
    MsgBoxPop();
  }
  curbufmonth = 0; //force calendar event counts to refresh
}

void editEventUI(CalendarEvent event, int pos) {
  //For now, the start date is not editable, but here is the code for when it is, anyway...
  EventDate oldstartdate;
  oldstartdate.day = event.startdate.day;
  oldstartdate.month = event.startdate.month;
  oldstartdate.year = event.startdate.year;
  
  int chooseres;
  int h=0, min=0, s=0;
  int ey=0, em=0, ed=0;

editEventTitleScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Title:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  
  int res = newEventTextInput(1, 3, 21, event.title, 0, 0, 1); //force text so title must be at least one char
  if (res == 1) { return; }
editEventLocationScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Location:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 128, event.location, 1);
  if (res == 3) { goto editEventTitleScreen; }
  else if (res == 1) { return; }
editEventDescriptionScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Description:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 1024, event.description, 1);
  if (res == 3) { goto editEventLocationScreen; }
  else if (res == 1) { return; }   
editEventStartTimeScreen:
  h=event.starttime.hour, min=event.starttime.minute, s=event.starttime.second;
  chooseres = chooseTime(&h, &min, &s, (char*) "  Edit Event", (char*)"  Start time:", 1, 1, 0, 1, 2);
  switch(chooseres) {
    case 0: //success
      event.starttime.hour = h;
      event.starttime.minute = min;
      event.starttime.second = s;
      event.timed = 1;
      break;
    case 1: //invalid time, restart
      goto editEventStartTimeScreen;
      break;
    case 2: //user abort on EXIT
      return;
      break;
    case 3: //user pressed F1 - previous
      goto editEventDescriptionScreen;
      break;
    case 4: //user pressed F2 - all-day event (no start time)
      event.starttime.hour = 0;
      event.starttime.minute = 0;
      event.starttime.second = 0;
      event.timed = 0;
      break;
  }

editEventEndDateScreen:
  ey=event.enddate.year, em=event.enddate.month, ed=event.enddate.day;
  chooseres = chooseCalendarDate(&ey, &em, &ed, (char*) "  Edit Event", (char*)"  End date:", 1, 1, 0, 1, 2 );
  switch(chooseres) {
    case 0: //success
        event.enddate.day = ed;
        event.enddate.month = em;
        event.enddate.year = ey;
      break;
    case 1: //invalid date, restart
      goto editEventEndDateScreen;
      break;
    case 2: //user abort on EXIT
      return;
      break;
    case 3: //user pressed F1 - previous
      goto editEventStartTimeScreen;
      break;
    case 4: //user pressed F2 - date is same as start date
      event.enddate = event.startdate;
      break;
  }

editEventEndTimeScreen:
  if(event.timed) {
    h=event.endtime.hour, min=event.endtime.minute, s=event.endtime.second;
    chooseres = chooseTime(&h, &min, &s, (char*) "  Edit Event", (char*)"  End time:", 1, 1, 0, 0, 2);
    switch(chooseres) {
      case 0: //success
        event.endtime.hour = h;
        event.endtime.minute = min;
        event.endtime.second = s;
        //event.endtime = endtime;
        break;
      case 1: //invalid time, restart
        goto editEventEndTimeScreen;
        break;
      case 2: //user abort on EXIT
        return;
        break;
      case 3: //user pressed F1 - previous
        goto editEventEndDateScreen;
        break;
    }
  } else {
    event.endtime.hour = 0;
    event.endtime.minute = 0;
    event.endtime.second = 0;
  }

  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Select category", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  int iresult, key, inscreen=1;
  GetFKeyPtr(0x036F, &iresult); // <
  FKey_Display(0, (int*)iresult);
  GetFKeyPtr(0x04A4, &iresult); // Finish
  FKey_Display(5, (int*)iresult);
  char buffer1[20];
  char buffer2[20];
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"   ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(event.category, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, (event.category <= 6 ? event.category-1 : TEXT_COLOR_YELLOW));
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (event.category == 0) {
          event.category = 7;
        } else {
          event.category--;
        }
        break;
      case KEY_CTRL_UP:
        if (event.category == 7) {
          event.category = 0;
        } else {
          event.category++;
        }
        break;
      case KEY_CTRL_F1:
        if (event.timed) { goto editEventEndTimeScreen; }
        else { goto editEventEndDateScreen; }
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_F6:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }

  event.daterange = 0;
  event.repeat = 0;
  event.dayofweek = dow(event.startdate.day, event.startdate.month, event.startdate.year);
  EditSMEMEvent(oldstartdate, pos, CALENDAR_FOLDER, event);
  curbufmonth = 0; //force calendar event counts to refresh
}

void changeEventCategory(CalendarEvent event, int pos) {
  //Allows for changing the category of a event/task with a color select dialog.
  unsigned char selcolor = (unsigned char) 0xFF; //just so it isn't uninitialized
  selcolor = ColorIndexDialog1( 0, 0 );
  if(selcolor != (unsigned char)0xFF) {
    //user didn't press EXIT, QUIT or AC/ON. input is validated.
    selcolor != 7 ? event.category = selcolor+1 : event.category = 0;
    EditSMEMEvent(event.startdate, pos, CALENDAR_FOLDER, event);
  }
}

void viewEvent(CalendarEvent calevent, int istask=0) {
  int key, inscreen = 1;
  int textX=0, textY=24;
  int scroll = 0;

  while(inscreen)
  {
    Bdisp_AllClr_VRAM();
    if (setting_display_statusbar == 1) DisplayStatusArea();

    textX=0; textY=scroll;
    textY = textY+24;
    PrintMini(&textX, &textY, (unsigned char*)"Location: ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
    //location line breaking:
    
    int temptextY = 0;
    int temptextX = 0;
    unsigned char singleword[1030] = ""; //buffer that will hold a single word, for use in multi-line
    unsigned char asrc[1030] = ""; //buffer over which string manipulations will happen
    strcpy((char*)asrc, (char*)calevent.location); 

    unsigned char* src = asrc;

    while(*src)
    {
      temptextX = 0;
      src = toksplit(src, ' ', (unsigned char*)singleword, 130); //break into words; next word
      //check if printing this word would go off the screen, with fake PrintMini drawing:
      PrintMini(&temptextX, &temptextY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0);
      if(temptextX + textX > LCD_WIDTH_PX-6) {
        //time for a new line
        textX=0;
        textY=textY+17;
        PrintMini(&textX, &textY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      } else {
        //still fits, print new word normally
        PrintMini(&textX, &textY, (unsigned char*)singleword, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      }
      //add a space, since it was removed from token
      PrintMini(&textX, &textY, (unsigned char*)" ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    }
    unsigned char buffer[50] = "";
    unsigned char buffer2[50] = "";
    if(!istask) {
      textY = textY + 25; textX=0;
      PrintMini(&textX, &textY, (unsigned char*)"Starts on ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
      strcpy((char*)buffer, "");
      fillCustomDate((char*)buffer, calevent.startdate.year, calevent.startdate.month, calevent.startdate.day);
      strcat((char*)buffer, " ");
      if (calevent.timed) {
        fillCustomTime((char*)buffer2, calevent.starttime.hour, calevent.starttime.minute, calevent.starttime.second);
        strcat((char*)buffer, (char*)buffer2);
      } else {
        strcat((char*)buffer, (char*)"(all day)");
      }
      PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textY = textY + 17; textX=0;
      PrintMini(&textX, &textY, (unsigned char*)"Ends on ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
      strcpy((char*)buffer, "");
      fillCustomDate((char*)buffer, calevent.enddate.year, calevent.enddate.month, calevent.enddate.day);
      strcat((char*)buffer, " ");
      strcpy((char*)buffer2, ""); //clear buffer2
      if (calevent.timed) {
        fillCustomTime((char*)buffer2, calevent.endtime.hour, calevent.endtime.minute, calevent.endtime.second);
        strcat((char*)buffer, (char*)buffer2);
      }
      PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); 
    }
    textY = textY + 17; textX=0;
    PrintMini(&textX, &textY, (unsigned char*)"Category: ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
    switch(calevent.category) {
      case 0:
        PrintMini(&textX, &textY, (unsigned char*)"0", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
        break;
      case 1:
        PrintMini(&textX, &textY, (unsigned char*)"1", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        break;
      case 2:
        PrintMini(&textX, &textY, (unsigned char*)"2", 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
        break;
      case 3:
        PrintMini(&textX, &textY, (unsigned char*)"3", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGREEN, COLOR_WHITE, 1, 0);        
        break;
      case 4:
        PrintMini(&textX, &textY, (unsigned char*)"4", 0, 0xFFFFFFFF, 0, 0, COLOR_AQUA, COLOR_WHITE, 1, 0);
        break;
      case 5:
        PrintMini(&textX, &textY, (unsigned char*)"5", 0, 0xFFFFFFFF, 0, 0, COLOR_RED, COLOR_WHITE, 1, 0);        
        break;
      case 6:
        PrintMini(&textX, &textY, (unsigned char*)"6", 0, 0xFFFFFFFF, 0, 0, COLOR_MAGENTA, COLOR_WHITE, 1, 0);
        break;
      case 7:
        PrintMini(&textX, &textY, (unsigned char*)"7", 0, 0xFFFFFFFF, 0, 0, COLOR_YELLOW, COLOR_WHITE, 1, 0);
        break;
    }
    
    textY = textY + 25; textX=0;
    PrintMini(&textX, &textY, (unsigned char*)"Description: ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX=0;
    
    //description line breaking:
    strcpy((char*)src, (char*)calevent.description); 
    temptextY = 0;
    temptextX = 0;

    while(*src)
    {
      temptextX = 0;
      src = toksplit(src, ' ', singleword, 1030); //break into words; next word
      //check if printing this word would go off the screen, with fake PrintMini drawing:
      PrintMini(&temptextX, &temptextY, singleword, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0);
      if(temptextX + textX > LCD_WIDTH_PX-6) {
        //time for a new line
        textX=0;
        textY=textY+17;
        PrintMini(&textX, &textY, singleword, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      } else {
        //still fits, print new word normally
        PrintMini(&textX, &textY, singleword, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      }
      //add a space, since it was removed from token
      PrintMini(&textX, &textY, (unsigned char*)" ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    }
    
  
    strcpy((char*)buffer, "  ");
    strcpy((char*)buffer2, "");
    strcat((char*)buffer, (char*)calevent.title);
    PrintXY(1, 1, (char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLUE);
    PrintXY(1, 1, (char*)buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    
    //draw a scrollbar:
    TScrollbar sb;
    sb.I1 = 0;
    sb.I5 = 0;
    sb.indicatormaximum = (textY-scroll);
    sb.indicatorheight = 10*17;
    sb.indicatorpos = -scroll;
    sb.barheight = LCD_HEIGHT_PX-24*2;
    sb.bartop = 24;
    sb.barleft = LCD_WIDTH_PX - 6;
    sb.barwidth = 6;

    Scrollbar(&sb);
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_UP:
        if (scroll < 0) {
          scroll = scroll + 17;
        }
        break;
      case KEY_CTRL_DOWN:
        if (textY > LCD_HEIGHT_PX-24*2) {
          scroll = scroll - 17;
        }
        break;
      case KEY_CTRL_EXIT: inscreen = 0; break;
    }
  }
}
void viewEvents(int y, int m, int d) {
  int key, inscreen = 1, pos=1, scroll=0, numevents=0;
  EventDate thisday;
  thisday.day = d; thisday.month = m; thisday.year = y;  
  numevents = GetSMEMeventsForDate(thisday, SMEM_CALENDAR_FOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* calevents = (CalendarEvent*)alloca(numevents*sizeof(CalendarEvent));
  
  char title[21] = "";
  char buffer[21] = "";
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  numevents = GetSMEMeventsForDate(thisday, SMEM_CALENDAR_FOLDER, calevents);
  int menu = 0;
  while(inscreen) {
    Bdisp_AllClr_VRAM();
    int curevent = 0; //current processing event
    if (numevents>0) {
      while(curevent < numevents) {
        char menuitem[100] = "";
        strcpy(menuitem, "  ");
        strcat(menuitem, (char*)calevents[curevent].title);
        strcat(menuitem, "                     "); //make sure we have a string big enough to have background when item is selected
        if(scroll < curevent+1) PrintXY(1,curevent+2-scroll,(char*)menuitem, (pos == curevent+1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), (calevents[curevent].category <= 6 ? calevents[curevent].category-1 : TEXT_COLOR_YELLOW)); //the text color is determined by the category. category 1 and 7 are black (category 7 would be white, but that's invisible) 
        curevent++;
      }
      //hide 8th item
      PrintXY(1,8,(char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      
      TScrollbar sb;
      sb.I1 = 0;
      sb.I5 = 0;
      sb.indicatormaximum = numevents;
      sb.indicatorheight = 6;
      sb.indicatorpos = scroll;
      sb.barheight = LCD_HEIGHT_PX - 24*3;
      sb.bartop = 24;
      sb.barleft = LCD_WIDTH_PX - 6;
      sb.barwidth = 6;
      Scrollbar(&sb);
    } else {
      PrintXY(6,4,(char*)"  (no events)", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
    if (setting_display_statusbar == 1) DisplayStatusArea();
    strcpy(title, "  Events for ");
    strcpy(buffer, "");
    fillCustomDate(buffer, y, m, d, setting_dateformat);
    strcat(title, buffer);
    PrintXY(1, 1, title, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    
    int iresult;
    if (menu == 0) {
      if(numevents>0) {
        GetFKeyPtr(0x049F, &iresult); // VIEW
        FKey_Display(0, (int*)iresult);
      }
      GetFKeyPtr(0x03B4, &iresult); // INSERT
      FKey_Display(1, (int*)iresult);
      if(numevents>0) {
        GetFKeyPtr(0x0185, &iresult); // EDIT
        FKey_Display(2, (int*)iresult);
        GetFKeyPtr(0x0038, &iresult); // DELETE
        FKey_Display(3, (int*)iresult);
        GetFKeyPtr(0x0104, &iresult); // DEL-ALL
        FKey_Display(4, (int*)iresult);
      }
    } else if (menu == 1) {
      if(numevents>0) {
        GetFKeyPtr(0x038D, &iresult); // COPY
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x04D2, &iresult); // MOVE
        FKey_Display(1, (int*)iresult);
      }
    }
    if(numevents>0) {
      GetFKeyPtr(0x0006, &iresult); // Rotate FKey menu arrow
      FKey_Display(5, (int*)iresult);
    }
    mGetKey(&key);
    CalendarEvent eventtoview;
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(pos == numevents)
        {
          pos = 1;
          scroll = 0;
        }
        else
        {
          pos++;
          if(pos > scroll+(numevents>6 ? 6 : numevents))
            scroll = pos -(numevents>6 ? 6 : numevents);
        }
        break;
      case KEY_CTRL_UP:
        if(pos == 1)
        {
          pos = numevents;
          scroll = pos-(numevents>6 ? 6 : numevents);
        }
        else
        {
          pos--;
          if(pos-1 < scroll)
            scroll = pos -1;
        }
        break;
      case KEY_CTRL_EXE:
        if(numevents>0) { eventtoview = calevents[pos-1]; viewEvent(eventtoview); }
        break;
      case KEY_CTRL_F1:
        if(numevents>0) {
          if (menu == 0) {
            eventtoview = calevents[pos-1]; viewEvent(eventtoview);
          } else if (menu == 1) {
            //COPY Code
            int ey=0, em=0, ed=0;
            int chooseres = chooseCalendarDate(&ey, &em, &ed, (char*) "  Copy Event", (char*)"  To following day:");
            switch(chooseres) {
              case 0: //success
                if(ey == (signed)calevents[pos-1].startdate.year && em == (signed)calevents[pos-1].startdate.month && ed == (signed)calevents[pos-1].startdate.day) {
                  break; //destination date is same as current event date
                }
                calevents[pos-1].startdate.day = ed;
                calevents[pos-1].startdate.month = em;
                calevents[pos-1].startdate.year = ey;
                if(GetSMEMeventsForDate(calevents[pos-1].startdate, SMEM_CALENDAR_FOLDER, calevents)+1 > MAX_DAY_EVENTS) {
                  AUX_DisplayErrorMessage( 0x2E );
                } else {
                  //already checked if passes num limit
                  int res = AddSMEMEvent(calevents[pos-1], CALENDAR_FOLDER);
                  if(res > 0) {
                    MsgBoxPush(4);
                    if (res == 4) { //error on size check
                      PrintXY(3, 2, (char*)"  Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
                    } else {
                      PrintXY(3, 2, (char*)"  Event copy ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
                    }
                    PrintXY(3, 3, (char*)"  Event could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
                    PrintXY(3, 4, (char*)"  be copied.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
                    PrintXY(3, 5, (char*)"     Press:[EXIT]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
                    int key,inscreen=1;
                    while(inscreen) {
                      mGetKey(&key);
                      switch(key)
                      {
                        case KEY_CTRL_EXIT:
                          inscreen=0;
                          break;
                      }
                    }
                    MsgBoxPop();
                  }
                }
                //reload events, because we modified calevents[pos-1] startdate.
                numevents = GetSMEMeventsForDate(thisday, SMEM_CALENDAR_FOLDER, calevents);
                pos = 1; //we don't know if current menu position is still valid as event count was changed.
                curbufmonth = 0; //force calendar event counts to refresh
                break;
            }
          }
        }
        break;
      case KEY_CTRL_F2:
        if (menu == 0) {
          if(numevents+1 > MAX_DAY_EVENTS) {
            AUX_DisplayErrorMessage( 0x2E );
          } else {
            addEventUI(y, m, d);
            return;
          }
        } else if (menu == 1) {
          if(numevents>0) {
            //MOVE Code
            int ey=0, em=0, ed=0;
            int chooseres = chooseCalendarDate(&ey, &em, &ed, (char*) "  Move Event", (char*)"  To following day:");
            switch(chooseres) {
              case 0: //success
                if(ey == (signed)calevents[pos-1].startdate.year && em == (signed)calevents[pos-1].startdate.month && ed == (signed)calevents[pos-1].startdate.day) {
                  break; //destination date is same as current event date
                }
                EventDate oldstartdate;
                oldstartdate.day = calevents[pos-1].startdate.day;
                oldstartdate.month = calevents[pos-1].startdate.month;
                oldstartdate.year = calevents[pos-1].startdate.year;
                calevents[pos-1].startdate.day = ed;
                calevents[pos-1].startdate.month = em;
                calevents[pos-1].startdate.year = ey;
                if(GetSMEMeventsForDate(calevents[pos-1].startdate, SMEM_CALENDAR_FOLDER, calevents)+1 > MAX_DAY_EVENTS) {
                  AUX_DisplayErrorMessage( 0x2E );
                } else {
                  EditSMEMEvent(oldstartdate, pos-1, CALENDAR_FOLDER, calevents[pos-1]);
                }
                //reload events, because we modified calevents[pos-1] startdate. EDIT: NO NEED TO, because we're going to return.
                numevents = GetSMEMeventsForDate(thisday, SMEM_CALENDAR_FOLDER, calevents);
                curbufmonth = 0; //force calendar event counts to refresh
                break;
            }
          }
        }
        break;
      case KEY_CTRL_F3:
        if (menu == 0) {        
          if(numevents>0) {
            editEventUI(calevents[pos-1], pos-1);
            return;
          }    
        }
        break;
      case KEY_CTRL_F4:
        if (menu == 0) {
          if(numevents>0) {
            delEventUI(y, m, d, pos-1);
            return;
          }
        }
        break;
      case KEY_CTRL_F5:
        if (menu == 0) {
          if(numevents>0) {
            delAllEventUI(y, m, d);
            return;
          }
        }
        break;
      case KEY_CTRL_F6:
        if (menu == 0) {
          if(numevents>0) {
            menu = 1;
          }
        } else {
          menu = 0;
        }
        break;
      case KEY_CTRL_FORMAT:
        //the "FORMAT" key is used in many places in the OS to format e.g. the color of a field,
        //so on this add-in it is used to change the category (color) of a task/calendar event.
        changeEventCategory(calevents[pos-1], pos-1);
        return;
        break;
      case KEY_CTRL_DEL:
        if(numevents>0) {
          delEventUI(y, m, d, pos-1);
          return;
        }
        break;
      case KEY_CTRL_EXIT: inscreen = 0; break;
      default:
        if(key && key < 30000)
        {
          //user pressed a char key. start adding an event
          if(numevents+1 > MAX_DAY_EVENTS) {
            //no need to show error, because if user presses key on accident he/she may not be able to understand why the error appeared
          } else {
            addEventUI(y, m, d, key);
            return;
          }
        }
        break;
    }
  }
} 
void calendarScreen() {
  int today_y = ((*RYRCNT >> 12) & 0b1111)*1000 + ((*RYRCNT >> 8) & 0b1111)*100 + ((*RYRCNT >> 4) & 0b1111)*10 + (*RYRCNT & 0b1111);
  int today_m = ((*RMONCNT >> 4) & 0b1)*10 + (*RMONCNT & 0b1111);
  int today_d = ((*RDAYCNT >> 4) & 0b11)*10 + (*RDAYCNT & 0b1111);
  int y = today_y;
  int m = today_m;
  int d = today_d;
  bool running = true;
  int menu = 1;
  int iresult;

  while (running)
  {
    if(y>9999||y<0) { y=today_y; m=today_m; d=today_d; } //protection: reset to today's date if somehow we're trying to access the calendar for an year after 9999, or before 0.
    drawCalendar(y,m,d, setting_show_events_count_on_calendar);
    switch (menu)
    {
    case 1:
        GetFKeyPtr(0x01FC, &iresult); // JUMP
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x049F, &iresult); // VIEW
        FKey_Display(1, (int*)iresult);
        GetFKeyPtr(0x03B4, &iresult); // INSERT
        FKey_Display(2, (int*)iresult);
        GetFKeyPtr(0x0104, &iresult); // DEL-ALL
        FKey_Display(3, (int*)iresult);
        break;
    case 2:
        GetFKeyPtr(0x0408, &iresult); // |<<
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0409, &iresult); // <<
        FKey_Display(1, (int*)iresult);
        GetFKeyPtr(0x040B, &iresult); // >>
        FKey_Display(2, (int*)iresult);
        GetFKeyPtr(0x040C, &iresult); // >>|
        FKey_Display(3, (int*)iresult);
        GetFKeyPtr(0x0238, &iresult); // ORIGINAL
        FKey_Display(4, (int*)iresult);
        GetFKeyPtr(0x015F, &iresult); // DATE
        FKey_Display(5, (int*)iresult);
        break;
    }
    int key;
    mGetKey(&key);
    int ny=0, nm=0, nd=0;
    switch(key) {
      case KEY_CTRL_F1:
        if (menu == 1) { menu = 2; }
        else if (menu == 2) { if (y > 0) y--; }
        break;
      case KEY_CTRL_F2:
        if (menu == 2) {
          if (y != 0)
              m--;
          else { if (m!=1) m--; } 
          if (m == 0)
          {
              m = 12;
              y--;
          }
        } else if (menu == 1) {
          viewEvents(y, m, d);
        }
        break;
      case KEY_CTRL_F3:
        if (menu == 2) {
          if (y != 9999)
              m++;
          else { if (m!=12) m++; }
          if (m == 13)
          {
              m = 1;
              y++;
          }
        } else if (menu == 1) {
          if(eventsfordayofcurmonth[d]+1 > MAX_DAY_EVENTS) {
            AUX_DisplayErrorMessage( 0x2E );
          } else {
            addEventUI(y, m, d);
            curbufmonth=0;//force calendar events to reload
          }
        }
        break;
      case KEY_CTRL_F4:
        if (menu == 2) {
          if (y != 9999) y++;
        } else if (menu == 1) {
          delAllEventUI(y, m, d);
        }
        break;
      case KEY_CTRL_F5:
        if (menu == 2) {
          y = today_y;
          m = today_m;
          d = today_d;
        }
        break;       
      case KEY_CTRL_F6:
        if (menu == 2) { if (0 == chooseCalendarDate(&ny, &nm, &nd, (char*)"  Jump to specific date", (char*)"")) { y=ny;m=nm;d=nd; } } //only update calendar if selection was clean
        break;     
      case KEY_CTRL_UP:
        d-=7;
        if (d < 1)
        {
            m--;
            if (m == 0)
            {
                m = 12;
                y--;
            }
            d = monthDays[m-1] + d + ((m == 2 && isLeap(y)) ? 1 : 0);
        }
        break;
      case KEY_CTRL_DOWN:
        d+=7;
        if (d > monthDays[m-1] + ((m == 2 && isLeap(y)) ? 1 : 0))
        {
            d = d - monthDays[m-1] - ((m == 2 && isLeap(y)) ? 1 : 0);
            m++;
            if (m == 13)
            {
                m = 1;
                y++;
            }
        }
        break;
      case KEY_CTRL_LEFT:
        d--;
        if (d == 0)
        {
            m--;
            if (m == 0)
            {
                m = 12;
                y--;
            }
            d = monthDays[m-1]+((m == 2 && isLeap(y)) ? 1 : 0);
        }
        break;
      case KEY_CTRL_RIGHT:
        d++;
        if (d > monthDays[m-1] + ((m == 2 && isLeap(y)) ? 1 : 0))
        {
            d = 1;
            m++;
            if (m == 13)
            {
                m = 1;
                y++;
            }
        }
        break;
      case KEY_CTRL_EXE:
        viewEvents(y, m, d);
        break;
      case KEY_CTRL_EXIT:
        if (menu == 2) { menu = 1; } else { return; }
        break;
    }
    if (menu == 2) {
      if (d > monthDays[m-1] + ((m == 2 && isLeap(y)) ? 1 : 0))
      {
          d = monthDays[m-1]+((m == 2 && isLeap(y)) ? 1 : 0);
      }
    } 
  }
}

// --- start of stopwatches ---
#define STOPWATCH1FILE (unsigned char*)"Stpwatc1"
#define STOPWATCH2FILE (unsigned char*)"Stpwatc2"
#define STOPWATCH3FILE (unsigned char*)"Stpwatc3"
#define STOPWATCH4FILE (unsigned char*)"Stpwatc4"
#define STOPWATCH5FILE (unsigned char*)"Stpwatc5"
#define STOPWATCH6FILE (unsigned char*)"Stpwatc6"

typedef struct {
  long long int starttime; //time stamp at which stopwatch was put to run (increases when timer is paused)
  long long int duration; //duration, not needed in stopwatches (it is calculated as it is needed). kept here so it's easier to resue code from timers
  long long int laststop; //time stamp at which the stopwarch was last paused
  long long int state; //0 if stopped, 1 if running, 2 if clear 
} stopwatch;

stopwatch stopwatch1;
stopwatch stopwatch2;
stopwatch stopwatch3;
stopwatch stopwatch4;
stopwatch stopwatch5;
stopwatch stopwatch6;

// Routines for saving and loading timers
void SaveStopwatch(int stopwatchno) {
  unsigned char* file;

  int createResult = MCS_CreateDirectory( DIRNAME );
  long long int buffer[6];
  switch(stopwatchno) {
    case 1: file = STOPWATCH1FILE; buffer[0] = stopwatch1.starttime; buffer[1] = stopwatch1.duration; buffer[2] = stopwatch1.laststop; buffer[3] = stopwatch1.state; break;
    case 2: file = STOPWATCH2FILE; buffer[0] = stopwatch2.starttime; buffer[1] = stopwatch2.duration; buffer[2] = stopwatch2.laststop; buffer[3] = stopwatch2.state; break;
    case 3: file = STOPWATCH3FILE; buffer[0] = stopwatch3.starttime; buffer[1] = stopwatch3.duration; buffer[2] = stopwatch3.laststop; buffer[3] = stopwatch3.state; break;
    case 4: file = STOPWATCH4FILE; buffer[0] = stopwatch4.starttime; buffer[1] = stopwatch4.duration; buffer[2] = stopwatch4.laststop; buffer[3] = stopwatch4.state; break;
    case 5: file = STOPWATCH5FILE; buffer[0] = stopwatch5.starttime; buffer[1] = stopwatch5.duration; buffer[2] = stopwatch5.laststop; buffer[3] = stopwatch5.state; break;
    case 6: file = STOPWATCH6FILE; buffer[0] = stopwatch6.starttime; buffer[1] = stopwatch6.duration; buffer[2] = stopwatch6.laststop; buffer[3] = stopwatch6.state; break;
    default: file = STOPWATCH1FILE; buffer[0] = stopwatch1.starttime; buffer[1] = stopwatch1.duration; buffer[2] = stopwatch1.laststop; buffer[3] = stopwatch1.state; break;
  }
  if(createResult != 0) // Check directory existence
  { // directory already exists, so delete the exiting file that may be there
    MCSDelVar2(DIRNAME, file);
  } 

  MCSPutVar2(DIRNAME, file, sizeof(buffer), buffer);
}

void LoadStopwatch(int stopwatchno) {
  int size;
  long long int buffer[6];
  unsigned char* file;
  switch(stopwatchno) {
    case 1: file = STOPWATCH1FILE; break;
    case 2: file = STOPWATCH2FILE; break;
    case 3: file = STOPWATCH3FILE; break;
    case 4: file = STOPWATCH4FILE; break;
    case 5: file = STOPWATCH5FILE; break;
    case 6: file = STOPWATCH6FILE; break;
    default: file = STOPWATCH1FILE; break;
  }
  MCSGetDlen2(DIRNAME, file, &size);
  if (size == 0) {
    //still no file, let's initializer this stopwatch's data and return
    switch(stopwatchno) {
      case 1: stopwatch1.starttime = 0; stopwatch1.duration = 0; stopwatch1.laststop = 0; stopwatch1.state = 2; break;
      case 2: stopwatch2.starttime = 0; stopwatch2.duration = 0; stopwatch2.laststop = 0; stopwatch2.state = 2; break;
      case 3: stopwatch3.starttime = 0; stopwatch3.duration = 0; stopwatch3.laststop = 0; stopwatch3.state = 2; break;
      case 4: stopwatch4.starttime = 0; stopwatch4.duration = 0; stopwatch4.laststop = 0; stopwatch4.state = 2; break;
      case 5: stopwatch5.starttime = 0; stopwatch5.duration = 0; stopwatch5.laststop = 0; stopwatch5.state = 2; break;
      case 6: stopwatch6.starttime = 0; stopwatch6.duration = 0; stopwatch6.laststop = 0; stopwatch6.state = 2; break;
      default: stopwatch1.starttime = 0; stopwatch1.duration = 0; stopwatch1.laststop = 0; stopwatch1.state = 2; break;
    }
    return;  
  }

  MCSGetData1(0, sizeof(buffer), buffer);
  
  switch(stopwatchno) {
    case 1: stopwatch1.starttime = buffer[0]; stopwatch1.duration = buffer[1]; stopwatch1.laststop = buffer[2]; stopwatch1.state = buffer[3]; break;
    case 2: stopwatch2.starttime = buffer[0]; stopwatch2.duration = buffer[1]; stopwatch2.laststop = buffer[2]; stopwatch2.state = buffer[3]; break;
    case 3: stopwatch3.starttime = buffer[0]; stopwatch3.duration = buffer[1]; stopwatch3.laststop = buffer[2]; stopwatch3.state = buffer[3]; break;
    case 4: stopwatch4.starttime = buffer[0]; stopwatch4.duration = buffer[1]; stopwatch4.laststop = buffer[2]; stopwatch4.state = buffer[3]; break;
    case 5: stopwatch5.starttime = buffer[0]; stopwatch5.duration = buffer[1]; stopwatch5.laststop = buffer[2]; stopwatch5.state = buffer[3]; break;
    case 6: stopwatch6.starttime = buffer[0]; stopwatch6.duration = buffer[1]; stopwatch6.laststop = buffer[2]; stopwatch6.state = buffer[3]; break;
    default: stopwatch1.starttime = buffer[0]; stopwatch1.duration = buffer[1]; stopwatch1.laststop = buffer[2]; stopwatch1.state = buffer[3]; break;
  }
}
void LoadAllStopwatches() {
  LoadStopwatch(1);
  LoadStopwatch(2);
  LoadStopwatch(3);
  LoadStopwatch(4);
  LoadStopwatch(5);
  LoadStopwatch(6);
}
void SaveAllStopwatches() {
  SaveStopwatch(1);
  SaveStopwatch(2);
  SaveStopwatch(3);
  SaveStopwatch(4);
  SaveStopwatch(5);
  SaveStopwatch(6);
}
long long int currentUnixTime()
{
  long long int year = ((*RYRCNT >> 12) & 0b1111)*1000 + ((*RYRCNT >> 8) & 0b1111)*100 + ((*RYRCNT >> 4) & 0b1111)*10 + (*RYRCNT & 0b1111);
  long long int month = ((*RMONCNT & 0b10000)>>4)*10 + (*RMONCNT & 0b1111);
  long long int day = ((*RDAYCNT >> 4) & 0b11)*10 + (*RDAYCNT & 0b1111);
  long long int hour = bcd_to_2digit(RHRCNT);
  long long int minute = bcd_to_2digit(RMINCNT);
  long long int second = bcd_to_2digit(RSECCNT);
  unsigned int fhour=0,fminute=0,fsecond=0,millisecond=0;
  RTC_GetTime( &fhour, &fminute, &fsecond, &millisecond );
  return DateTime2Unix(year, month, day, hour, minute, second, millisecond);
}
void startStopwatch(int stopwatchno) {
  switch(stopwatchno) {
    case 1: stopwatch1.starttime = stopwatch1.starttime+currentUnixTime()-stopwatch1.laststop; stopwatch1.state = 1; break;
    case 2: stopwatch2.starttime = stopwatch2.starttime+currentUnixTime()-stopwatch2.laststop; stopwatch2.state = 1; break;
    case 3: stopwatch3.starttime = stopwatch3.starttime+currentUnixTime()-stopwatch3.laststop; stopwatch3.state = 1; break;
    case 4: stopwatch4.starttime = stopwatch4.starttime+currentUnixTime()-stopwatch4.laststop; stopwatch4.state = 1; break;
    case 5: stopwatch5.starttime = stopwatch5.starttime+currentUnixTime()-stopwatch5.laststop; stopwatch5.state = 1; break;
    case 6: stopwatch6.starttime = stopwatch6.starttime+currentUnixTime()-stopwatch6.laststop; stopwatch6.state = 1; break;
    default: stopwatch1.starttime = stopwatch1.starttime+currentUnixTime()-stopwatch1.laststop; stopwatch1.state = 1; break;
  }
  SaveStopwatch(stopwatchno);
}
void stopStopwatch(int stopwatchno) {
  switch(stopwatchno) {
    case 1: stopwatch1.laststop = currentUnixTime(); stopwatch1.state = 0; break;
    case 2: stopwatch2.laststop = currentUnixTime(); stopwatch2.state = 0; break;
    case 3: stopwatch3.laststop = currentUnixTime(); stopwatch3.state = 0; break;
    case 4: stopwatch4.laststop = currentUnixTime(); stopwatch4.state = 0; break;
    case 5: stopwatch5.laststop = currentUnixTime(); stopwatch5.state = 0; break;
    case 6: stopwatch6.laststop = currentUnixTime(); stopwatch6.state = 0; break;
    default: stopwatch1.laststop = currentUnixTime(); stopwatch1.state = 0; break;
  }
  SaveStopwatch(stopwatchno);
}
void clearStopwatch(int stopwatchno) {
  switch(stopwatchno) {
    case 1: stopwatch1.state = 2; break;
    case 2: stopwatch2.state = 2; break;
    case 3: stopwatch3.state = 2; break;
    case 4: stopwatch4.state = 2; break;
    case 5: stopwatch5.state = 2; break;
    case 6: stopwatch6.state = 2; break;
    default: stopwatch1.state = 2; break;
  }
  SaveStopwatch(stopwatchno);
}
void setStopwatch(int stopwatchno) {
  //stopwatch starts when it is set
  switch(stopwatchno) {
    case 1: stopwatch1.starttime = currentUnixTime(); stopwatch1.laststop = 0; stopwatch1.duration = 0; stopwatch1.state = 1; break;
    case 2: stopwatch2.starttime = currentUnixTime(); stopwatch2.laststop = 0; stopwatch2.duration = 0; stopwatch2.state = 1; break;
    case 3: stopwatch3.starttime = currentUnixTime(); stopwatch3.laststop = 0; stopwatch3.duration = 0; stopwatch3.state = 1; break;
    case 4: stopwatch4.starttime = currentUnixTime(); stopwatch4.laststop = 0; stopwatch4.duration = 0; stopwatch4.state = 1; break;
    case 5: stopwatch5.starttime = currentUnixTime(); stopwatch5.laststop = 0; stopwatch5.duration = 0; stopwatch5.state = 1; break;
    case 6: stopwatch6.starttime = currentUnixTime(); stopwatch6.laststop = 0; stopwatch6.duration = 0; stopwatch6.state = 1; break;
    default: stopwatch1.starttime = currentUnixTime(); stopwatch1.laststop = 0; stopwatch1.duration = 0; stopwatch1.state = 1; break;
  }
  SaveStopwatch(stopwatchno);
}
void formatStopwatchString(int stopwatchno, char* string)
{
  long long int unixdiff;
  switch(stopwatchno) {
    case 1:
    default:
      strcpy(string, "  S1:");
      if(stopwatch1.state == 2) { strcat(string, "                       "); return; } //nothing else to add, stopwatch is clear
      else if(stopwatch1.state == 0) {
        //diff will be calculated in a different way, so that it is always stopped
        unixdiff = stopwatch1.laststop-stopwatch1.starttime;
      } else {
        unixdiff = currentUnixTime()-stopwatch1.starttime;
      }
      break;
    case 2:
      strcpy(string, "  S2:");
      if(stopwatch2.state == 2) { strcat(string, "                       "); return; }
      else if(stopwatch2.state == 0) {
        unixdiff = stopwatch2.laststop-stopwatch2.starttime;
      } else {
        unixdiff = currentUnixTime()-stopwatch2.starttime;
      }
      break;
    case 3:
      strcpy(string, "  S3:");
      if(stopwatch3.state == 2) { strcat(string, "                       "); return; }
      else if(stopwatch3.state == 0) {
        unixdiff = stopwatch3.laststop-stopwatch3.starttime;
      } else {
        unixdiff = currentUnixTime()-stopwatch3.starttime;
      }
      break;
    case 4:
      strcpy(string, "  S4:");
      if(stopwatch4.state == 2) { strcat(string, "                       "); return; }
      else if(stopwatch4.state == 0) {
        unixdiff = stopwatch4.laststop-stopwatch4.starttime;
      } else {
        unixdiff = currentUnixTime()-stopwatch4.starttime;
      }
      break;
    case 5:
      strcpy(string, "  S5:");
      if(stopwatch5.state == 2) { strcat(string, "                       "); return; }
      else if(stopwatch5.state == 0) {
        unixdiff = stopwatch5.laststop-stopwatch5.starttime;
      } else {
        unixdiff = currentUnixTime()-stopwatch5.starttime;
      }
      break;
    case 6:
      strcpy(string, "  S6:");
      if(stopwatch6.state == 2) { strcat(string, "                       "); return; }
      else if(stopwatch6.state == 0) {
        unixdiff = stopwatch6.laststop-stopwatch6.starttime;
      } else {
        unixdiff = currentUnixTime()-stopwatch6.starttime;
      }
      break;
  }
  char buffer[20];
  long long int days=0,hours=0,minutes=0,seconds=0, milliseconds=0;

  milliseconds=unixdiff;  
  seconds = milliseconds / 1000;
  milliseconds %= 1000;
  minutes = seconds / 60;
  seconds %= 60;
  hours = minutes / 60;
  minutes %= 60;
  days = hours / 24;
  hours %= 24;
  

  if (days) {
    itoa(days, (unsigned char*)buffer);
    strcat((char*)string, buffer);
    strcat((char*)string, "d");
    strcat((char*)string, " ");
  }
  
  itoa(hours, (unsigned char*)buffer);
  if (hours < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(minutes, (unsigned char*)buffer);
  if (minutes < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(seconds, (unsigned char*)buffer);
  if (seconds < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  
  strcat((char*)string, ".");
  itoa(milliseconds, (unsigned char*)buffer);
  strcat((char*)string, buffer);

  strcat(string, "                       "); //make sure we have enough spaces to have a background when menu item is selected
}

void drawStopwatchAppropriateFKeys(int stopwatchno) {
  int iresult;
  // stop icon - 0031
  // CLEAR - 0149
  // play icon - 040A
  // SET - 0010
  switch (stopwatchno) {
    case 1:
    default:
      if (stopwatch1.state == 2) {
        //stopwatch is clear, don't show CLEAR but SET
        //and don't show start/stop button
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (stopwatch1.state == 1) {
        //stopwatch is running, show stop button and CLEAR
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        //stopwatch is stopped, show play button and CLEAR
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 2:
      if (stopwatch2.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (stopwatch2.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 3:
      if (stopwatch3.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (stopwatch3.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 4:
      if (stopwatch4.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (stopwatch4.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 5:
      if (stopwatch5.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (stopwatch5.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 6:
      if (stopwatch6.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (stopwatch6.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
  }
}

void processStopwatchReturnKey(int stopwatchno) { /*does nothing for now*/ }
void processStopwatchF1key(int stopwatchno) {
  //stop, start or nothing?
  switch (stopwatchno) {
    case 1:
    default:
      if (stopwatch1.state == 1) {
        stopStopwatch(stopwatchno);
      } else if (stopwatch1.state == 0) {
        startStopwatch(stopwatchno);
      }
      //do nothing when stopwatch is clear
      break;
    case 2:
      if (stopwatch2.state == 1) {
        stopStopwatch(stopwatchno);
      } else if (stopwatch2.state == 0) {
        startStopwatch(stopwatchno);
      }
      break;
    case 3:
      if (stopwatch3.state == 1) {
        stopStopwatch(stopwatchno);
      } else if (stopwatch3.state == 0) {
        startStopwatch(stopwatchno);
      }
      break;
    case 4:
      if (stopwatch4.state == 1) {
        stopStopwatch(stopwatchno);
      } else if (stopwatch4.state == 0) {
        startStopwatch(stopwatchno);
      }
      break;
    case 5:
      if (stopwatch5.state == 1) {
        stopStopwatch(stopwatchno);
      } else if (stopwatch5.state == 0) {
        startStopwatch(stopwatchno);
      }
      break;
    case 6:
      if (stopwatch6.state == 1) {
        stopStopwatch(stopwatchno);
      } else if (stopwatch6.state == 0) {
        startStopwatch(stopwatchno);
      }
      break;
  }
}
void processStopwatchF2key(int stopwatchno) {
  //set or clear?
  switch (stopwatchno) {
    case 1:
    default:
      if (stopwatch1.state == 2) {
        setStopwatch(stopwatchno);
      } else {
        clearStopwatch(stopwatchno);
      }
      break;
    case 2:
      if (stopwatch2.state == 2) {
        setStopwatch(stopwatchno);
      } else {
        clearStopwatch(stopwatchno);
      }
      break;
    case 3:
      if (stopwatch3.state == 2) {
        setStopwatch(stopwatchno);
      } else {
        clearStopwatch(stopwatchno);
      }
      break;
    case 4:
      if (stopwatch4.state == 2) {
        setStopwatch(stopwatchno);
      } else {
        clearStopwatch(stopwatchno);
      }
      break;
    case 5:
      if (stopwatch5.state == 2) {
        setStopwatch(stopwatchno);
      } else {
        clearStopwatch(stopwatchno);
      }
      break;
    case 6:
      if (stopwatch6.state == 2) {
        setStopwatch(stopwatchno);
      } else {
        clearStopwatch(stopwatchno);
      }
      break;
  }
}
void drawStopwatchesMenu(int pos, int scroll, int numitems)
{  
  Bdisp_AllClr_VRAM();
  PrintXY(1, 1, (char*)"  Stopwatches", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int textX=18*12; int textY=4;
  //escape sequence doesn't want to be together with number 3.91:
  PrintMini(&textX, &textY, (unsigned char*)"\xE5\xBE", 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"3.91 ms", 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);
  //display stopwatches  
  char stopwatch1string[100]; formatStopwatchString(1, stopwatch1string);
  char stopwatch2string[100]; formatStopwatchString(2, stopwatch2string);
  char stopwatch3string[100]; formatStopwatchString(3, stopwatch3string);
  char stopwatch4string[100]; formatStopwatchString(4, stopwatch4string);
  char stopwatch5string[100]; formatStopwatchString(5, stopwatch5string);
  char stopwatch6string[100]; formatStopwatchString(6, stopwatch6string);  
  
  if(scroll < 1) PrintXY(1,2,(char*)stopwatch1string, (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLUE);
  if(scroll < 2) PrintXY(1,3-scroll,(char*)stopwatch2string, (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_RED);
  if(scroll < 3) PrintXY(1,4-scroll,(char*)stopwatch3string, (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_GREEN);
  if(scroll < 4) PrintXY(1,5-scroll,(char*)stopwatch4string, (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_PURPLE);
  if(scroll < 5) PrintXY(1,6-scroll,(char*)stopwatch5string, (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 6) PrintXY(1,7-scroll,(char*)stopwatch6string, (pos == 6 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLUE);

  drawStopwatchAppropriateFKeys(pos);
  
  if (setting_display_statusbar == 1) DisplayStatusArea();
}
void stopwatchScreen() {
  short unsigned int key; int pos = 1, scroll = 0, inscreen = 1, keyCol = 0, keyRow = 0;
  int numitems = 6; //total number of items in menu

  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  
  while(inscreen)
  {
    drawStopwatchesMenu(pos, scroll, numitems);
    Bdisp_PutDisp_DD();
    if (0 != GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key) ) {
      key = PRGM_GetKey();
      switch(key)
      {
        case KEY_PRGM_DOWN:
          if(pos == numitems)
          {
            pos = 1;
            scroll = 0;
          }
          else
          {
            pos++;
            if(pos > scroll+6)
              scroll = pos -6;
          }
          break;
        case KEY_PRGM_UP:
          if(pos == 1)
          {
            pos = numitems;
            scroll = pos-6;
          }
          else
          {
            pos--;
            if(pos-1 < scroll)
              scroll = pos -1;
          }
          break;
        case KEY_PRGM_RETURN:
          processStopwatchReturnKey(pos);
          SaveAllStopwatches(); //just to be sure
          break;
        case KEY_PRGM_F1:
          processStopwatchF1key(pos);
          SaveAllStopwatches(); //just to be sure
          break;
        case KEY_PRGM_F2:
          processStopwatchF2key(pos);
          SaveAllStopwatches(); //just to be sure
          break;
        case KEY_PRGM_EXIT: SaveAllStopwatches(); inscreen = 0; break;
      }
    }
  }
}
// --- start of timers ---
#define TIMER1FILE (unsigned char*)"Timer1"
#define TIMER2FILE (unsigned char*)"Timer2"
#define TIMER3FILE (unsigned char*)"Timer3"
#define TIMER4FILE (unsigned char*)"Timer4"
#define TIMER5FILE (unsigned char*)"Timer5"
#define TIMER6FILE (unsigned char*)"Timer6"

typedef struct {
  long long int starttime; //time stamp at which timer was put to run
  long long int duration; //duration, in seconds (increases when timer is paused)
  long long int laststop; //time stamp at which the timer was last paused
  long long int state; //0 if stopped, 1 if running, 2 if clear 
} timer;

timer timer1;
timer timer2;
timer timer3;
timer timer4;
timer timer5;
timer timer6;

// Routines for saving and loading timers
void SaveTimer(int timerno) {
  unsigned char* file;

  int createResult = MCS_CreateDirectory( DIRNAME );
  long long int buffer[6];
  switch(timerno) {
    case 1: file = TIMER1FILE; buffer[0] = timer1.starttime; buffer[1] = timer1.duration; buffer[2] = timer1.laststop; buffer[3] = timer1.state; break;
    case 2: file = TIMER2FILE; buffer[0] = timer2.starttime; buffer[1] = timer2.duration; buffer[2] = timer2.laststop; buffer[3] = timer2.state; break;
    case 3: file = TIMER3FILE; buffer[0] = timer3.starttime; buffer[1] = timer3.duration; buffer[2] = timer3.laststop; buffer[3] = timer3.state; break;
    case 4: file = TIMER4FILE; buffer[0] = timer4.starttime; buffer[1] = timer4.duration; buffer[2] = timer4.laststop; buffer[3] = timer4.state; break;
    case 5: file = TIMER5FILE; buffer[0] = timer5.starttime; buffer[1] = timer5.duration; buffer[2] = timer5.laststop; buffer[3] = timer5.state; break;
    case 6: file = TIMER6FILE; buffer[0] = timer6.starttime; buffer[1] = timer6.duration; buffer[2] = timer6.laststop; buffer[3] = timer6.state; break;
    default: file = TIMER1FILE; buffer[0] = timer1.starttime; buffer[1] = timer1.duration; buffer[2] = timer1.laststop; buffer[3] = timer1.state; break;
  }
  if(createResult != 0) // Check directory existence
  { // directory already exists, so delete the exiting file that may be there
    MCSDelVar2(DIRNAME, file);
  } 

  MCSPutVar2(DIRNAME, file, sizeof(buffer), buffer);
}

void LoadTimer(int timerno) {
  int size;
  long long int buffer[6];
  unsigned char* file;
  switch(timerno) {
    case 1: file = TIMER1FILE; break;
    case 2: file = TIMER2FILE; break;
    case 3: file = TIMER3FILE; break;
    case 4: file = TIMER4FILE; break;
    case 5: file = TIMER5FILE; break;
    case 6: file = TIMER6FILE; break;
    default: file = TIMER1FILE; break;
  }
  MCSGetDlen2(DIRNAME, file, &size);
  if (size == 0) {
    //still no file, let's initializer this timer's data and return
    switch(timerno) {
      case 1: timer1.starttime = 0; timer1.duration = 0; timer1.laststop = 0; timer1.state = 2; break;
      case 2: timer2.starttime = 0; timer2.duration = 0; timer2.laststop = 0; timer2.state = 2; break;
      case 3: timer3.starttime = 0; timer3.duration = 0; timer3.laststop = 0; timer3.state = 2; break;
      case 4: timer4.starttime = 0; timer4.duration = 0; timer4.laststop = 0; timer4.state = 2; break;
      case 5: timer5.starttime = 0; timer5.duration = 0; timer5.laststop = 0; timer5.state = 2; break;
      case 6: timer6.starttime = 0; timer6.duration = 0; timer6.laststop = 0; timer6.state = 2; break;
      default: timer1.starttime = 0; timer1.duration = 0; timer1.laststop = 0; timer1.state = 2; break;
    }
    return;  
  }

  MCSGetData1(0, sizeof(buffer), buffer);
  
  switch(timerno) {
    case 1: timer1.starttime = buffer[0]; timer1.duration = buffer[1]; timer1.laststop = buffer[2]; timer1.state = buffer[3]; break;
    case 2: timer2.starttime = buffer[0]; timer2.duration = buffer[1]; timer2.laststop = buffer[2]; timer2.state = buffer[3]; break;
    case 3: timer3.starttime = buffer[0]; timer3.duration = buffer[1]; timer3.laststop = buffer[2]; timer3.state = buffer[3]; break;
    case 4: timer4.starttime = buffer[0]; timer4.duration = buffer[1]; timer4.laststop = buffer[2]; timer4.state = buffer[3]; break;
    case 5: timer5.starttime = buffer[0]; timer5.duration = buffer[1]; timer5.laststop = buffer[2]; timer5.state = buffer[3]; break;
    case 6: timer6.starttime = buffer[0]; timer6.duration = buffer[1]; timer6.laststop = buffer[2]; timer6.state = buffer[3]; break;
    default: timer1.starttime = buffer[0]; timer1.duration = buffer[1]; timer1.laststop = buffer[2]; timer1.state = buffer[3]; break;
  }
}
void LoadAllTimers() {
  LoadTimer(1);
  LoadTimer(2);
  LoadTimer(3);
  LoadTimer(4);
  LoadTimer(5);
  LoadTimer(6);
}
void SaveAllTimers() {
  SaveTimer(1);
  SaveTimer(2);
  SaveTimer(3);
  SaveTimer(4);
  SaveTimer(5);
  SaveTimer(6);
}

void startTimer(int timerno) {
  switch(timerno) {
    case 1: timer1.duration = timer1.duration+currentUnixTime()-timer1.laststop; timer1.state = 1; break;
    case 2: timer2.duration = timer2.duration+currentUnixTime()-timer2.laststop; timer2.state = 1; break;
    case 3: timer3.duration = timer3.duration+currentUnixTime()-timer3.laststop; timer3.state = 1; break;
    case 4: timer4.duration = timer4.duration+currentUnixTime()-timer4.laststop; timer4.state = 1; break;
    case 5: timer5.duration = timer5.duration+currentUnixTime()-timer5.laststop; timer5.state = 1; break;
    case 6: timer6.duration = timer6.duration+currentUnixTime()-timer6.laststop; timer6.state = 1; break;
    default: timer1.duration = timer1.duration+currentUnixTime()-timer1.laststop; timer1.state = 1; break;
  }
  SaveTimer(timerno);
}
void stopTimer(int timerno) {
  switch(timerno) {
    case 1: timer1.laststop = currentUnixTime(); timer1.state = 0; break;
    case 2: timer2.laststop = currentUnixTime(); timer2.state = 0; break;
    case 3: timer3.laststop = currentUnixTime(); timer3.state = 0; break;
    case 4: timer4.laststop = currentUnixTime(); timer4.state = 0; break;
    case 5: timer5.laststop = currentUnixTime(); timer5.state = 0; break;
    case 6: timer6.laststop = currentUnixTime(); timer6.state = 0; break;
    default: timer1.laststop = currentUnixTime(); timer1.state = 0; break;
  }
  SaveTimer(timerno);
}
void clearTimer(int timerno) {
  switch(timerno) {
    case 1: timer1.state = 2; break;
    case 2: timer2.state = 2; break;
    case 3: timer3.state = 2; break;
    case 4: timer4.state = 2; break;
    case 5: timer5.state = 2; break;
    case 6: timer6.state = 2; break;
    default: timer1.state = 2; break;
  }
  SaveTimer(timerno);
}
void setTimer(int timerno, long long int duration) {
  //timer starts when it is set
  switch(timerno) {
    case 1: timer1.starttime = currentUnixTime(); timer1.laststop = 0; timer1.duration = duration; timer1.state = 1; break;
    case 2: timer2.starttime = currentUnixTime(); timer2.laststop = 0; timer2.duration = duration; timer2.state = 1; break;
    case 3: timer3.starttime = currentUnixTime(); timer3.laststop = 0; timer3.duration = duration; timer3.state = 1; break;
    case 4: timer4.starttime = currentUnixTime(); timer4.laststop = 0; timer4.duration = duration; timer4.state = 1; break;
    case 5: timer5.starttime = currentUnixTime(); timer5.laststop = 0; timer5.duration = duration; timer5.state = 1; break;
    case 6: timer6.starttime = currentUnixTime(); timer6.laststop = 0; timer6.duration = duration; timer6.state = 1; break;
    default: timer1.starttime = currentUnixTime(); timer1.laststop = 0; timer1.duration = duration; timer1.state = 1; break;
  }
  SaveTimer(timerno);
}
void formatTimerString(int timerno, char* string)
{
  long long int unixtime = currentUnixTime();
  long long int unixdiff;
  switch(timerno) {
    case 1:
    default:
      strcpy(string, "  T1:");
      if(timer1.state == 2) { strcat(string, "                       "); return; } //nothing else to add, timer is clear
      else if(timer1.state == 0) {
        //diff will be calculated in a different way, so that it is always stopped
        unixdiff = timer1.starttime+timer1.duration-timer1.laststop;
      } else {
        unixdiff = timer1.starttime+timer1.duration-unixtime;
      }
      break;
    case 2:
      strcpy(string, "  T2:");
      if(timer2.state == 2) { strcat(string, "                       "); return; }
      else if(timer2.state == 0) {
        unixdiff = timer2.starttime+timer2.duration-timer2.laststop;
      } else {
        unixdiff = timer2.starttime+timer2.duration-unixtime;
      }
      break;
    case 3:
      strcpy(string, "  T3:");
      if(timer3.state == 2) { strcat(string, "                       "); return; }
      else if(timer3.state == 0) {
        unixdiff = timer3.starttime+timer3.duration-timer3.laststop;
      } else {
        unixdiff = timer3.starttime+timer3.duration-unixtime;
      }
      break;
    case 4:
      strcpy(string, "  T4:");
      if(timer4.state == 2) { strcat(string, "                       "); return; }
      else if(timer4.state == 0) {
        unixdiff = timer4.starttime+timer4.duration-timer4.laststop;
      } else {
        unixdiff = timer4.starttime+timer4.duration-unixtime;
      }
      break;
    case 5:
      strcpy(string, "  T5:");
      if(timer5.state == 2) { strcat(string, "                       "); return; }
      else if(timer5.state == 0) {
        unixdiff = timer5.starttime+timer5.duration-timer5.laststop;
      } else {
        unixdiff = timer5.starttime+timer5.duration-unixtime;
      }
      break;
    case 6:
      strcpy(string, "  T6:");
      if(timer6.state == 2) { strcat(string, "                       "); return; }
      else if(timer6.state == 0) {
        unixdiff = timer6.starttime+timer6.duration-timer6.laststop;
      } else {
        unixdiff = timer6.starttime+timer6.duration-unixtime;
      }
      break;
  }
  char buffer[20];
  long long int days=0,hours=0,minutes=0,seconds=0, milliseconds=0;

  milliseconds=unixdiff;  
  seconds = milliseconds / 1000;
  milliseconds %= 1000;
  minutes = seconds / 60;
  seconds %= 60;
  hours = minutes / 60;
  minutes %= 60;
  days = hours / 24;
  hours %= 24;
  

  if (days) {
    itoa(days, (unsigned char*)buffer);
    strcat((char*)string, buffer);
    strcat((char*)string, "d");
    strcat((char*)string, " ");
  }
  
  itoa(hours, (unsigned char*)buffer);
  if (hours < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(minutes, (unsigned char*)buffer);
  if (minutes < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(seconds, (unsigned char*)buffer);
  if (seconds < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  
  strcat((char*)string, ".");
  itoa(milliseconds, (unsigned char*)buffer);
  strcat((char*)string, buffer);

  strcat(string, "                       "); //make sure we have enough spaces to have a background when menu item is selected
}
void drawTimerAppropriateFKeys(int timerno) {
  int iresult;
  // stop icon - 0031
  // CLEAR - 0149
  // play icon - 040A
  // SET - 0010
  switch (timerno) {
    case 1:
    default:
      if (timer1.state == 2) {
        //timer is clear, don't show CLEAR but SET
        //and don't show start/stop button
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (timer1.state == 1) {
        //timer is running, show stop button and CLEAR
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        //timer is stopped, show play button and CLEAR
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 2:
      if (timer2.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (timer2.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 3:
      if (timer3.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (timer3.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 4:
      if (timer4.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (timer4.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 5:
      if (timer5.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (timer5.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
    case 6:
      if (timer6.state == 2) {
        GetFKeyPtr(0x0010, &iresult); // SET
        FKey_Display(1, (int*)iresult);
      } else if (timer6.state == 1) {
        GetFKeyPtr(0x0031, &iresult); // stop icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      } else {
        GetFKeyPtr(0x040A, &iresult); // play icon
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0149, &iresult); // CLEAR
        FKey_Display(1, (int*)iresult);
      }
      break;
  }

}

void setTimerGUI(int timerno) {
  int key, inscreen = 1;
  long long int days = 0;
  long long int hours = 0;
  long long int minutes = 0;
  long long int seconds = 0;

  char buffer1[50] = "";
  char buffer2[50] = "";
  // DAYS SELECTION SCREEN:
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set timer", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Days", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(days, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //hour
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (days != 0) days--;
        break;
      case KEY_CTRL_UP:
        if (days != 730) days++;
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }
  // HOUR SELECTION SCREEN:
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set timer", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Hours", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(hours, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //hour
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (hours == 0) {
          hours = 23;
        } else {
          hours--;
        }
        break;
      case KEY_CTRL_UP:
        if (hours == 23) {
          hours = 0;
        } else {
          hours++;
        }
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }

  // MINUTE SELECTION SCREEN:
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set timer", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Minutes", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(minutes, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //minute
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (minutes == 0) {
          minutes = 59;
        } else {
          minutes--;
        }
        break;
      case KEY_CTRL_UP:
        if (minutes == 59) {
          minutes = 0;
        } else {
          minutes++;
        }
        break;
      case KEY_CTRL_EXE:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }
  
  // SECOND SELECTION SCREEN:
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Set timer", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Seconds", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  int textX=0, textY=150;
  PrintMiniMini( &textX, &textY, (unsigned char*)"NOTE: the timer is set and starts running automatically", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"once you press [EXE].", 0, TEXT_COLOR_BLACK, 0 );
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"                       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(seconds, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK); //day
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (seconds == 0) {
          seconds = 59;
        } else {
          seconds--;
        }
        break;
      case KEY_CTRL_UP:
        if (seconds == 59) {
          seconds = 0;
        } else {
          seconds++;
        }
        break;
      case KEY_CTRL_EXE:
        //All set, set timer
        // convert to seconds
        seconds = seconds + 60*minutes + 60*60*hours + 60*60*24*days;
        setTimer(timerno, seconds*1000);
        
        return;
      case KEY_CTRL_EXIT: return;
    }
  }
}
void processTimerReturnKey(int timerno) { /*does nothing at least for now */ }
void processTimerF1key(int timerno) {
  //stop, start or nothing?
  switch (timerno) {
    case 1:
    default:
      if (timer1.state == 1) {
        stopTimer(timerno);
      } else if (timer1.state == 0) {
        startTimer(timerno);
      }
      //do nothing when timer is clear
      break;
    case 2:
      if (timer2.state == 1) {
        stopTimer(timerno);
      } else if (timer2.state == 0) {
        startTimer(timerno);
      }
      break;
    case 3:
      if (timer3.state == 1) {
        stopTimer(timerno);
      } else if (timer3.state == 0) {
        startTimer(timerno);
      }
      break;
    case 4:
      if (timer4.state == 1) {
        stopTimer(timerno);
      } else if (timer4.state == 0) {
        startTimer(timerno);
      }
      break;
    case 5:
      if (timer5.state == 1) {
        stopTimer(timerno);
      } else if (timer5.state == 0) {
        startTimer(timerno);
      }
      break;
    case 6:
      if (timer6.state == 1) {
        stopTimer(timerno);
      } else if (timer6.state == 0) {
        startTimer(timerno);
      }
      break;
  }
}
void processTimerF2key(int timerno) {
  //set or clear?
  switch (timerno) {
    case 1:
    default:
      if (timer1.state == 2) {
        setTimerGUI(timerno);
      } else {
        clearTimer(timerno);
      }
      break;
    case 2:
      if (timer2.state == 2) {
        setTimerGUI(timerno);
      } else {
        clearTimer(timerno);
      }
      break;
    case 3:
      if (timer3.state == 2) {
        setTimerGUI(timerno);
      } else {
        clearTimer(timerno);
      }
      break;
    case 4:
      if (timer4.state == 2) {
        setTimerGUI(timerno);
      } else {
        clearTimer(timerno);
      }
      break;
    case 5:
      if (timer5.state == 2) {
        setTimerGUI(timerno);
      } else {
        clearTimer(timerno);
      }
      break;
    case 6:
      if (timer6.state == 2) {
        setTimerGUI(timerno);
      } else {
        clearTimer(timerno);
      }
      break;
  }
}
int passiveTimerEndedMessage = 1;
void timerEndedMessage(int timerno) {
  if(passiveTimerEndedMessage) {
    char timer[5];
    char dispstr[15];
    itoa(timerno, (unsigned char*)timer);
    strcpy(dispstr, "Timer ");
    strcat(dispstr, timer);
    strcat(dispstr, " finished");
    DefineStatusMessage((char*)dispstr, 1, 0, 0);
  } else {
    unsigned short key; 
    int keyCol, keyRow; 
    unsigned int initlevel = GetBacklightSubLevel_RAW();
    unsigned int prevlevel = 249;
    char timer[5];
    char dispstr[15];
    itoa(timerno, (unsigned char*)timer);
    strcpy(dispstr, "  Timer ");
    strcat(dispstr, timer);
    strcat(dispstr, " finished");
    Bdisp_AllClr_VRAM();
    if (halfSecondTimer > 0) { halfSeconds = 0; Timer_Start(halfSecondTimer); } else { return; }
    unsigned int prevhalfsecond = halfSeconds;
    while (1) {
      PrintXY(3, 4, dispstr, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
      Bdisp_PutDisp_DD();
      //the following getkeywait does not process MENU so we always have a chance to set the brightness correctly
      if (0 != GetKeyWait_OS(&keyCol,&keyRow,KEYWAIT_HALTOFF_TIMEROFF,0,1, &key) ) {
        SetBacklightSubLevel_RAW(initlevel); DrawFrame( COLOR_WHITE );
        if (halfSecondTimer > 0) { Timer_Stop(halfSecondTimer); }
        return;
      } 
      if(halfSeconds > prevhalfsecond) { //half second has passed
        if (prevlevel == 249) { SetBacklightSubLevel_RAW(0); prevlevel = 0; Bdisp_Fill_VRAM( COLOR_BLACK, 3 ); DrawFrame( COLOR_BLACK ); }
        else { SetBacklightSubLevel_RAW(249); prevlevel = 249; Bdisp_Fill_VRAM( COLOR_WHITE, 3 ); DrawFrame( COLOR_WHITE );}
        prevhalfsecond = halfSeconds;
      }
    }
  }
}
void checkTimerEnd(int timerno, int ispassive=0) {
  //checks whether a timer has got to zero
  switch (timerno) {
    case 1:
    default:
      if (timer1.state == 1) { //only check if timer is running
        if (timer1.starttime+timer1.duration <= currentUnixTime()) {
          //timer has finished, clear it and show message
          clearTimer(timerno);
          timerEndedMessage(timerno);
        }
      }
      break;
    case 2:
      if (timer2.state == 1) { if (timer2.starttime+timer2.duration <= currentUnixTime()) { clearTimer(timerno); timerEndedMessage(timerno); }}
      break;
    case 3:
      if (timer3.state == 1) { if (timer3.starttime+timer3.duration <= currentUnixTime()) { clearTimer(timerno); timerEndedMessage(timerno); }}
      break;
    case 4:
      if (timer4.state == 1) { if (timer4.starttime+timer4.duration <= currentUnixTime()) { clearTimer(timerno); timerEndedMessage(timerno); }}
      break;
    case 5:
      if (timer5.state == 1) { if (timer5.starttime+timer5.duration <= currentUnixTime()) { clearTimer(timerno); timerEndedMessage(timerno); }}
      break;
    case 6:
      if (timer6.state == 1) { if (timer6.starttime+timer6.duration <= currentUnixTime()) { clearTimer(timerno); timerEndedMessage(timerno); }}
      break;
  }
}
void checkAllTimersEnd() {
  checkTimerEnd(1);
  checkTimerEnd(2);
  checkTimerEnd(3);
  checkTimerEnd(4);
  checkTimerEnd(5);
  checkTimerEnd(6);
}
void checkAllTimersEndNoActive() {
  if (passiveTimerEndedMessage) {
    checkTimerEnd(1);
    checkTimerEnd(2);
    checkTimerEnd(3);
    checkTimerEnd(4);
    checkTimerEnd(5);
    checkTimerEnd(6);
  }
}
void drawTimersMenu(int pos, int scroll, int numitems)
{  
  Bdisp_AllClr_VRAM();
  PrintXY(1, 1, (char*)"  Timers", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int textX=18*7; int textY=4;
  //escape sequence doesn't want to be together with number 3.91:
  PrintMini(&textX, &textY, (unsigned char*)"\xE5\xBE", 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"3.91 ms", 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);
  //display timers  
  char timer1string[100]; formatTimerString(1, timer1string);
  char timer2string[100]; formatTimerString(2, timer2string);
  char timer3string[100]; formatTimerString(3, timer3string);
  char timer4string[100]; formatTimerString(4, timer4string);
  char timer5string[100]; formatTimerString(5, timer5string);
  char timer6string[100]; formatTimerString(6, timer6string);  
  
  if(scroll < 1) PrintXY(1,2,(char*)timer1string, (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLUE);
  if(scroll < 2) PrintXY(1,3-scroll,(char*)timer2string, (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_RED);
  if(scroll < 3) PrintXY(1,4-scroll,(char*)timer3string, (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_GREEN);
  if(scroll < 4) PrintXY(1,5-scroll,(char*)timer4string, (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_PURPLE);
  if(scroll < 5) PrintXY(1,6-scroll,(char*)timer5string, (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 6) PrintXY(1,7-scroll,(char*)timer6string, (pos == 6 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLUE);

  drawTimerAppropriateFKeys(pos);
  
  if (setting_display_statusbar == 1) DisplayStatusArea();
}
void timerScreen() {
  short unsigned int key; int pos = 1, scroll = 0, inscreen = 1, keyCol = 0, keyRow = 0;
  int numitems = 6; //total number of items in menu

  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  
  while(inscreen)
  {
    passiveTimerEndedMessage = 0; //here we want blinking messages
    checkAllTimersEnd();
    drawTimersMenu(pos, scroll, numitems);
    Bdisp_PutDisp_DD();
    if (0 != GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key) ) {
      passiveTimerEndedMessage = 1; //key was pressed and we may go out of this screen, so turn on passive messages
      key = PRGM_GetKey();
      switch(key)
      {
        case KEY_PRGM_DOWN:
          if(pos == numitems)
          {
            pos = 1;
            scroll = 0;
          }
          else
          {
            pos++;
            if(pos > scroll+6)
              scroll = pos -6;
          }
          break;
        case KEY_PRGM_UP:
          if(pos == 1)
          {
            pos = numitems;
            scroll = pos-6;
          }
          else
          {
            pos--;
            if(pos-1 < scroll)
              scroll = pos -1;
          }
          break;
        case KEY_PRGM_RETURN:
          processTimerReturnKey(pos);
          SaveAllTimers(); //just to be sure
          break;
        case KEY_PRGM_F1:
          processTimerF1key(pos);
          SaveAllTimers(); //just to be sure
          break;
        case KEY_PRGM_F2:
          processTimerF2key(pos);
          SaveAllTimers(); //just to be sure
          break;
        case KEY_PRGM_EXIT: SaveAllTimers(); passiveTimerEndedMessage = 1; inscreen = 0; break;
      }
    }
  }
}

// --- start of tasks list ---
// tasks: because I'm lazy and to keep the binary size small by reusing portions of code,
// tasks are just calendar events starting on 00/00/0000 00:00:00 and ending on the same time and day.
// this date is inaccessible through the calendar
// only their title, location and description are accessible through the tasks UI (this may change in the future).
void addTaskUI(int key=0) {
  CalendarEvent event;
  EventDate startdate;
  EventDate enddate;
  EventTime starttime;
  EventTime endtime;
  
  startdate.day = 0;
  startdate.month = 0;
  startdate.year = 0;
  event.startdate = startdate;

  enddate.day = 0;
  enddate.month = 0;
  enddate.year = 0;
  event.enddate = enddate;
  
  starttime.hour = 0;
  starttime.minute = 0;
  starttime.second = 0;
  event.starttime = starttime;
  
  endtime.hour = 0;
  endtime.minute = 0;
  endtime.second = 0;
  event.endtime = endtime;
  
  //clean buffers:
  strcpy((char*)event.title, "");
  strcpy((char*)event.location, "");
  strcpy((char*)event.description, "");
addTaskTitleScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Title:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int res = newEventTextInput(1, 3, 21, event.title, 0, 0, 1, key); //force text so title must be at least one char. provide initial key (if it is zero, it is ignored anyway)
  if (res == 1) { return; }
addTaskLocationScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Location:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 128, event.location, 1);
  if (res == 3) { goto addTaskTitleScreen; }
  else if (res == 1) { return; }
addTaskDescriptionScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Description:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 1024, event.description, 1);
  if (res == 3) { goto addTaskLocationScreen; }
  else if (res == 1) { return; }   


  Bdisp_AllClr_VRAM();
  SetBackGround(0x0A);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Add New Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Select category", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  int iresult, inscreen=1; key = 0;
  GetFKeyPtr(0x036F, &iresult); // <
  FKey_Display(0, (int*)iresult);
  GetFKeyPtr(0x04A4, &iresult); // Finish
  FKey_Display(5, (int*)iresult);
  char buffer1[20];
  char buffer2[20];
  event.category = 1;
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"   ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(event.category, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, (event.category <= 6 ? event.category-1 : TEXT_COLOR_YELLOW));
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (event.category == 0) {
          event.category = 7;
        } else {
          event.category--;
        }
        break;
      case KEY_CTRL_UP:
        if (event.category == 7) {
          event.category = 0;
        } else {
          event.category++;
        }
        break;
      case KEY_CTRL_F1:
        goto addTaskDescriptionScreen;
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_F6:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }

  event.daterange = 0;
  event.repeat = 0;
  event.dayofweek = dow(startdate.day, startdate.month, startdate.year);
  res = AddSMEMEvent(event, CALENDAR_FOLDER);
  if(res > 0) {
    MsgBoxPush(4);
    if (res == 4) {
      PrintXY(3, 2, (char*)"  Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else {
      PrintXY(3, 2, (char*)"  Task add. ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
    PrintXY(3, 3, (char*)"  Task could not be", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY(3, 4, (char*)"  added.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY(3, 5, (char*)"     Press:[EXIT]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    int key,inscreen=1;
    while(inscreen) {
      mGetKey(&key);
      switch(key)
      {
        case KEY_CTRL_EXIT:
          inscreen=0;
          break;
      }
    }
    MsgBoxPop();
  }
}
void editTaskUI(CalendarEvent event, int pos) {  
  event.startdate.day = 0;
  event.startdate.month = 0;
  event.startdate.year = 0;

  event.enddate.day = 0;
  event.enddate.month = 0;
  event.enddate.year = 0;
  
  event.starttime.hour = 0;
  event.starttime.minute = 0;
  event.starttime.second = 0;
  
  event.endtime.hour = 0;
  event.endtime.minute = 0;
  event.endtime.second = 0;
  
editTaskTitleScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Title:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  
  int res = newEventTextInput(1, 3, 21, event.title, 0, 0, 1); //force text so title must be at least one char
  if (res == 1) { return; }
editTaskLocationScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Location:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 128, event.location, 1);
  if (res == 3) { goto editTaskTitleScreen; }
  else if (res == 1) { return; }
editTaskDescriptionScreen:
  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Description:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  res = newEventTextInput(1, 3, 1024, event.description, 1);
  if (res == 3) { goto editTaskLocationScreen; }
  else if (res == 1) { return; }   

  Bdisp_AllClr_VRAM();
  SetBackGround(6);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Edit Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(3, 2, (char*)"  Select category", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
  PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
  int iresult, key, inscreen=1;
  GetFKeyPtr(0x036F, &iresult); // <
  FKey_Display(0, (int*)iresult);
  GetFKeyPtr(0x04A4, &iresult); // Finish
  FKey_Display(5, (int*)iresult);
  char buffer1[20];
  char buffer2[20];
  while(inscreen)
  {
    PrintXY(5, 5, (char*)"   ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
    strcpy(buffer1, "  ");
    itoa(event.category, (unsigned char*)buffer2);
    strcat(buffer1, buffer2);
    PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, (event.category <= 6 ? event.category-1 : TEXT_COLOR_YELLOW));
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if (event.category == 0) {
          event.category = 7;
        } else {
          event.category--;
        }
        break;
      case KEY_CTRL_UP:
        if (event.category == 7) {
          event.category = 0;
        } else {
          event.category++;
        }
        break;
      case KEY_CTRL_F1:
        goto editTaskDescriptionScreen;
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_F6:
        inscreen = 0;
        break;
      case KEY_CTRL_EXIT: return;
    }
  }

  event.daterange = 0;
  event.dayofweek = dow(event.startdate.day, event.startdate.month, event.startdate.year);
  EditSMEMEvent(event.startdate, pos, CALENDAR_FOLDER, event);
}
void toggleTaskActivity(CalendarEvent event, int pos) {  
  //use the repeat setting as a task activity indicator. 1 is active/done (check), 0 is unchecked.
  if(event.repeat == 1) {
    event.repeat = 0;
  } else {
    event.repeat = 1;
  }
  EditSMEMEvent(event.startdate, pos, CALENDAR_FOLDER, event);
}
int curSelTask = 1;
int viewTasks() {
  //returns 1 when it wants to be restarted (refresh tasks)
  //returns 0 if the idea really is to exit the screen
  int key, inscreen = 1, scroll=0, numtasks=0;
  curSelTask = 1;

  EventDate taskday;
  taskday.day = 0; taskday.month = 0; taskday.year = 0;  
  numtasks = GetSMEMeventsForDate(taskday, SMEM_CALENDAR_FOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* tasks = (CalendarEvent*)alloca(numtasks*sizeof(CalendarEvent));
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  numtasks = GetSMEMeventsForDate(taskday, SMEM_CALENDAR_FOLDER, tasks);
  int menu = 0;
  while(inscreen) {
    Bdisp_AllClr_VRAM();
    int curtask = 0; //current processing event
    if (numtasks>0) {
  
      while(curtask < numtasks) {
        unsigned char menuitem[100] = "";
        strcpy((char*)menuitem, "  ");
        strcat((char*)menuitem, (char*)tasks[curtask].title);
        strcat((char*)menuitem, "                     "); //make sure we have a string big enough to have background when item is selected
        if(scroll < curtask+1) {
          PrintXY(1,curtask+2-scroll,(char*)menuitem, (curSelTask == curtask+1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), (tasks[curtask].category <= 6 ? tasks[curtask].category-1 : TEXT_COLOR_YELLOW)); //the text color is determined by the category. category 1 and 7 are black (category 7 would be white, but that's invisible)
          if(tasks[curtask].repeat == 0) {
            PrintXY(21,curtask+2-scroll,(char*)"  \xe6\xa5", (curSelTask == curtask+1 ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL), (tasks[curtask].category <= 6 ? tasks[curtask].category-1 : TEXT_COLOR_YELLOW)); //the text color is determined by the category. category 1 and 7 are black (category 7 would be white, but that's invisible)
          } else {
            PrintXY(21,curtask+2-scroll,(char*)"  \xe6\xa9", (curSelTask == curtask+1 ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL), (tasks[curtask].category <= 6 ? tasks[curtask].category-1 : TEXT_COLOR_YELLOW)); //the text color is determined by the category. category 1 and 7 are black (category 7 would be white, but that's invisible)
          }     
        } 
        curtask++;
      }
      //hide 8th item
      PrintXY(1,8,(char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);

      TScrollbar sb;
      sb.I1 = 0;
      sb.I5 = 0;
      sb.indicatormaximum = numtasks;
      sb.indicatorheight = 6;
      sb.indicatorpos = scroll;
      sb.barheight = LCD_HEIGHT_PX - 24*3;
      sb.bartop = 24;
      sb.barleft = LCD_WIDTH_PX - 6;
      sb.barwidth = 6;
      Scrollbar(&sb);
  
    } else {
      PrintXY(7,4,(char*)"  (no tasks)", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
    if (setting_display_statusbar == 1) DisplayStatusArea();
    PrintXY(1, 1, (char*)"  Task list", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    
    int iresult;
    if (menu == 0) {
      if(numtasks>0) {
        GetFKeyPtr(0x049F, &iresult); // VIEW
        FKey_Display(0, (int*)iresult);
      }
      GetFKeyPtr(0x03B4, &iresult); // INSERT
      FKey_Display(1, (int*)iresult);
      if(numtasks>0) {
        GetFKeyPtr(0x0185, &iresult); // EDIT
        FKey_Display(2, (int*)iresult);
        GetFKeyPtr(0x0038, &iresult); // DELETE
        FKey_Display(3, (int*)iresult);
        GetFKeyPtr(0x0104, &iresult); // DEL-ALL
        FKey_Display(4, (int*)iresult);
        GetFKeyPtr(0x049D, &iresult); // Switch [white]
        FKey_Display(5, (int*)iresult);
      }
    }
    mGetKey(&key);
    CalendarEvent tasktoview;
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(curSelTask == numtasks)
        {
          curSelTask = 1;
          scroll = 0;
        }
        else
        {
          curSelTask++;
          if(curSelTask > scroll+(numtasks>6 ? 6 : numtasks))
            scroll = curSelTask -(numtasks>6 ? 6 : numtasks);
        }
        break;
      case KEY_CTRL_UP:
        if(curSelTask == 1)
        {
          curSelTask = numtasks;
          scroll = curSelTask-(numtasks>6 ? 6 : numtasks);
        }
        else
        {
          curSelTask--;
          if(curSelTask-1 < scroll)
            scroll = curSelTask -1;
        }
        break;
      case KEY_CTRL_EXE:
        if(numtasks>0) { tasktoview = tasks[curSelTask-1]; viewEvent(tasktoview, 1); }
        break;
      case KEY_CTRL_F1:
        if(numtasks>0) {
          if (menu == 0) {
            tasktoview = tasks[curSelTask-1]; viewEvent(tasktoview, 1);
          }
        }
        break;
      case KEY_CTRL_F2:
        if (menu == 0) {
          if(numtasks+1 > MAX_DAY_EVENTS) {
            AUX_DisplayErrorMessage( 0x2E );
          } else {
            addTaskUI();
            return 1;
          }
        }
        break;
      case KEY_CTRL_F3:
        if (menu == 0) {        
          if(numtasks>0) {
            editTaskUI(tasks[curSelTask-1], curSelTask-1);
            return 1;
          }    
        }
        break;
      case KEY_CTRL_F4:
        if (menu == 0) {
          if(numtasks>0) {
            delEventUI(0, 0, 0, curSelTask-1, 1);
            curSelTask = 1;
            return 1;
          }
        }
        break;
      case KEY_CTRL_F5:
        if (menu == 0) {
          if(numtasks>0) {
            delAllEventUI(0, 0, 0, 1);
            curSelTask = 1;
            return 1;
          }
        }
        break;
      case KEY_CTRL_F6:
        if (menu == 0) {
          if(numtasks>0) {
            toggleTaskActivity(tasks[curSelTask-1], curSelTask-1);
            //don't return, because number of events didn't change
            //(so our menu pos is still valid) and allocation size didn't change either.
            //just refresh events on the same allocated buffer
            numtasks = GetSMEMeventsForDate(taskday, SMEM_CALENDAR_FOLDER, tasks);
          }
        }
        break;
      case KEY_CTRL_FORMAT:
        //the "FORMAT" key is used in many places in the OS to format e.g. the color of a field,
        //so on this add-in it is used to change the category (color) of a task/calendar event.
        changeEventCategory(tasks[curSelTask-1], curSelTask-1);
        //don't return, because number of events didn't change
        //(so our menu pos is still valid) and allocation size didn't change either.
        //just refresh events on the same allocated buffer
        numtasks = GetSMEMeventsForDate(taskday, SMEM_CALENDAR_FOLDER, tasks);
        break;
      case KEY_CTRL_DEL:
        if(numtasks>0) {
          delEventUI(0, 0, 0, curSelTask-1, 1);
          return 1;
        }
        break;
      case KEY_CTRL_EXIT: return 0; break;
      default:
        if(key && key < 30000)
        {
          //user pressed a char key. start adding a task
          if(numtasks+1 > MAX_DAY_EVENTS) {
            //no need to show this if user presses key on accident, as him/her may not understand the error in that case.
          } else {
            addTaskUI(key);
            return 1;
          }
        }
        break;
    }
  }
  return 0;
} 
// end of tasks
// start of moon information
//#define HAVE_MOON_INFO
#ifdef HAVE_MOON_INFO
int moon_phase(int y, int m, int d)
{
    /*
      calculates the moon phase (0-7), accurate to 1 segment.
      0 = > new moon.
      4 => full moon.
      */

    int c,e;
    double jd;
    int b;

    if (m < 3) {
        y--;
        m += 12;
    }
    ++m;
    c = 365.25*y;
    e = 30.6*m;
    jd = c+e+d-694039.09;  /* jd is total days elapsed */
    jd /= 29.53;           /* divide by the moon cycle (29.53 days) */
    b = jd;		   /* int(jd) -> b, take integer part of jd */
    jd -= b;		   /* subtract integer part to leave fractional part of original jd */
    b = jd*8 + 0.5;	   /* scale fraction from 0-8 and round by adding 0.5 */
    b = b & 7;		   /* 0 and 8 are the same so turn 8 into 0 */
    return b;
}
/*  Astronomical constants  */
#define EPOCH       2444238.5      /* 1980 January 0.0 */

/*  Constants defining the Sun's apparent orbit  */
       
#define ELONGE      278.833540     /* Ecliptic longitude of the Sun at epoch 1980.0 */
#define ELONGP      282.596403     /* Ecliptic longitude of the Sun at perigee */
#define ECCENT      0.016718       /* Eccentricity of Earth's orbit */
#define SUNSMAX     1.495985e8     /* Semi-major axis of Earth's orbit, km */
#define SUNANGSIZ   0.533128       /* Sun's angular size, degrees, at
                                      semi-major axis distance */
/*  Elements of the Moon's orbit, epoch 1980.0  */
#define MMLONG      64.975464      /* Moon's mean longitude at the epoch */
#define MMLONGP     349.383063     /* Mean longitude of the perigee at the epoch */
#define MLNODE      151.950429     /* Mean longitude of the node at the epoch */
#define MINC        5.145396       /* Inclination of the Moon's orbit */
#define MECC        0.054900       /* Eccentricity of the Moon's orbit */
#define MANGSIZ     0.5181         /* Moon's angular size at distance a
                                            from Earth */
#define MSMAX       384401.0       /* Semi-major axis of Moon's orbit in km */
#define MPARALLAX   0.9507         /* Parallax at distance a from Earth */
#define SYNMONTH    29.53058868    /* Synodic month (new Moon to new Moon) */
#define LUNATBASE   2423436.0      /* Base date for E. W. Brown's numbered
                                       series of lunations (1923 January 16) */
       
/*  Properties of the Earth  */
       
#define EARTHRAD    6378.16        /* Radius of Earth in kilometres */
       
/*  Handy mathematical functions  */

#define PI 3.14159265358979323846 

#define SGN(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))       /* Extract sign */
#define ABS(x) ((x) < 0 ? (-(x)) : (x))                   /* Absolute val */
#define FIXANGLE(a)  ((a) < 0 ? 360.*((a)/360.-(long)((a)/360.)+1) : 360.*((a)/360.-(long)((a)/360.)))
                                                          /* angle to 0-30 */
#define FIXRANGLE(a) ((a) < 0 ? 2*PI*((a)/2/PI-(long)((a)/2/PI)+1) : 2*PI*((a)/2/PI-(long)((a)/2/PI)))
                                                          /* angle to 0-2PI */
#define TORAD(d) ((d) * (PI / 180.0))                     /* Deg->Rad     */
#define TODEG(d) ((d) * (180.0 / PI))                     /* Rad->Deg     */

#define abs(a)	   (((a) < 0) ? -(a) : (a))
double sin(double x) //not the most precision. too lazy to find a great one (even if it's slower)
{
    const double B = 4/PI;
    const double C = -4/(PI*PI);

    double y = B * x + C * x * abs(x);

    const double P = 0.225;

    y = P * (y * abs(y) - y) + y; 
    return y;
}
#define cos(x) (sin(x+(PI/2)))
#define dsin(x) (sin(TORAD((x))))                   /* Sin from deg */
#define dcos(x) (cos(TORAD((x))))                   /* Cos from deg */

/* JDATE -- Convert (GMT) date and time to Julian day and fraction. */
long jdate(int year, int month, int day)
{
   long c, m, y;
       
   y = year + 1900;
   m = month + 1;
   if (m > 2) {
      m = m - 3;
   } else {
      m = m + 9;
      y--;
   }
   c = y / 100L;                     /* Compute century */
   y -= 100L * c;
   return (day + (c * 146097L) / 4 + (y * 1461L) / 4 +
                 (m * 153L + 2) / 5 + 1721119L);
}

/*  JTIME  --  Convert (GMT)  date  and  time  to  astronomical
               Julian   time  (i.e. Julian  date  plus  day  fraction,
               expressed as a double).  */
double jtime(int year, int month, int day, int hour, int minute, int second)
{
 return (jdate(year, month, day) - 0.5) +
           (second + 60L * (minute + 60L * hour)) / 86400.0;
}

/*  PHASE  --  Calculate phase of moon as a fraction:       
          The  argument  is  the  time  for  which  the  phase is 
          requested, expressed as a Julian date and fraction.  Returns  the  terminator
          phase  angle  as a percentage of a full circle (i.e., 0 to 1), and 
	  stores into pointer arguments  the  illuminated  fraction  of the 
	  Moon's  disc, the Moon's age in days and fraction, the distance 
          of the Moon from the centre of the Earth, and  the  angular  
          diameter subtended  by the Moon as seen by an observer at the centre of 
          the Earth.
*/
       
double moon_phase(
        double  pdate,                      /* Date for which to calculate phase */
        double  *pphase,                    /* Illuminated fraction */
        double  *mage,                      /* Age of moon in days */
        double  *dist,                      /* Distance in kilometres */
        double  *angdia,                    /* Angular diameter in degrees */
        double  *sudist,                    /* Distance to Sun */
        double  *suangdia)                  /* Sun's angular diameter */
{
    double Day, N, M, Ec, Lambdasun, ml, MM, /* MN,*/ Ev, Ae, A3, MmP,
           mEc, A4, lP, V, lPP, 
           /* NP, y, x, Lambdamoon, BetaM, */
           MoonAge, MoonPhase,
           MoonDist, MoonDFrac, MoonAng, 
           /* MoonPar,*/
           F, SunDist, SunAng,Mrad;
       
          /* Calculation of the Sun's position */
       
    Day = pdate - EPOCH; 		   /* Date within epoch */
    N = FIXANGLE((360 / 365.2422) * Day);   /* Mean anomaly of the Sun */
    M = FIXANGLE(N + ELONGE - ELONGP);	   /* Convert from perigee
					      co-ordinates to epoch 1980.0 */
    Mrad=TORAD(M);
    Ec = Mrad+2*ECCENT*sin(Mrad)+
 	1.25*ECCENT*ECCENT*sin(2*Mrad)+
 	ECCENT*ECCENT*ECCENT*sin(3*Mrad);    /* gives true anomaly accurate to ~arcsec
	                                      (compared to usual formula,
					      that is) */
/*	  if(Mrad > PI) Ec=Ec-2*PI; */
    Ec = TODEG(Ec);
    
    Lambdasun = FIXANGLE(Ec + ELONGP);      /* Sun's geocentric ecliptic
					       longitude */
    /* Orbital distance factor */
    F = (1 + ECCENT * dcos(Ec)) / (1 - ECCENT * ECCENT);
    SunDist = SUNSMAX / F;		    /* Distance to Sun in km */
    SunAng = F * SUNANGSIZ;		    /* Sun's angular size in degrees */
 
    /* Calculation of the Moon's position */
 
    /* Moon's mean longitude */
    ml = FIXANGLE(13.1763966 * Day + MMLONG);
 
    /* Moon's mean anomaly */
    MM = FIXANGLE(ml - 0.1114041 * Day - MMLONGP);
 
    /* Moon's ascending node mean longitude */
      /*  MN = FIXANGLE(MLNODE - 0.0529539 * Day); */
       
     /* Evection */
     Ev = 1.2739 * dsin(2 * (ml - Lambdasun) - MM);
  
     /* Annual equation */
     Ae = 0.1858 * dsin(M);
  
     /* Correction term */
     A3 = 0.37 * dsin(M);
  
     /* Corrected anomaly */
     MmP = MM + Ev - Ae - A3;
  
     /* Correction for the equation of the centre */
     mEc = 6.2886 * dsin(MmP);
  
     /* Another correction term */
     A4 = 0.214 * dsin(2 * MmP);
  
     /* Corrected longitude */
     lP = ml + Ev + mEc - Ae + A4;
       
     /* Variation */
     V = 0.6583 * dsin(2 * (lP - Lambdasun));
  
     /* True longitude */
     lPP = lP + V;
  
     /* Corrected longitude of the node */
     /*    NP = MN - 0.16 * dsin(M); */
  
     /* Y inclination coordinate */
 /*    y = dsin(lPP - NP) * dcos(MINC); */
  
     /* X inclination coordinate */
 /*    x = dcos(lPP - NP); */
  
     /* Ecliptic longitude */
 /*    Lambdamoon = TODEG(atan2(y, x)); */
 /*    Lambdamoon += NP; */
  
     /* Ecliptic latitude */
 /*    BetaM = TODEG(asin(sin(TORAD(lPP - NP)) * sin(TORAD(MINC)))); */
  
     /* Calculation of the phase of the Moon */
  
     /* Age of the Moon in degrees */
     MoonAge = lPP - Lambdasun;
  
     /* Phase of the Moon */
     MoonPhase = (1 - dcos(MoonAge)) / 2;
  
     /* Calculate distance of moon from the centre of the Earth */
  
     MoonDist = (MSMAX * (1 - MECC * MECC)) /
		(1 + MECC * dcos(MmP + mEc));
  
     /* Calculate Moon's angular diameter */
  
     MoonDFrac = MoonDist / MSMAX;
     MoonAng = MANGSIZ / MoonDFrac;
  
     /* Calculate Moon's parallax */
  
 /*    MoonPar = MPARALLAX / MoonDFrac; */
  
     *pphase = MoonPhase;
     *mage = SYNMONTH * (FIXANGLE(MoonAge) / 360.0);
     *dist = MoonDist;
     *angdia = MoonAng;
     *sudist = SunDist;
     *suangdia = SunAng;
     return FIXANGLE(MoonAge) / 360.0;
}
void moonInformation() {
  int key, textX=0, textY=24+4;
  int today_y = ((*RYRCNT >> 12) & 0b1111)*1000 + ((*RYRCNT >> 8) & 0b1111)*100 + ((*RYRCNT >> 4) & 0b1111)*10 + (*RYRCNT & 0b1111);
  int today_m = ((*RMONCNT >> 4) & 0b1)*10 + (*RMONCNT & 0b1111);
  int today_d = ((*RDAYCNT >> 4) & 0b11)*10 + (*RDAYCNT & 0b1111);
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Moon information", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);

  //PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, colorfg, colorbg, 0, 0); //get length
  //textX = LCD_WIDTH_PX/2 - textX/2; //center
  unsigned char buffer[50] = "";
  int moonphase = moon_phase(today_y, today_m, today_d);
  switch(moonphase) {
    case 0: strcpy((char*)buffer, (char*)"New moon"); break;
    case 1: strcpy((char*)buffer, (char*)"Waxing crescent moon"); break;
    case 2: strcpy((char*)buffer, (char*)"First quarter moon"); break;
    case 3: strcpy((char*)buffer, (char*)"Waxing gibbous moon"); break;
    case 4: strcpy((char*)buffer, (char*)"Full moon"); break;
    case 5: strcpy((char*)buffer, (char*)"Waning gibbous moon"); break;
    case 6: strcpy((char*)buffer, (char*)"Last quarter moon"); break;
    case 7: strcpy((char*)buffer, (char*)"Waning crescent moon"); break; 
  }
  PrintMini(&textX, &textY, (unsigned char*)"Phase: ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

  double jd = jtime(today_y, today_m, today_d, bcd_to_2digit(RHRCNT), bcd_to_2digit(RMINCNT), bcd_to_2digit(RSECCNT));
  double cphase,aom,cdist,cangdia,csund,csuang;
  double phasefrac = moon_phase(jd, &cphase, &aom, &cdist, &cangdia, &csund, &csuang);
  textX=0; textY+=17;
  PrintMini(&textX, &textY, (unsigned char*)"Distance: ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  itoa(int(csund), buffer);
  PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)" km", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  
  /*textY=textY+17;
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
  PrintMini(&textX, &textY, (unsigned char*)cfreq, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);*/
  mGetKey(&key);
}
// end of moon information
#endif
void drawTimeMenu(int pos, int scroll, int numitems)
{  
  drawFkeyPopup(2, setting_black_theme);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  if(setting_black_theme) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Time tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);

  if(scroll < 1) PrintXY(2,3,(char*)"  Calendar           ", (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 2) PrintXY(2,4-scroll,(char*)"  Stopwatch          ", (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 3) PrintXY(2,5-scroll,(char*)"  Timer              ", (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
if(scroll < 4) PrintXY(2,6-scroll,(char*)"  Tasks              ", (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
#ifdef HAVE_MOON_INFO  
  if(scroll < 5 && scroll > 0) PrintXY(2,7-scroll,(char*)"  Moon information   ", (pos == 5 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);

  TScrollbar sb;
  sb.I1 = 0;
  sb.I5 = 0;
  sb.indicatormaximum = numitems;
  sb.indicatorheight = 4;
  sb.indicatorpos = scroll;
  sb.barheight = LCD_HEIGHT_PX - 24*5;
  sb.bartop = 24*2;
  sb.barleft = LCD_WIDTH_PX - 6 - 18 - 5;
  sb.barwidth = 6;
  Scrollbar(&sb);
#endif
}

void timeMenu() {
  int key, pos = 1, scroll = 0, inscreen = 1;
#ifdef HAVE_MOON_INFO
  int numitems = 5; //total number of items in menu
#else 
  int numitems = 4; //total number of items in menu
#endif
  int tres = 1;
  while(inscreen)
  {
    drawTimeMenu(pos, scroll, numitems);
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(pos == numitems)
        {
          pos = 1;
          scroll = 0;
        }
        else
        {
          pos++;
          if(pos > scroll+4)
            scroll = pos -4;
        }
        break;
      case KEY_CTRL_UP:
        if(pos == 1)
        {
          pos = numitems;
          scroll = pos-4;
        }
        else
        {
          pos--;
          if(pos-1 < scroll)
            scroll = pos -1;
        }
        break;
      case KEY_CTRL_EXE:
        switch(pos)
        {
          case 1:
            calendarScreen();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 2:
            stopwatchScreen();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 3:
            timerScreen();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 4:
            while(tres==1) { //restart screen until user exits
              tres = viewTasks();
            }
            inscreen = 0;
            break;
#ifdef HAVE_MOON_INFO 
          case 5:
            moonInformation();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
#endif
          default: break;
        }
        break;
      case KEY_CHAR_1: calendarScreen(); inscreen = 0; break;
      case KEY_CHAR_2: stopwatchScreen(); inscreen = 0; break;
      case KEY_CHAR_3: timerScreen(); inscreen = 0; break;
      case KEY_CHAR_4: while(tres==1) { tres = viewTasks(); } inscreen = 0; break;
#ifdef HAVE_MOON_INFO 
      case KEY_CHAR_5: moonInformation(); break;
#endif
      case KEY_CTRL_EXIT: inscreen = 0; break;
    }
  }
}
//////////////////////////////////////////
// END OF TIME TOOLS
//////////////////////////////////////////
// START OF GENERIC TOOLS
//////////////////////////////////////////
//Syscall test findings (don't belong here and should be taken out of here and saved in a safe location ASAP):
//with syscall == 0x1E77, no effect (may be EnableGetkeyToMain...)
//THIS MAY BE "	CallbackAtQuitMainFunction"!!: with syscall == 0x1E78, once you press Menu you're taken to the Link app, and there pressing Menu results in a brief spinning icon but no Menu. Pressing Menu in one of the submenus of the Link screen results in you being taken to the main Link screen. Conclusion: Menu became Link.
//with syscall == 0x1E79,0x1E7A,0x1E7B,0x1E7C,0x1E7E,0x1E7F, no effect.
/*void ruler() {
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
#define RULER_Y LCD_HEIGHT_PX/2
#define MARK_HEIGHT 10
#define MARK_START 0
#define MARK_END LCD_WIDTH_PX
#define RULER_COLOR COLOR_BLACK
  drawLine(0, RULER_Y, LCD_WIDTH_PX, RULER_Y, RULER_COLOR);
  drawLine(MARK_START, RULER_Y-MARK_HEIGHT/2, MARK_START, RULER_Y+MARK_HEIGHT/2, RULER_COLOR);
  drawLine(MARK_START+55, RULER_Y-MARK_HEIGHT/2, MARK_START+55, RULER_Y+MARK_HEIGHT/2, RULER_COLOR);
  drawLine(MARK_START+2*55, RULER_Y-MARK_HEIGHT/2, MARK_START+2*55, RULER_Y+MARK_HEIGHT/2, RULER_COLOR);
  drawLine(MARK_START+3*55, RULER_Y-MARK_HEIGHT/2, MARK_START+3*55, RULER_Y+MARK_HEIGHT/2, RULER_COLOR);
  drawLine(MARK_START+4*55, RULER_Y-MARK_HEIGHT/2, MARK_START+4*55, RULER_Y+MARK_HEIGHT/2, RULER_COLOR);
  drawLine(MARK_START+5*55, RULER_Y-MARK_HEIGHT/2, MARK_START+5*55, RULER_Y+MARK_HEIGHT/2, RULER_COLOR); 
  drawLine(MARK_START+6*55, RULER_Y-MARK_HEIGHT/2, MARK_START+6*55, RULER_Y+MARK_HEIGHT/2, RULER_COLOR);

  int key;
  GetKey(&key);
}*/

// START of file browser
typedef struct
{
  char filename[256]; //filename, not proper for use with Bfile.
  char name[120]; //friendly name (without //fls0/ or complete path)
  int isfolder;
  int isselected;
} File;
typedef struct
{
	unsigned short id, type;
	unsigned long fsize, dsize;
	unsigned int property;
	unsigned long address;
} file_type_t;
int GetAnyFiles(File files[], char* basepath) {
  /*searches storage memory for folders and files, returns their count*/
  /*basepath should start with \\fls0\ and should always have a slash (\) at the end */
	unsigned short path[0x10A], found[0x10A];
	unsigned char buffer[0x10A];

	// make the buffer
	strcpy((char*)buffer, basepath);
	strcat((char*)buffer, "*");
	
	int curitem = 0;
	file_type_t fileinfo;
	int findhandle;
	Bfile_StrToName_ncpy(path, buffer, 0x10A);
	int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
	while(!ret) {
		Bfile_NameToStr_ncpy(buffer, found, 0x10A);
		if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 || strcmp((char*)buffer, "@MainMem") == 0))
		{
			strcpy(files[curitem].name, (char*)buffer);
			strcpy(files[curitem].filename, basepath);
			strcat(files[curitem].filename, (char*)buffer);
			if(fileinfo.fsize == 0) files[curitem].isfolder = 1; else files[curitem].isfolder = 0;
			files[curitem].isselected = 0; //clear selection. this means selection is cleared when changing directory (doesn't happen with native file manager)
			curitem++;
		}
		ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
	}
	Bfile_FindClose(findhandle);
	
	return curitem;
}
void deleteSelectedFiles(File* files, int numfiles, int todelfiles) {
  //files: the array (list) of files to perform operations in. NOT files to delete (this will only delete selected files)
  //numfiles: total number of files in array
  //todelfiles: the number of files to delete (count of selected files). allows for not having to loop all the way to the end (once the amount of deleted files is deleted, the loop stops)
  //REFRESH the files array after calling this!
  int curfile = 0; //current processing file (not number of deleted files!)
  int delfiles = 0; //number of files deleted
  unsigned short path[0x10A];
  char buffer[50] = "";
  char buffer2[5] = "";
  if (numfiles>0) {
    while(curfile < numfiles && delfiles < todelfiles) {  
      if (files[curfile].isselected) {
        Bfile_StrToName_ncpy(path, (unsigned char*)files[curfile].filename, 0x10A);
        Bfile_DeleteEntry( path );
        delfiles++;
      }
      strcpy(buffer, "  Deleting (");
      itoa(delfiles, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      strcat(buffer, "  /");
      itoa(todelfiles, (unsigned char*)buffer2);
      strcat(buffer, buffer2);
      strcat(buffer, "  )");
      PrintXY(1,8,(char*)buffer, TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      
      curfile++;
    }
  }
  PrintXY(1,8,(char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
}
void fileBrowser() {
  char title[6] = "Files";
  int inloop = 1;
  char browserbasepath[256] = "\\\\fls0\\";
  int curSelFile = 1;
  while(inloop) {
    int key, inscreen = 1, scroll=0, numfiles=0, selfiles=0;
    curSelFile = 1;
    File files[200]; 
    
    Bdisp_AllClr_VRAM();
    DisplayStatusArea();
    numfiles = GetAnyFiles(files, browserbasepath);
    while(inscreen) {
      Bdisp_AllClr_VRAM();
      int curfile = 0; //current processing file
      selfiles = 0;
      if (numfiles>0) {
        while(curfile < numfiles) {
          unsigned char menuitem[100] = "";
          strcpy((char*)menuitem, "    ");
          strcat((char*)menuitem, (char*)files[curfile].name);
          strcat((char*)menuitem, "                     "); //make sure we have a string big enough to have background when item is selected
          if(scroll < curfile+1) {
            PrintXY(1,curfile+2-scroll,(char*)menuitem, (curSelFile == curfile+1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);     
            if (files[curfile].isfolder == 1) {
              if((curfile+2-scroll)>=2&&(curfile+2-scroll)<=7) CopySpriteMasked((unsigned char*)folder_icon, 18, (curfile+2-scroll)*24+4, 17, 15, 0xf81f  );
            }
            if (files[curfile].isselected) {
              if (curSelFile == curfile+1) {
                  PrintXY(1,curfile+2-scroll,(char*)"   ", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
                  PrintXY(1,curfile+2-scroll,(char*)"  \xe6\x9b", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_GREEN);
              } else {
                PrintXY(1,curfile+2-scroll,(char*)"   ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
                PrintXY(1,curfile+2-scroll,(char*)"  \xe6\x9b", TEXT_MODE_NORMAL, TEXT_COLOR_PURPLE);
              }
            }
          }
          if (files[curfile].isselected) selfiles++;
          curfile++;
        }
        //hide 8th item
        PrintXY(1,8,(char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  
        TScrollbar sb;
        sb.I1 = 0;
        sb.I5 = 0;
        sb.indicatormaximum = numfiles;
        sb.indicatorheight = 6;
        sb.indicatorpos = scroll;
        sb.barheight = LCD_HEIGHT_PX - 24*3;
        sb.bartop = 24;
        sb.barleft = LCD_WIDTH_PX - 6;
        sb.barwidth = 6;
        Scrollbar(&sb);
      } else {
        PrintXY(8,4,(char*)"  No Data", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      }
      DisplayStatusArea();
      char titleBuffer[23] = "";
      strcpy(titleBuffer, "  ");
      strcat(titleBuffer, title);
      PrintXY(1, 1, titleBuffer, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
      int textX=strlen(title)*18+10, textY=6;
      char friendlypath[256] = "";
      strcpy(friendlypath, browserbasepath+6);
      friendlypath[strlen(friendlypath)-1] = '\0'; //remove ending slash like OS does
      PrintMini(&textX, &textY, (unsigned char*)friendlypath, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      int iresult;
      if(numfiles>0) {
        GetFKeyPtr(0x0037, &iresult); // SELECT (white)
        FKey_Display(0, (int*)iresult);
      }
      if(selfiles>0) {
        GetFKeyPtr(0x0038, &iresult); // DELETE
        FKey_Display(5, (int*)iresult);
      }
      GetKey(&key);
      switch(key)
      {
        case KEY_CTRL_DOWN:
          if(curSelFile == numfiles)
          {
            curSelFile = 1;
            scroll = 0;
          }
          else
          {
            curSelFile++;
            if(curSelFile > scroll+(numfiles>6 ? 6 : numfiles))
              scroll = curSelFile -(numfiles>6 ? 6 : numfiles);
          }
          break;
        case KEY_CTRL_UP:
          if(curSelFile == 1)
          {
            curSelFile = numfiles;
            scroll = curSelFile-(numfiles>6 ? 6 : numfiles);
          }
          else
          {
            curSelFile--;
            if(curSelFile-1 < scroll)
              scroll = curSelFile -1;
          }
          break;
        case KEY_CTRL_EXE:
          if(files[curSelFile-1].isfolder) {
            strcpy(browserbasepath, files[curSelFile-1].filename); //switch to selected folder
            strcat(browserbasepath, "\\");
            inscreen = 0; //reload at new folder
          }
          break;
        case KEY_CTRL_F1:
          files[curSelFile-1].isselected ? files[curSelFile-1].isselected = 0 : files[curSelFile-1].isselected = 1;
          break;
        case KEY_CTRL_F6:
          deleteSelectedFiles(files, numfiles, selfiles);
          inscreen = 0; //reload file list
          break;
        case KEY_CTRL_EXIT:
        	if(!strcmp(browserbasepath,"\\\\fls0\\")) { //check that we aren't already in the root folder
        	  //we are, return
            return;
          } else {
        	  int i=strlen(browserbasepath)-2;
        	  while (i>=0 && browserbasepath[i] != '\\')
        		  i--;
        	  if (browserbasepath[i] == '\\') {
              char tmp[256] = "";
              memcpy(tmp,browserbasepath,i+1);
              tmp[i+1] = '\0';
              strcpy(browserbasepath, tmp);
        	  }
            inscreen = 0; //reload at new folder
          }
          break;
      }
    }
  }
}
// END of file browser

#define TOTAL_SMEM 16801792 //as seen on the TEST MODE, on the emulator, OS 1.02, and on the TEST MODE of a real fx-CG 20, OS 1.04.
#define DRAW_MEMUSAGE_GRAPHS
void memoryCapacityViewer() {
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Memory usage", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  int smemfree;
  unsigned char buffer[50] ="";
  unsigned short smemMedia[7]={'\\','\\','f','l','s','0',0};
  Bfile_GetMediaFree_OS( smemMedia, &smemfree );
  itoa(smemfree, buffer);
  unsigned char smemtext[70] = "";
  strcpy((char*)smemtext, "Storage: ");
  strcat((char*)smemtext, (char*)buffer);
  strcat((char*)smemtext, " bytes free");
  int textY = 24+6; int textX = 0;
  PrintMini(&textX, &textY, smemtext, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  strcpy((char*)smemtext, "out of ");
  itoa(TOTAL_SMEM, buffer);
  strcat((char*)smemtext, (char*)buffer);
  strcat((char*)smemtext, " bytes (");
  itoa(TOTAL_SMEM-smemfree, buffer);
  strcat((char*)smemtext, (char*)buffer);
  strcat((char*)smemtext, " bytes used)");
  textY = textY + 17; textX = 60;
  PrintMiniMini( &textX, &textY, smemtext, 0, TEXT_COLOR_BLACK, 0 );  
  
  
#ifdef DRAW_MEMUSAGE_GRAPHS
  textY = textY + 12;
  //what could be done in one line, has to be done in 3+another var, because of integer overflows -.-
  unsigned int tmpvar = TOTAL_SMEM-smemfree;
  tmpvar = LCD_WIDTH_PX*tmpvar;
  unsigned int barwidthcpl = tmpvar/TOTAL_SMEM;
  drawRectangle(0, textY+24, LCD_WIDTH_PX, 20, COLOR_GRAY);
  drawRectangle(0, textY+24, barwidthcpl, 20, COLOR_BLUE);
  
  int newTextX = 0;
  int newTextY = textY+5;
  itoa(100*(TOTAL_SMEM-smemfree)/TOTAL_SMEM, buffer);
  strcat((char*)buffer, "% used");
  PrintMiniMini( &newTextX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 1 ); //fake draw
  textX = LCD_WIDTH_PX/2 - newTextX/2;
  PrintMiniMini( &textX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 0 ); //draw  
  
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_WHITE, COLOR_GRAY);  
  VRAMReplaceColorInRect(0, textY+24, barwidthcpl, 20, COLOR_GRAY, COLOR_BLUE);
  
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_CYAN, COLOR_WHITE);
  textX = 0; textY = textY + 5;
#endif
  
  int MCSmaxspace; int MCScurrentload; int MCSfreespace;  
  MCS_GetState( &MCSmaxspace, &MCScurrentload, &MCSfreespace );
  barwidthcpl = (LCD_WIDTH_PX*(MCSmaxspace-MCSfreespace))/MCSmaxspace;
  itoa(MCSfreespace, buffer);
  unsigned char mcstext[70] = "";
  strcpy((char*)mcstext, "Main: ");
  strcat((char*)mcstext, (char*)buffer);
  strcat((char*)mcstext, " bytes free");  
  textY = textY + 17; textX = 0;
  PrintMini(&textX, &textY, mcstext, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  strcpy((char*)mcstext, "out of ");
  itoa(MCSmaxspace, buffer);
  strcat((char*)mcstext, (char*)buffer);
  strcat((char*)mcstext, " bytes (");
  itoa(MCScurrentload, buffer);
  strcat((char*)mcstext, (char*)buffer);
  strcat((char*)mcstext, " bytes used)");
  textY = textY + 17; textX = 60;
  PrintMiniMini( &textX, &textY, mcstext, 0, TEXT_COLOR_BLACK, 0 );
#ifdef DRAW_MEMUSAGE_GRAPHS
  textY = textY + 12;
  drawRectangle(0, textY+24, LCD_WIDTH_PX, 20, COLOR_GRAY);
  drawRectangle(0, textY+24, barwidthcpl, 20, COLOR_BLUE);

  newTextX = 0;
  newTextY = textY+5;
  itoa(100*MCScurrentload/MCSmaxspace, buffer);
  strcat((char*)buffer, "% used");
  PrintMiniMini( &newTextX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 1 ); //fake draw
  textX = LCD_WIDTH_PX/2 - newTextX/2;
  PrintMiniMini( &textX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 0 ); //draw  
  
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_WHITE, COLOR_GRAY);  
  VRAMReplaceColorInRect(0, textY+24, barwidthcpl, 20, COLOR_GRAY, COLOR_BLUE);
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_CYAN, COLOR_WHITE);
#endif
  int key;
  GetKey(&key);
}

typedef struct
{
  int active; //whether the add-in is set to show in menu (.g3a) or hidden (.h3a)
  char filename[128]; //filename, not proper for use with Bfile.
  char name[50]; //friendly name
} AddIn;
int GetAddins(AddIn addins[]) {
  /*searches storage memory for active and inactive add-ins, returns their count*/
	unsigned short path[0x10A], path2[0x10A], found[0x10A];
	unsigned char buffer[0x10A], buffer2[0x10A];

	// make the buffer
	strcpy((char*)buffer, "\\\\fls0\\*");
	strcpy((char*)buffer2, "\\\\fls0\\*");
	//     strcat((char*)buffer, filter);
	
	int curitem = 0;
	file_type_t fileinfo;
	int findhandle;
	Bfile_StrToName_ncpy(path, buffer, 0x10A);
	Bfile_StrToName_ncpy(path2, buffer2, 0x10A);
	int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
	Bfile_StrToName_ncpy(path, (unsigned char*)"*.g3a", 0x10A);
	Bfile_StrToName_ncpy(path2, (unsigned char*)"*.h3a", 0x10A);
	while(!ret) {
		Bfile_NameToStr_ncpy(buffer, found, 0x10A);
		if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 || strcmp((char*)buffer, "utilities.g3a") == 0) &&
		   ((Bfile_Name_MatchMask((const short int*)path, (const short int*)found)) || (Bfile_Name_MatchMask((const short int*)path2, (const short int*)found))))
		{
			strcpy(addins[curitem].filename, (char*)buffer);
			//TODO: get friendly add-in name from system add-in table
			strcpy(addins[curitem].name, (char*)buffer);
			addins[curitem].name[strlen((char*)buffer)-4] = '\0';
			addins[curitem].active= (Bfile_Name_MatchMask((const short int*)path, (const short int*)found) ? 1 : 0);
			curitem++;
		}
		ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
	}

	Bfile_FindClose(findhandle);
	
	return curitem;
}
int curSelAddin = 1;
int addinManager() {
  //returns 1 when it wants to be restarted (refresh addins)
  //returns 0 if the idea really is to exit the screen
  int key, inscreen = 1, scroll=0, numaddins=0;
  curSelAddin = 1;
  AddIn addins[200]; 
  
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  numaddins = GetAddins(addins);
  int menu = 0;
  while(inscreen) {
    Bdisp_AllClr_VRAM();
    int curaddin = 0; //current processing addin
    if (numaddins>0) {
  
      while(curaddin < numaddins) {
        unsigned char menuitem[100] = "";
        strcpy((char*)menuitem, "  ");
        strcat((char*)menuitem, (char*)addins[curaddin].name);
        strcat((char*)menuitem, "                     "); //make sure we have a string big enough to have background when item is selected
        if(scroll < curaddin+1) {
          PrintXY(1,curaddin+2-scroll,(char*)menuitem, (curSelAddin == curaddin+1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), (addins[curaddin].active ? TEXT_COLOR_BLACK : TEXT_COLOR_CYAN));     
        } 
        curaddin++;
      }
      //hide 8th item
      PrintXY(1,8,(char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      TScrollbar sb;
      sb.I1 = 0;
      sb.I5 = 0;
      sb.indicatormaximum = numaddins;
      sb.indicatorheight = 6;
      sb.indicatorpos = scroll;
      sb.barheight = LCD_HEIGHT_PX - 24*3;
      sb.bartop = 24;
      sb.barleft = LCD_WIDTH_PX - 6;
      sb.barwidth = 6;
      Scrollbar(&sb);
  
    } else {
      PrintXY(5,4,(char*)"   (no Add-Ins)", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
    if (setting_display_statusbar == 1) DisplayStatusArea();
    PrintXY(1, 1, (char*)"  Add-In Manager", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    
    int iresult;
    if (menu == 0) {
      if(numaddins>0) {
        GetFKeyPtr((addins[curSelAddin-1].active == 0 ? 0x0011 : 0x0012), &iresult); // On/Off (white)
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0038, &iresult); // DELETE
        FKey_Display(1, (int*)iresult);
      }
      GetFKeyPtr(0x03FD, &iresult); // HELP (white)
      FKey_Display(5, (int*)iresult);
    }
    mGetKey(&key);
    unsigned short newpath[0x10A];
    char buffer[0x10A] = "";
    unsigned short oldpath[0x10A];
    int textX=0, textY=0;
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(curSelAddin == numaddins)
        {
          curSelAddin = 1;
          scroll = 0;
        }
        else
        {
          curSelAddin++;
          if(curSelAddin > scroll+(numaddins>6 ? 6 : numaddins))
            scroll = curSelAddin -(numaddins>6 ? 6 : numaddins);
        }
        break;
      case KEY_CTRL_UP:
        if(curSelAddin == 1)
        {
          curSelAddin = numaddins;
          scroll = curSelAddin-(numaddins>6 ? 6 : numaddins);
        }
        else
        {
          curSelAddin--;
          if(curSelAddin-1 < scroll)
            scroll = curSelAddin -1;
        }
        break;
      case KEY_CTRL_F1:
        if(addins[curSelAddin-1].active) { //disable
          strcpy(buffer, "\\\\fls0\\");
          strcat(buffer, addins[curSelAddin-1].filename);
          Bfile_StrToName_ncpy(oldpath, (unsigned char*)buffer, 0x10A);
          buffer[strlen((char*)buffer)-3] = 'h'; //so it goes from g3a to h3a
          Bfile_StrToName_ncpy(newpath, (unsigned char*)buffer, 0x10A);
          Bfile_RenameEntry( oldpath , newpath );
        } else { //enable
          strcpy(buffer, "\\\\fls0\\");
          strcat(buffer, addins[curSelAddin-1].filename);
          Bfile_StrToName_ncpy(oldpath, (unsigned char*)buffer, 0x10A);
          buffer[strlen((char*)buffer)-3] = 'g'; //so it goes from h3a to g3a
          Bfile_StrToName_ncpy(newpath, (unsigned char*)buffer, 0x10A);
          Bfile_RenameEntry( oldpath , newpath );
        }
        return 1; //reload list
        break;
      case KEY_CTRL_F2:
        MsgBoxPush(4);
        while (1) {
          
          PrintXY(3, 2, (char*)"  Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(3, 3, (char*)"  Selected Add-In?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(3, 4, (char*)"     Yes:[F1]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(3, 5, (char*)"     No :[F6]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mGetKey(&key);
          if (key==KEY_CTRL_F1) {
            MsgBoxPop();
            strcpy(buffer, "\\\\fls0\\");
            strcat(buffer, addins[curSelAddin-1].filename);
            Bfile_StrToName_ncpy(oldpath, (unsigned char*)buffer, 0x10A);
            Bfile_DeleteEntry( oldpath );
            return 1;
          } else if (key == KEY_CTRL_F6 || key == KEY_CTRL_EXIT ) {
            MsgBoxPop();
            break;
          }
        }
        break;
      case KEY_CTRL_F6:
        MsgBoxPush(4);
        textX = 2*18+2; textY = 2*24-26;
        PrintMiniMini( &textX, &textY, (unsigned char*)"This tool lets you hide add-ins from the Main", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"Menu without the need for deleting them from", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"your calculator. To hide an add-in, press [F1]", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"(Off) when it is selected. Hidden add-ins are", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"shown in light blue. To make an hidden add-in", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"appear back in the Main Menu, press [F1] (On).", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"This tool also lets you delete (uninstall) add-ins.", 0, TEXT_COLOR_BLACK, 0 );
        textX = 12*18+2; textY= textY+12+2;
        PrintMiniMini( &textX, &textY, (unsigned char*)"Press any key...", 0, TEXT_COLOR_BLACK, 0 );
        mGetKey(&key);
        MsgBoxPop();
        MsgBoxPush(4);
        textX = 2*18+2; textY = 2*24-26;
        PrintMiniMini( &textX, &textY, (unsigned char*)"To hide add-ins, this tool simply changes their", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"file extension between g3a and h3a (hidden).", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"This means the hiding effect is permanent even", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"if the Utilities add-in is uninstalled. This", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"add-in can't perform operations on itself.", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"Add-ins are shown in the same order as they", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"are in memory.", 0, TEXT_COLOR_BLACK, 0 );
        textX = 12*18+2; textY= textY+12+2;
        PrintMiniMini( &textX, &textY, (unsigned char*)"Press any key...", 8, TEXT_COLOR_BLACK, 0 );
        mGetKey(&key);
        MsgBoxPop();
        MsgBoxPush(4);
        textX = 2*18+2; textY = 2*24-26;
        PrintMiniMini( &textX, &textY, (unsigned char*)"Enabling and disabling add-ins may change", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"their position in memory, and consequently in", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"the Main Menu and in this list.", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"Use this to your advantage, but note that the", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"order of the add-ins can't be directly", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"controlled without removing all add-ins and", 0, TEXT_COLOR_BLACK, 0 );
        textX = 2*18+2; textY= textY+12;
        PrintMiniMini( &textX, &textY, (unsigned char*)"putting them back one by one, through PC-USB.", 0, TEXT_COLOR_BLACK, 0 );
        textX = 12*18+2; textY= textY+12+2;
        PrintMiniMini( &textX, &textY, (unsigned char*)"Press any key...", 8, TEXT_COLOR_BLACK, 0 );
        mGetKey(&key);
        MsgBoxPop();
        break;
      case KEY_CTRL_EXIT: return 0; break;
    }
  }
  return 0;
}
void changeFKeyColor() {
  unsigned char*keycolor = (unsigned char*) 0xFD8013E4;
  unsigned char selcolor = (unsigned char) 0xFF; //just so it isn't uninitialized
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Function key color", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  int textX=0; int textY=132;
  PrintMiniMini( &textX, &textY, (unsigned char*)"Please note that only the Utilities add-in and an hidden", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"debug screen on your calculator are able to change this", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"setting, which survives reboots. To reset it back to the", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"black color you need to use this add-in, the hidden debug", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"screen or to reset the Main Memory.", 0, TEXT_COLOR_BLACK, 0 );
  Bdisp_PutDisp_DD();
  selcolor = ColorIndexDialog1( *keycolor, 0 );
  if(selcolor != (unsigned char)0xFF) {
    //user didn't press EXIT, QUIT or AC/ON. input is validated.
    *keycolor = selcolor;
  }
}
void drawToolsMenu(int pos, int scroll, int numitems)
{  
  drawFkeyPopup(3, setting_black_theme);
  if (setting_display_statusbar == 1) DisplayStatusArea();
  if(setting_black_theme) {
    DrawFrame(COLOR_BLACK);
    darkenStatusbar(); 
  }
  PrintXY(2, 2, (char*)"  Tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);

  if(scroll < 1) PrintXY(2,3,(char*)"  File browser       ", (pos == 1 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 2) PrintXY(2,4-scroll,(char*)"  Memory usage       ", (pos == 2 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 3) PrintXY(2,5-scroll,(char*)"  Add-In Manager     ", (pos == 3 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);
  if(scroll < 4) PrintXY(2,6-scroll,(char*)"  Function key color ", (pos == 4 ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND), TEXT_COLOR_BLACK);

}

void toolsMenu() {
  int key, pos = 1, scroll = 0, inscreen = 1;
  int numitems = 4; //total number of items in menu
  while(inscreen)
  {
    drawToolsMenu(pos, scroll, numitems);
    mGetKey(&key);
    int tres = 1;
    switch(key)
    {
      case KEY_CTRL_DOWN:
        if(pos == numitems)
        {
          pos = 1;
          scroll = 0;
        }
        else
        {
          pos++;
          if(pos > scroll+4)
            scroll = pos -4;
        }
        break;
      case KEY_CTRL_UP:
        if(pos == 1)
        {
          pos = numitems;
          scroll = pos-4;
        }
        else
        {
          pos--;
          if(pos-1 < scroll)
            scroll = pos -1;
        }
        break;
      case KEY_CTRL_EXE:
        switch(pos)
        {
          case 1:
            //ruler();
            fileBrowser();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 2:
            memoryCapacityViewer();
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 3:
            while(tres==1) { tres = addinManager(); }
            inscreen = 0; //exit popup to avoid strange drawings
            break;
          case 4:
            changeFKeyColor();
            inscreen = 0;
            break;
          default: break;
        }
        break;
      case KEY_CHAR_1: fileBrowser(); inscreen = 0; break;
      case KEY_CHAR_2: memoryCapacityViewer(); inscreen = 0; break;
      case KEY_CHAR_3: while(tres==1) { tres = addinManager(); } inscreen = 0; break;
      case KEY_CHAR_4: changeFKeyColor(); inscreen = 0; break;
      case KEY_CTRL_EXIT: inscreen = 0; break;
    }
  }
}

//////////////////////////////////////////
// END OF GENERIC TOOLS
//////////////////////////////////////////
// START OF CLOCK UNADJUSTMENT DETECTION
//////////////////////////////////////////
int RTCunadjusted() {
  //returns 1 if the RTC is unadjusted.
  //this function finds out if the clock is unadjusted if its date/time is set to a previous date/time than 1st Jan 2013 00:00
  //   (because that date has passed on all timezones and of course no adjusted clock will have it).
  //this function is only meant to be useful for detecting if the RTC performed a reset, e.g. after taking out the batteries.
  //in that case, the RTC usually resets to some date around 2010. So this function will serve its purpose.
  if(1356998400000 > currentUnixTime()) {
    return 1;
  } return 0;
}
void RTCunadjustedWizard() {
  //first check if RTC is unadjusted. if not, return.
  if(!RTCunadjusted()) return;
  
  int textX = 0;
  int textY = 0;
  int key;
  Bdisp_AllClr_VRAM();
  if (setting_display_statusbar == 1) DisplayStatusArea();
  
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
        Bdisp_AllClr_VRAM();
        if (setting_display_statusbar == 1) DisplayStatusArea();
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
        setTime();
        Bdisp_AllClr_VRAM();
        if (setting_display_statusbar == 1) DisplayStatusArea();
        PrintXY(1, 1, (char*)"  Clock unadjusted", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
        textY = 24+17; textX = 0;
        PrintMini(&textX, &textY, (unsigned char*)"Now let's set the date.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        textY = textY + 17+17; textX = 0;
        PrintMini(&textX, &textY, (unsigned char*)"Press any key to start.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        mGetKey(&key);
        setDate();
        Bdisp_AllClr_VRAM();
        if (setting_display_statusbar == 1) DisplayStatusArea();
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
        return;
        break;
      case KEY_CTRL_EXIT:
        return;
    }
  }
}
//////////////////////////////////////////
// END OF CLOCK UNADJUSTMENT DETECTION
//////////////////////////////////////////
// START OF FIRST-RUN WIZARD
//////////////////////////////////////////
void firstRunWizard() {
  int inscreen = 1;
  int textX = 0;
  int textY = 0;
  int key;
  float alpha = 1;
  PrintMini(&textX, &textY, (unsigned char*)"tny. internet media", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  int textLen = textX;
  textX = LCD_WIDTH_PX/2 - textLen/2;
  int black, orange;
  const unsigned char text1[100] = "in colaboration with";
  const unsigned char text2[100] = "[too many people to list here]";
  const unsigned char text3[100] = "present";
  const unsigned char text4[100] = "Utilities";
//intro:
  DrawFrame( 0xFFFFFF  );
  Bdisp_AllClr_VRAM();
  while (alpha>=0) {
    drawtnyimLogo( LCD_WIDTH_PX/2-138/2, LCD_HEIGHT_PX/2-42/2, alpha);
    black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
    orange = drawRGB24toRGB565(alphaBlend(255, 210, alpha), alphaBlend(255, 68, alpha), alphaBlend(255, 19, alpha));
    textY = LCD_HEIGHT_PX/2+42/2-24;
    textX = LCD_WIDTH_PX/2 - textLen/2;
    PrintMini(&textX, &textY, (unsigned char*)"tny. ", 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"i", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"nternet ", 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"m", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"edia", 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    Bdisp_PutDisp_DD();
    blockForTicks(5);
    alpha=alpha-0.05;
  }
  blockForTicks(128*2);
  Bdisp_AllClr_VRAM();
  alpha=0;
  while (alpha<=1) {
    drawtnyimLogo( LCD_WIDTH_PX/2-138/2, LCD_HEIGHT_PX/2-42/2, alpha);
    black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
    orange = drawRGB24toRGB565(alphaBlend(255, 210, alpha), alphaBlend(255, 68, alpha), alphaBlend(255, 19, alpha));
    textY = LCD_HEIGHT_PX/2+42/2-24;
    textX = LCD_WIDTH_PX/2 - textLen/2;
    PrintMini(&textX, &textY, (unsigned char*)"tny. ", 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"i", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"nternet ", 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"m", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)"edia", 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    Bdisp_PutDisp_DD();
    blockForTicks(5);
    alpha=alpha+0.05;
  }
  Bdisp_AllClr_VRAM();
  alpha=1;
  textX=0;
  PrintMini(&textX, &textY, (unsigned char*)text1, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  textLen = textX; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)text2, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  int text2Len = textX;
  while (alpha>=0) {
    black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
    textY = LCD_HEIGHT_PX/2-24-12;
    textX = LCD_WIDTH_PX/2 - textLen/2;
    PrintMini(&textX, &textY, (unsigned char*)text1, 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    textY = LCD_HEIGHT_PX/2-24+12;
    textX = LCD_WIDTH_PX/2 - text2Len/2;
    PrintMini(&textX, &textY, (unsigned char*)text2, 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    Bdisp_PutDisp_DD();
    blockForTicks(5);
    alpha=alpha-0.05;
  }
  blockForTicks(128+64);
  Bdisp_AllClr_VRAM();
  alpha=0;
  while (alpha<=1) {
    black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
    textY = LCD_HEIGHT_PX/2-24-12;
    textX = LCD_WIDTH_PX/2 - textLen/2;
    PrintMini(&textX, &textY, (unsigned char*)text1, 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    textY = LCD_HEIGHT_PX/2-24+12;
    textX = LCD_WIDTH_PX/2 - text2Len/2;
    PrintMini(&textX, &textY, (unsigned char*)text2, 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    Bdisp_PutDisp_DD();
    blockForTicks(5);
    alpha=alpha+0.05;
  }  
  Bdisp_AllClr_VRAM();
  alpha=1;
  textX=0;
  PrintMini(&textX, &textY, (unsigned char*)text3, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  textLen = textX; textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)text4, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  text2Len = textX;
  int blue;
  while (alpha>=0) {
    black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
    blue = drawRGB24toRGB565(alphaBlend(255, 14, alpha), alphaBlend(255, 106, alpha), alphaBlend(255, 255, alpha));
    textY = LCD_HEIGHT_PX/2-24-12;
    textX = LCD_WIDTH_PX/2 - textLen/2;
    PrintMini(&textX, &textY, (unsigned char*)text3, 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    textY = LCD_HEIGHT_PX/2-24+12;
    textX = LCD_WIDTH_PX/2 - text2Len/2;
    PrintMini(&textX, &textY, (unsigned char*)text4, 0, 0xFFFFFFFF, 0, 0, blue, COLOR_WHITE, 1, 0);
    Bdisp_PutDisp_DD();
    blockForTicks(5);
    alpha=alpha-0.05;
  }
  blockForTicks(128*3);
  Bdisp_AllClr_VRAM();
  alpha=0;
  while (alpha<=1) {
    black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
    blue = drawRGB24toRGB565(alphaBlend(255, 14, alpha), alphaBlend(255, 106, alpha), alphaBlend(255, 255, alpha));
    textY = LCD_HEIGHT_PX/2-24-12;
    textX = LCD_WIDTH_PX/2 - textLen/2;
    PrintMini(&textX, &textY, (unsigned char*)text3, 0, 0xFFFFFFFF, 0, 0, black, COLOR_WHITE, 1, 0);
    textY = LCD_HEIGHT_PX/2-24+12;
    textX = LCD_WIDTH_PX/2 - text2Len/2;
    PrintMini(&textX, &textY, (unsigned char*)text4, 0, 0xFFFFFFFF, 0, 0, blue, COLOR_WHITE, 1, 0);
    Bdisp_PutDisp_DD();
    blockForTicks(5);
    alpha=alpha+0.05;
  }    
screen1:
  inscreen = 1;
  Bdisp_AllClr_VRAM();
  while(inscreen) {
    if (setting_display_statusbar == 1) DisplayStatusArea();
    
    int iresult;
    GetFKeyPtr(0x04A3, &iresult); // Next
    FKey_Display(5, (int*)iresult);
    
    PrintXY(1, 1, (char*)"  Welcome to Utilities", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    textY = 24; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"This add-in provides functionality", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"not originally present on Casio Prizm", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"(fx-CG 10/20) calculators:", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 22; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)" - Clock, stopwatch and timer", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)" - Calendar with agenda and tasklist", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)" - Fine timeout&backlight adjustment", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)" - Calculator code lock", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);    
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
        inscreen = 0;
        break;
    }
  }
  /////
screen2:
  inscreen = 1;
  while(inscreen) {
    Bdisp_AllClr_VRAM();
    if (setting_display_statusbar == 1) DisplayStatusArea();

    int iresult;
    GetFKeyPtr(0x036F, &iresult); // <
    FKey_Display(0, (int*)iresult);
    GetFKeyPtr(0x04A3, &iresult); // Next
    FKey_Display(5, (int*)iresult);
    
    PrintXY(1, 1, (char*)"  Welcome to Utilities", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
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
        goto screen1;
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_F6:
        inscreen = 0;
        break;
    }
  }
  /////
screen3:
  inscreen = 1;
  while(inscreen) {
    Bdisp_AllClr_VRAM();
    if (setting_display_statusbar == 1) DisplayStatusArea();

    int iresult;
    GetFKeyPtr(0x036F, &iresult); // <
    FKey_Display(0, (int*)iresult);
    GetFKeyPtr(0x04A3, &iresult); // Next
    FKey_Display(5, (int*)iresult);
    
    PrintXY(1, 1, (char*)"  Welcome to Utilities", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    textY = 24; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"Important notes:", 0, 0xFFFFFFFF, 0, 0, COLOR_ORANGE, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"There is a calculator lock function,", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"that enables you to lock your", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
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
        goto screen2;
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_F6:
        inscreen = 0;
        break;
    }
  }
  /////
//screen4:
  inscreen = 1;
  while(inscreen) {
    Bdisp_AllClr_VRAM();
    if (setting_display_statusbar == 1) DisplayStatusArea();

    int iresult;
    GetFKeyPtr(0x036F, &iresult); // <
    FKey_Display(0, (int*)iresult);
    GetFKeyPtr(0x04A4, &iresult); // Finish
    FKey_Display(5, (int*)iresult);
    
    PrintXY(1, 1, (char*)"  Welcome to Utilities", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    textY = 24; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"Thanks for reading this welcome", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"message. After selecting Finish, you", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"may be warned that the clock is", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"unadjusted. In that case, you'll be", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"guided to adjust it.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 20; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"You can change this add-in's settings", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 17; textX = 0;
    PrintMini(&textX, &textY, (unsigned char*)"by pressing Shift and then Menu.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    textY = textY + 20; textX = 0;
    PrintMiniMini( &textX, &textY, (unsigned char*)"Settings are saved in the Main Memory.", 0, TEXT_COLOR_BLACK, 0 );
    mGetKey(&key); //do a managed GetKey now so that users can go into Settings.
    switch(key)
    {
      case KEY_CTRL_F1:
        goto screen3;
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_F6:
        SaveSettings(SETTINGSFILE); //initializes the settings file and disables the first run flag
        inscreen = 0;
        break;
    }
  }
}
//////////////////////////////////////////
// END OF FIRST-RUN WIZARD
//////////////////////////////////////////

//////////////////////////////////////////
// START OF MASTER-CONTROL
//////////////////////////////////////////
//Master-control is using for controlling add-in and calculator functionality by entering numeric codes.
//It is meant for easy debugging of things that aren't yet in a nice UI, available to the user.
//(even because some of these things aren't meant to be available to the common user as they can be things like unknown syscalls testing)
int testTimer = 0;
int curTestValue = 0;
void testTimerHandler() {

}

void masterControl() {
  char buffer[100] = ""; // Some string length
  int start = 0; // Used for scrolling left and right
  int cursor = 0; // Cursor position

  buffer[0] = '\0'; // This sets the first character to \0, also represented by "", an empty string
  
  SetSetupSetting( (unsigned int)0x14, 0); //we only accept numbers, so switch off alpha/shift
  DisplayMBString((unsigned char*)buffer, start, cursor, 1, 8);

  
  Cursor_SetFlashOn(0);
  int key;
  while(1)
  {
    GetKey(&key);
    if(key == KEY_CTRL_EXIT) {
      Cursor_SetFlashOff();
      return;
    }
    if(key == KEY_CTRL_EXE) {
        Cursor_SetFlashOff();
        if (strcmp(buffer, "4520000") == 0) {
          settingsMenu();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520001") == 0) {
          setTime();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520002") == 0) {
          setDate();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520003") == 0) {
          setTimeFormat();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520004") == 0) {
          setLongDateFormat();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520005") == 0) {
          setDateFormat();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520006") == 0) {
          if (setting_black_theme == 1) setting_black_theme = 0; else setting_black_theme = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520007") == 0) {
          if (setting_display_statusbar == 1) setting_display_statusbar = 0; else setting_display_statusbar = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520008") == 0) {
          if (setting_show_advanced == 1) setting_show_advanced = 0; else setting_show_advanced = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520009") == 0) {
          if (setting_display_fkeys == 1) setting_display_fkeys = 0; else setting_display_fkeys = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520010") == 0) {
          setStartupBrightness();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520011") == 0) {
          if (setting_password_show_last_char == 1) setting_password_show_last_char = 0; else setting_password_show_last_char = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4520012") == 0) {
          if (setting_show_events_count_on_calendar == 1) setting_show_events_count_on_calendar = 0; else setting_show_events_count_on_calendar = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;          
        } else if (strcmp(buffer, "4529001") == 0) {
          if (setting_enable_lock_func == 1) setting_enable_lock_func = 0; else setting_enable_lock_func = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4529002") == 0) {
          setting_is_first_run = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4529003") == 0) { //enable/disable debug. See the definition for the setting for more info
          if (setting_debug_on == 1) setting_debug_on = 0; else setting_debug_on = 1;
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4529900") == 0) {
          SaveSettings(SETTINGSFILE, 0); //force no first-run overriding
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4529901") == 0) {
          LoadSettings(SETTINGSFILE);
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4529902") == 0) {
          MCSDelVar2(DIRNAME, SETTINGSFILE);
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4530000") == 0) {
          timeMenu();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4530001") == 0) {
          calendarScreen();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4540000") == 0) {
          powerMenu();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4540001") == 0) {
          changePowerTimeout();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4540002") == 0) {
          changeBacklightTimeout();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4540003") == 0) {
          changeBacklightLevel();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4540004") == 0) {
          setCPUclock();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4540005") == 0) {
          powerInformation();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "4550000") == 0) {
          lightMenu();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "5000000") == 0) {
          showAbout();
          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else if (strcmp(buffer, "9009001") == 0) {

          DisplayStatusArea();
          DefineStatusMessage((char*)"OK", 1, 0, 0);
          break;
        } else {
          DisplayStatusArea();
          DefineStatusMessage((char*)"", 1, 0, 0);
        }
        buffer[0] = '\0'; start = 0; cursor = 0;
        DisplayMBString((unsigned char*)buffer, start, cursor, 1, 8);
        Cursor_SetFlashOn(0);
    }
    if(key && key < 30000) {
      cursor = EditMBStringChar((unsigned char*)buffer, 100, cursor, key);
      DisplayMBString((unsigned char*)buffer, start, cursor, 1, 8);
    } else {
      EditMBStringCtrl((unsigned char*)buffer, 100, &start, &cursor, &key, 1, 8);
    }
  }
  Cursor_SetFlashOff();

}
//////////////////////////////////////////
// END OF MASTER-CONTROL
//////////////////////////////////////////
void managedLockCalc() {
  if (setting_enable_lock_func) {
    int lockres = lockCalc(setting_display_statusbar, setting_password_show_last_char, setting_lock_auto_turnoff);
    if(lockres == 0) {
      if (setting_unlock_runmat == 1) { APP_RUNMAT(0,0); }
      else if (setting_unlock_runmat == 2) {
        MsgBoxPush(4);
        PrintXY(3, 2, (char*)"  Open Run-Mat?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY(3, 4, (char*)"     Yes:[F1]/[1]", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY(3, 5, (char*)"     No :Other key", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        int key,inscreen=1;
        while(inscreen) {
          mGetKey(&key);
          switch(key)
          {
            case KEY_CTRL_F1:
            case KEY_CHAR_1:
              MsgBoxPop(); 
              APP_RUNMAT(0,0);
              break;
            default:
              inscreen=0;
              break;
          }
        }
        MsgBoxPop();
      }
    } 
  }
}
int main()
{
  unsigned short key = 0;
  unsigned short prevkey = 0;
  int keyCol; int keyRow; unsigned short wkey; //these aren't actually used, but they are needed to hold different getkey-like results
  int textmode = TEXT_MODE_TRANSPARENT_BACKGROUND;
  //Load settings
  LoadSettings(SETTINGSFILE);
  if (setting_display_statusbar == 1) {
    EnableStatusArea(0);
    DefineStatusAreaFlags(DSA_SETDEFAULT, 0, 0, 0);
    DefineStatusAreaFlags(3, SAF_BATTERY | SAF_TEXT | SAF_GLYPH | SAF_ALPHA_SHIFT, 0, 0);
  } else {
    EnableStatusArea(3);
  }
  // Set brightness if setting is set
  if (setting_startup_brightness != 250 && 0 <= setting_startup_brightness && setting_startup_brightness <= 249) {
    SetBacklightSubLevel_RAW(setting_startup_brightness);
  }
  
  LoadAllTimers(); //so that timers completion can be checked on the main screen too
  LoadAllStopwatches();
  //install and start user timer for passive "timer X finished" message on statusbar, at any point in the add-in, or blinking message whenever passiveTimerEndedMessage is 0
  int checkTimer = Timer_Install(0, checkAllTimersEndNoActive, 500);
  if (checkTimer > 0) { Timer_Start(checkTimer); }
  
  //install and start user timer for incrementing a value every half-second.
  //this is used for blinking the screen on active timer ended message, and flashlight, because the timer of GetKeyWait goes broken when there's an user timer.
  //this also allows for blinking the screen faster than one second.
  //this timer is enabled and disabled as it is needed.
  halfSecondTimer = Timer_Install(0, halfSecondTimerHandler, 500);
  
  Bdisp_EnableColor(1); 
  if(setting_is_first_run) firstRunWizard();
  RTCunadjustedWizard();
  while (1) {
    passiveTimerEndedMessage = 0; //here we want active
    checkAllTimersEnd();
    Bdisp_AllClr_VRAM();
    Bdisp_EnableColor(1); 

    //black theme, or not?
    if (setting_black_theme == 1) {
      Bdisp_Fill_VRAM( COLOR_BLACK, 3 );
      textmode = TEXT_MODE_INVERT;
      DrawFrame( 0x000000  );
    } else {
      textmode = TEXT_MODE_TRANSPARENT_BACKGROUND;
      DrawFrame( 0xFFFFFF  );
    }
    if (setting_display_statusbar == 1) {
      DisplayStatusArea();
      DefineStatusMessage((char*)"", 1, 0, 0);
    }
    
    char timeStr[14] = "  "; //two spaces for printxy...

    // Print time     
    if (setting_timeformat == 0) { //24 hour
      fillTime(timeStr + 2,0);
      PrintXY(8,3,timeStr, textmode, TEXT_COLOR_BLACK);
    } else { //12 hour
      fillTime(timeStr + 2,1);
      PrintXY(6,3,timeStr, textmode, TEXT_COLOR_BLACK);
    }
    if (setting_black_theme == 1) {
      drawLongDate(90, setting_longdateformat, COLOR_WHITE, COLOR_BLACK, 1);
      if (setting_display_statusbar) {
        darkenStatusbar();

      }
    } else {
      drawLongDate(90);
    }

    //Show FKeys
    if (setting_display_fkeys) {
      int iresult;
      GetFKeyPtr(0x043A, &iresult); // POWER
      FKey_Display(0, (int*)iresult);
      GetFKeyPtr(0x043E, &iresult); // LIGHT
      FKey_Display(1, (int*)iresult);
      GetFKeyPtr(0x012A, &iresult); // TIME
      FKey_Display(2, (int*)iresult);
      GetFKeyPtr(0x011C, &iresult); // TOOL
      FKey_Display(3, (int*)iresult);
      if (setting_enable_lock_func) {
        GetFKeyPtr(0x04D3, &iresult); // key icon (lock)
        FKey_Display(4, (int*)iresult);
      }
      if (setting_black_theme) {
        darkenFkeys((setting_enable_lock_func == 1 ? 5 : 4));
      }
    }

    Bdisp_PutDisp_DD();
    if (0 != GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key) ) {
      passiveTimerEndedMessage = 1; //key was pressed and we may go out of this screen, so turn on passive messages
      key = PRGM_GetKey();
      switch (key) {
        case KEY_PRGM_SHIFT:
          //turn on/off shift manually because getkeywait doesn't do it
          if (GetSetupSetting( (unsigned int)0x14) == 0) { SetSetupSetting( (unsigned int)0x14, 1); }
          else { SetSetupSetting( (unsigned int)0x14, 0); }
          break;
        case KEY_PRGM_MENU:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            settingsMenu();
          } else {
            //trigger Menu manually, as PRGM_GetKey does not do it
            GetKeyWait_OS(&keyCol, &keyRow, 2, 1, 0, &wkey);
          }
          break;
        case KEY_PRGM_ACON:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            if (setting_display_statusbar == 1) DisplayStatusArea();
            PowerOff(1);
            SetSetupSetting( (unsigned int)0x14, 0);
            if (setting_display_statusbar == 1) DisplayStatusArea();
          }
          break;
        case KEY_PRGM_F1:
          powerMenu();
          break;
        case KEY_PRGM_F2:
          lightMenu();
          break;
        case KEY_PRGM_F3:
          timeMenu();
          break;
        case KEY_PRGM_F4:
          toolsMenu();
          break;
        case KEY_PRGM_F5:
          managedLockCalc();
          break;
        case KEY_PRGM_F6:
          break;
        case 71: //KEY_PRGM_0, which is not defined in the SDK and I'm too lazy to add it every time I update the includes folder...
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            if (setting_display_statusbar == 1) DisplayStatusArea();
            masterControl();
          }
          break;
        case KEY_PRGM_RETURN:
          if(setting_lock_on_exe) {
            managedLockCalc();
          }
          break;
        default:
          break;
      }
      if (key!=prevkey && key!=KEY_PRGM_SHIFT) { SetSetupSetting( (unsigned int)0x14, 0); }
    }
  }
}

