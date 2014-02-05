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
#include "constantsProvider.hpp"
#include "settingsProvider.hpp"

const char *dayofweek[] = {"Sunday",
                           "Monday",
                           "Tuesday",
                           "Wednesday",
                           "Thursday",
                           "Friday",
                           "Saturday"
                          };
const char *dayofweekshort[] = {"Sun",
                                "Mon",
                                "Tue",
                                "Wed",
                                "Thu",
                                "Fri",
                                "Sat"
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

// RTC, CLOCK and CALENDAR CODE

unsigned int bcd_to_2digit(unsigned char* bcd) {
  return ((((*(bcd))&0xf0)>>4)*10) + ((*(bcd))&0x0f);
}
int getMonthDays(int month) {
  return monthDays[month-1]; 
}
const char* getCurrentDOWAsString()
{
    return dayofweek[(*RWKCNT & 0b111)];
}

const char* getCurrentDOWAsShortString()
{
    return dayofweekshort[(*RWKCNT & 0b111)];
}

const char* getCurrentMonthAsString()
{
    return monthNames[((*RMONCNT & 0b10000)>>4)*10 + (*RMONCNT & 0b1111) - 1];
}
const char* getMonthAsString(int month) {
  return monthNames[month-1]; 
}
const char* getDOWAsString(int dow) {
  return dayofweek[dow-1]; 
}
const char* getDOWAsShortString(int dow) {
  return dayofweekshort[dow-1]; 
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

int getDayOfYear(int y, int m, int d) {
  // year must be provided because of leap years
  int days = 0;
  for(int i = 1; i<m; i++) {
    if(i!=2) days+=getMonthDays(i);
    else days+=(isLeap(y)? 29 : 28);
  }
  return days+d;
}
int getWeekNumber(int y, int m, int d) {
  int julian = getDayOfYear(y,m,d);
  // since this is only for display purposes, we can mix settings here:
  if(GetSetting(SETTING_WEEK_START_DAY)==1) {
    if(julian>1) julian--;
    else { y--; julian = getDayOfYear(y,m,d); }
  }
  int dowk = dow(y,m,d);
  int dowkJan1 = dow(y,1,1);
  int weekNum = ((julian + 6) / 7);
  if (dowk < dowkJan1)
      weekNum++;
  return weekNum;
}

int getCurrentYear() {
  return ((*RYRCNT >> 12) & 0b1111)*1000 + ((*RYRCNT >> 8) & 0b1111)*100 + ((*RYRCNT >> 4) & 0b1111)*10 + (*RYRCNT & 0b1111);
}
int getCurrentMonth() {
  return ((*RMONCNT & 0b10000)>>4)*10 + (*RMONCNT & 0b1111);
}
int getCurrentDay() {
  return ((*RDAYCNT >> 4) & 0b11)*10 + (*RDAYCNT & 0b1111);
}
int getCurrentHour() {
  return bcd_to_2digit(RHRCNT); 
}
int getCurrentMinute() {
  return bcd_to_2digit(RMINCNT);
}
int getCurrentSecond() {
  return bcd_to_2digit(RSECCNT);
}
int getCurrentMillisecond() {
  unsigned int ihour, iminute, isecond, imillisecond;
  RTC_GetTime( &ihour, &iminute, &isecond, &imillisecond );
  return (int)imillisecond;
}

// Info and license for the following routine:
// Script Library Contribution by Flennan Roffo
// Logic Scripted Products & Script Services
// Peacock Park (183,226,69)
// (c) 2007 - Flennan Roffo
//
// Distributed as GPL, donated to wiki.secondlife.com on 19 sep 2007
// Modified by gbl08ma for use in the Casio Prizm Utilities add-in
#define DAYS_PER_YEAR 365
#define SECONDS_PER_DAY 86400
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60
long long int DateTime2Unix(long long int year, long long int month, long long int day, long long int hour, long long int minute, long long int second, long long int millisecond)
{
  long long int time = 0;
  long long int yr = 1970;
  long long int mt = 1;
  long long int days;

  while(yr < year)
  {
    days = (isLeap(yr++) ? DAYS_PER_YEAR + 1 : DAYS_PER_YEAR);
    time += days * SECONDS_PER_DAY*1000;
  }

  while (mt < month)
  {
    if(mt!=2) days = getMonthDays(mt++);
    else { days = (isLeap(year)? 29 : 28); mt++; }
    time += days * SECONDS_PER_DAY*1000;
  }

  days = day - 1;
  time += days * SECONDS_PER_DAY*1000;
  time += hour * SECONDS_PER_HOUR*1000;
  time += minute * SECONDS_PER_MINUTE*1000;
  time += second*1000;
  time += millisecond;

  return time;
}

int getRTCisUnadjusted() {
  //returns 1 if the RTC is unadjusted.
  //this function finds out if the clock is unadjusted if its date/time is set to a previous date/time than 1st Jan 2013 00:00:00.000
  //   (because that date has passed on all timezones and of course no adjusted clock will have it).
  //this function is only meant to be useful for detecting if the RTC performed a reset, e.g. after taking out the batteries.
  //in that case, the RTC usually resets to some date around 2010. So this function will serve its purpose.
  if((long long int)KNOWN_PAST_TIMESTAMP > currentUnixTime()) {
    return 1;
  } return 0;
}
void currentDateToString(char *buffer, int format) {
  dateToString(buffer, getCurrentYear(), getCurrentMonth(), getCurrentDay(), format);
}
void dateToString(char *buffer, int y, int m, int d, int format)
{
  char day[2];
  char month[2];
  char year[6];
  itoa(y, (unsigned char*)year);
  itoa(m, (unsigned char*)month);
  itoa(d, (unsigned char*)day);
  strcpy(buffer, (char*)"");
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

void currentTimeToString(char *buffer, int format)
{
  timeToString(buffer, getCurrentHour(), getCurrentMinute(), getCurrentSecond(), format, GetSetting(SETTING_CLOCK_SECONDS));
}
void timeToString(char *buffer, int h, int min, int sec, int format, int showSeconds)
{
  char hour[2];
  char minute[2];
  char second[2];
  itoa(min, (unsigned char*)minute);
  itoa(sec, (unsigned char*)second);

  int pm = 0;
  if(format && h >= 12) { 
    pm = 1;
    h = h - 12;
    if (h == 0) h = 12;
  }
  itoa(h, (unsigned char*)hour);
  if (h < 10) { strcat(buffer, "0"); }
  strcat(buffer, hour);
  strcat(buffer, ":");
  
  if (min < 10) { strcat(buffer, "0"); }
  strcat(buffer, minute);
  if(showSeconds) {
    strcat(buffer, ":");
    
    if (sec < 10) { strcat(buffer, "0"); }
    strcat(buffer, second);
  }

  if(format) {
    if(pm) strcat(buffer, " PM");
    else strcat(buffer, " AM");
  }
}


long long int currentUnixTime() { // please note, this is milliseconds from epoch and not seconds
  unsigned int fhour=0,fminute=0,fsecond=0,millisecond=0;
  RTC_GetTime( &fhour, &fminute, &fsecond, &millisecond );
  return DateTime2Unix(getCurrentYear(), getCurrentMonth(), getCurrentDay(), getCurrentHour(), getCurrentMinute(), getCurrentSecond(), millisecond);
} 

void setTime(int hour, int minute, int second) {
  char temp = *RCR2; //for rtc stopping/starting
  
  //stop RTC
  temp |= 0b10;
  temp &= 0b11111110;
  *RCR2 = temp;

  // hour
  *RHRCNT  = ((hour / 10) << 4) | (hour % 10);
  // minute
  *RMINCNT = ((minute / 10) << 4) | (minute % 10);
  // second
  *RSECCNT = ((second / 10) << 4) | (second % 10);


  // start RTC
  *RCR2 |= 1;
}

void setDate(int year, int month, int day) {
  char temp = *RCR2; //for rtc stopping/starting

  //stop RTC
  temp |= 0b10;
  temp &= 0b11111110;
  *RCR2 = temp;

  // set year
  *RYRCNT  = ((year / 1000) << 12) | (((year % 1000)/100) << 8) | (((year % 100) / 10) << 4) | (year % 10);
  // set month
  *RMONCNT = ((month / 10) << 4) | (month % 10);
  // set day
  *RDAYCNT = ((day / 10) << 4) | (day % 10);
  // set day of week
  *RWKCNT = dow(year, month, day) & 0b111;

  // start RTC
  *RCR2 |= 1;
}

void blockForTicks(int ticks) { //stop program execution for n ticks (1 tick = 1/128 s)
  int ot = RTC_GetTicks();
  while (RTC_GetTicks()-ot <= ticks) {}
}
void blockForMilliseconds(int ms) { //stop program execution for n milliseconds
  int ticks = (ms*128)/1000;
  blockForTicks(ticks);
}

int getMSdiff(int prevticks, int newticks) { // get difference in milliseconds between two amounts of ticks
  int tickdiff = newticks - prevticks;
  return (tickdiff*1000)/128;
}

int isTimeValid(int h, int m, int s) {
  if (h > 23 || h < 0 || m > 59 || m < 0 || s > 59 || s < 0) return 0;
  else return 1;
}

int isDateValid(int y, int m, int d) {
  if(y<=0 || y>9999) return 0; // otherwise we will have problems with calendar events and the like
  if (m > 12 || m <= 0 || d > ( m==2? (isLeap(y)? 29 : 28) : getMonthDays(m))) return 0;
  else return 1;
}

// The following two are used to calculate date differences in the calendar event copying/moving function

// converts an amount of days to a date
// result goes in the three last parameters
// converts an amount of days to a date
// result goes in the three last parameters
void DaysToDate(long day_number, long* year, long* month, long* day) {
  *year = ((long long)10000 * day_number + 14780) / (long long)3652425;
  long ddd = day_number - (365L * *year + *year / 4L - *year / 100 + *year / 400L);
  if (ddd < 0) {
    *year = *year - 1;
    ddd = day_number - (365L * *year + *year / 4L - *year / 100L + *year / 400L);
  }
  long mi = (100L * ddd + 52L) / 3060L;
  *month = (mi + 2) % 12 + 1;
  *year = *year + (mi + 2) / 12;
  *day = ddd - (mi * 306 + 5) / 10 + 1;
}

// converts a date to an amount of days.
// returns result. parameters stay untouched
long int DateToDays(int y, int m, int d)
{
  long int ly,lm;
  lm = ((long int)m + 9LL) % 12LL;
  ly = (long int)y - (long int)lm/10LL;
  return ly*365LL + ly/4LL - ly/100LL + ly/400LL + (lm*306LL + 5LL)/10LL + ( (long int)d - 1LL );
}

void stringToDate(char* string, int* yr, int* m, int *d, int format) {
  // string: a date in the format DDMMYYYY, MMDDYYYY or YYYYMMDD
  // if year < 1000 it has to be zero-padded.
  // result goes into arguments 2-4. format is the format to expect (values are the same as on the dateformat setting)
  // if format is not specified, it is read according to settings.
  // does not check if the date read is valid or if the string has the right size.
  char year[6] = "";
  char month[3] = "";
  char day[3] = "";
  switch(format == -1? GetSetting(SETTING_DATEFORMAT) : format) {
    case 0:
      day[0] = string[0]; day[1] = string[1]; day[2] = '\0';
      month[0] = string[2]; month[1] = string[3]; month[2] = '\0';
      year[0] = string[4]; year[1] = string[5]; year[2] = string[6]; year[3] = string[7]; year[4] = '\0';
      break;
    case 1:
      day[0] = string[2]; day[1] = string[3]; day[2] = '\0';
      month[0] = string[0]; month[1] = string[1]; month[2] = '\0';
      year[0] = string[4]; year[1] = string[5]; year[2] = string[6]; year[3] = string[7]; year[4] = '\0';
      break;
    case 2:
      day[0] = string[6]; day[1] = string[7]; day[2] = '\0';
      month[0] = string[4]; month[1] = string[5]; month[2] = '\0';
      year[0] = string[0]; year[1] = string[1]; year[2] = string[2]; year[3] = string[3]; year[4] = '\0';
      break;
  }

  *yr = sys_atoi(year);
  *m = sys_atoi(month);
  *d = sys_atoi(day);
}

void stringToTime(char* string, int* h, int* m, int *s) {
  char hour[3] = "";
  char minute[3] = "";
  char second[3] = "";
  hour[0] = string[0]; hour[1] = string[1]; hour[2] = '\0';
  minute[0] = string[2]; minute[1] = string[3]; minute[2] = '\0';
  second[0] = string[4]; second[1] = string[5]; second[2] = '\0';

  *h = sys_atoi(hour);
  *m = sys_atoi(minute);
  *s = sys_atoi(second);
}
const char *dateSettingInput[] = {"DDMMYYYY",
                                  "MMDDYYYY",
                                 "YYYYMMDD"
                                 };
const char* dateSettingToInputDisplay(int setting) {
  if(setting==-1) setting = GetSetting(SETTING_DATEFORMAT);
  if(setting < 0 || setting > 2) return NULL;
  return dateSettingInput[setting];
}