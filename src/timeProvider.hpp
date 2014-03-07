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

int getMonthDays(int month);
int getMonthDaysWithLeap(int month, int year);
const char* getCurrentDOWAsString();
const char* getCurrentDOWAsShortString();
const char* getCurrentMonthAsString();
const char* getDOWAsString(int dow);
const char* getDOWAsShortString(int dow);
const char* getMonthAsString(int month);
bool isLeap(int y);
int dow(int y, int m, int d);
int getDayOfYear(int y, int m, int d);
int getWeekNumber(int y, int m, int d);

int getCurrentYear();
int getCurrentMonth();
int getCurrentDay();
int getCurrentHour();
int getCurrentMinute();
int getCurrentSecond();
int getCurrentMillisecond();

long long int DateTime2Unix(long long int year, long long int month, long long int day, long long int hour, long long int minute, long long int second, long long int millisecond);

int getRTCisUnadjusted();

void currentDateToString(char *buffer, int format=GetSetting(SETTING_DATEFORMAT));
void dateToString(char *buffer, int y, int m, int d, int format=GetSetting(SETTING_DATEFORMAT));
void currentTimeToString(char *buffer, int format=GetSetting(SETTING_TIMEFORMAT));
void timeToString(char *buffer, int h, int min, int sec, int format=GetSetting(SETTING_TIMEFORMAT), int showSeconds=1);

long long int currentUnixTime(); // please note, this is milliseconds from epoch and not seconds

void setTime(int hour, int minute, int second);
void setDate(int year, int month, int day);

void blockForTicks(int ticks);
void blockForMilliseconds(int ms);
int getMSdiff(int prevticks, int newticks);

int isTimeValid(int h, int m, int s);
int isDateValid(int y, int m, int d);

void DaysToDate(long int d, long int* eyear, long int* emonth, long int* eday);
long int DateToDays(int y, int m, int d);

void stringToDate(char* string, int* yr, int* m, int *d, int format = GetSetting(SETTING_DATEFORMAT));
void stringToTime(char* string, int* h, int* m, int *s);

const char* dateSettingToInputDisplay(int setting = GetSetting(SETTING_DATEFORMAT));
#endif