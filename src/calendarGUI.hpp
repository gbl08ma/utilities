#ifndef __CALENDARGUI_H
#define __CALENDARGUI_H

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

#include "menuGUI.hpp"
#include "calendarProvider.hpp"

#define EVENTEDITORTYPE_ADD 0
#define EVENTEDITORTYPE_EDIT 1

#define EVENTEDITOR_RETURN_EXIT 0
#define EVENTEDITOR_RETURN_CONFIRM 1
#define EVENTDELETE_RETURN_EXIT 0
#define EVENTDELETE_RETURN_CONFIRM 1
void viewCalendar(short dateselection=0);
short viewMonthCalendar(short dateselection=0);
short viewWeekCalendar();
short viewWeekCalendarSub(Menu* menu, short* y, short* m, short* d, short* jumpToSel, short* keepMenuSel);
void viewEvents(int y, int m, int d);
void viewEvent(CalendarEvent* event, int istask=0);
int viewEventsSub(Menu* menu, int y, int m, int d);
void fillInputDate(int yr, int m, int d, char* buffer);
void fillInputTime(int h, int m, int s, char* buffer);
int eventEditor(int y, int m, int d, int type=EVENTEDITORTYPE_ADD, CalendarEvent* event=NULL, int istask=0);
void drawCalendar(int year, int month, int d, int show_event_count, short* eventcount, short* busydays, short* bufmonth, short* bufyear);
int moveEvent(CalendarEvent* events, int count, int pos, int isCopy=0);
int deleteEventUI(int y, int m, int d, CalendarEvent* events, int count, int pos, int istask=0);
int deleteAllEventUI(int y, int m, int d, int istask=0);
short chooseCalendarDate(short *yr, short *m, short *d, char* message, char* message2, short graphical = 0);
void invalidFieldMsg(short istime);
void setEventChrono(CalendarEvent* event);
int changeEventCategory(CalendarEvent* event);
void viewNthEventOnDay(EventDate* date, int pos);
void searchEventsGUI(int y, int m, int d);
void viewBusyMap(int type, int y, int m, int d);
void drawDayBusyMap(EventDate* thisday, int startx, int starty, int width, int height, int showHourMarks, int isWeek, int maxx);
void drawWeekBusyMap(int y, int m, int d, int startx, int starty, int width, int height);
void calendarTools(int y, int m, int d);
void repairCalendarDatabase();
void trimCalendarDatabase();
void importCalendarEvents();

#endif