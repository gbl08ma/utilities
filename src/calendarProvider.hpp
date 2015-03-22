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
#define MAX_DAY_EVENTS_WEEKVIEW 100 //max same-day events to load in week view
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
  //unsigned int daterange=0; //not used for now
  EventDate startdate;
  EventDate enddate;
  //unsigned int dayofweek=1; // not used for now
  unsigned int repeat=0;
  unsigned int timed=1; //full-day = 0, timed = 1
  EventTime starttime;
  EventTime endtime;
  char title[24]; //can't be 21, because otherwise somehow the location will replace the last chars of title
  char location[134]; //can't be 128, because otherwise somehow the description may flow into the location.
  char description[1030]; //orig 1024
} CalendarEvent;

typedef struct // a simplified calendar event, for use when the memory available is little and one needs to show many events
{
  unsigned int category=0;
  EventDate startdate;
  EventTime starttime;
  unsigned int origpos=0; //position in original file (useful for search results). zero based.
  unsigned char title[24];
} SimpleCalendarEvent;
// end of type definitions

void calEventToChar(CalendarEvent* calEvent, char* buf);
void charToCalEvent(char* src, CalendarEvent* calEvent);
void charToSimpleCalEvent(char* src, SimpleCalendarEvent* calEvent);
void smemFilenameFromDate(EventDate* date, unsigned short* filename, const char* folder);
int AddEvent(CalendarEvent* calEvent, const char* folder, int secondCall=0);
int ReplaceEventFile(EventDate *startdate, CalendarEvent* newEvents, const char* folder, int count);
void RemoveEvent(EventDate *startdate, CalendarEvent* events, const char* folder, int count, int calEventPos);
void RemoveDay(EventDate* date, const char* folder);
int GetEventsForDate(EventDate* startdate, const char* folder, CalendarEvent* calEvents, int limit=0, SimpleCalendarEvent* simpleCalEvents = NULL);
void GetEventCountsForMonth(int year, int month, int* buffer, int* busydays);
int SearchEventsOnDay(EventDate* date, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit);
int SearchEventsOnYearOrMonth(int y, int m, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit);
void repairEventsFile(char* name, const char* folder, int* checkedevents, int* problemsfound);
void setDBneedsRepairFlag(int value);
int getDBneedsRepairFlag();

#endif
