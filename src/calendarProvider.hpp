#ifndef __CALENDARPROVIDER_H
#define __CALENDARPROVIDER_H

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

#define MAX_DAY_EVENTS 100 //max events in one day
#define MAX_EVENT_FILESIZE 50000

#define FILE_HEADER "PCALEVT" //Prizm CALendar EVenT / Portable CALendar EVenT
#define FIELD_SEPARATOR '\x1D'
#define EVENT_SEPARATOR '\x1E'

typedef struct // Event date definition
{
  unsigned int day=0;
  unsigned int month=0;
  unsigned int year=0;
} EventDate;

typedef struct // Event time definition
{
  unsigned int hour=0;
  unsigned int minute=0;
  unsigned int second=0;
} EventTime;

typedef struct // Defines what a calendar event contains
{
  unsigned int category=0;
  unsigned int daterange=0;
  EventDate startdate;
  EventDate enddate;
  unsigned int dayofweek=1;
  unsigned int repeat=0;
  unsigned int timed=1; //full-day = 0, timed = 1
  EventTime starttime;
  EventTime endtime;
  unsigned char title[25]; //can't be 21, because otherwise somehow the location will replace the last chars of title
  unsigned char location[135]; //can't be 128, because otherwise somehow the description may flow into the location.
  unsigned char description[1030]; //orig 1024
} CalendarEvent;

typedef struct // a simplified calendar event, for use when the memory available is little and one needs to show many events
{
  unsigned int category=0;
  EventDate startdate;
  unsigned char title[25];
  unsigned int origpos=0; //position in original file (useful for search results). zero based.
} SimpleCalendarEvent;
// end of type definitions

void append(unsigned char* s, char c);
void calEventToChar(CalendarEvent* calEvent, unsigned char* buf);
void charToCalEvent(unsigned char* src, CalendarEvent* calEvent);
void filenameFromDate(EventDate* date, char* filename);
int AddEvent(CalendarEvent* calEvent, const char* folder);
//int EditEvent(EventDate* startdate, int calEventPos, const char* folder, CalendarEvent* editedEvent);
int ReplaceEventFile(EventDate *startdate, CalendarEvent* newEvents, const char* folder, int count);
int RemoveEvent(EventDate* startdate, int calEventPos, const char* folder);
int RemoveDay(EventDate* date, const char* folder);
int GetEventsForDate(EventDate* startdate, const char* folder, CalendarEvent* calEvents, int limit=0);
void GetEventCountsForMonth(int year, int month, int* buffer);
void toggleTaskActivity(CalendarEvent* event, int pos, int count);
int SearchEventsOnDay(EventDate* date, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit);

unsigned char *toksplit(unsigned char *src, char tokchar, unsigned char *token, int lgh);
#endif
