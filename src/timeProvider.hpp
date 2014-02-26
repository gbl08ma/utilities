#ifndef __TIMEPROVIDER_H
#define __TIMEPROVIDER_H

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

#include "settingsProvider.hpp"

// Thanks to AHelper and http://prizm.cemetech.net/index.php?title=RTC_Unit for these
#define RYRCNT  (unsigned short*)0xA413FECE
#define RMONCNT (unsigned char*)0xA413FECC
#define RDAYCNT (unsigned char*)0xA413FECA
 
#define RHRCNT  (unsigned char*)0xA413FEC6
#define RMINCNT (unsigned char*)0xA413FEC4
#define RSECCNT (unsigned char*)0xA413FEC2
#define RWKCNT (unsigned char*)0xA413FEC8
#define RCR2 (unsigned char*)0xA413FEDE

short getMonthDays(short month);
short getMonthDaysWithLeap(short month, short year);
const char* getCurrentDOWAsString();
const char* getCurrentDOWAsShortString();
const char* getCurrentMonthAsString();
const char* getDOWAsString(short dow);
const char* getDOWAsShortString(short dow);
const char* getMonthAsString(short month);
bool isLeap(short y);
short dow(short y, short m, short d);
int getDayOfYear(short y, short m, short d);
short getWeekNumber(short y, short m, short d);

short getCurrentYear();
short getCurrentMonth();
short getCurrentDay();
short getCurrentHour();
short getCurrentMinute();
short getCurrentSecond();
short getCurrentMillisecond();

long long int DateTime2Unix(long long int year, long long int month, long long int day, long long int hour, long long int minute, long long int second, long long int millisecond);

short getRTCisUnadjusted();

void currentDateToString(char *buffer, short format=GetSetting(SETTING_DATEFORMAT));
void dateToString(char *buffer, short y, short m, short d, short format=GetSetting(SETTING_DATEFORMAT));
void currentTimeToString(char *buffer, short format=GetSetting(SETTING_TIMEFORMAT));
void timeToString(char *buffer, short h, short min, short sec, short format=GetSetting(SETTING_TIMEFORMAT), short showSeconds=1);

long long int currentUnixTime(); // please note, this is milliseconds from epoch and not seconds

void setTime(short hour, short minute, short second);
void setDate(short year, short month, short day);

void blockForTicks(int ticks);
void blockForMilliseconds(int ms);
int getMSdiff(int prevticks, int newticks);

short isTimeValid(short h, short m, short s);
short isDateValid(short y, short m, short d);

void DaysToDate(long int d, long int* eyear, long int* emonth, long int* eday);
long int DateToDays(short y, short m, short d);

void stringToDate(char* string, short* yr, short* m, short *d, short format = GetSetting(SETTING_DATEFORMAT));
void stringToTime(char* string, short* h, short* m, short *s);

const char* dateSettingToInputDisplay(short setting=-1);
#endif