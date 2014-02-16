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
void viewCalendar(int dateselection=0);
int viewMonthCalendar(int dateselection=0);
int viewWeekCalendar();
int viewWeekCalendarSub(Menu* menu, int* y, int* m, int* d, int* jumpToSel, int* keepMenuSel);
void viewEvents(int y, int m, int d);
void viewEvent(CalendarEvent* event, int istask=0);
int viewEventsSub(Menu* menu, int y, int m, int d);
void fillInputDate(int yr, int m, int d, char* buffer);
void fillInputTime(int h, int m, int s, char* buffer);
int eventEditor(int y, int m, int d, int type=EVENTEDITORTYPE_ADD, CalendarEvent* event=NULL, int istask=0);
void drawCalendar(int year, int month, int d, int show_event_count, int* eventcount, int* busydays, int* bufmonth, int* bufyear);
int moveEvent(CalendarEvent* events, int count, int pos, int isCopy=0);
int deleteEventUI(int y, int m, int d, CalendarEvent* events, int count, int pos, int istask=0);
int deleteAllEventUI(int y, int m, int d, int istask=0);
int chooseCalendarDate(int *yr, int *m, int *d, char* message, char* message2, int graphical = 0);
void invalidFieldMsg(int istime);
void setEventChrono(CalendarEvent* event);
int changeEventCategory(CalendarEvent* event);
void viewNthEventOnDay(EventDate* date, int pos);
void searchEventsGUI(int y=0, int m=0, int d=0);
void viewBusyMap(int type, int y, int m, int d);
void drawDayBusyMap(EventDate* thisday, int startx, int starty, int width, int height, int showHourMarks, int isWeek, int maxx);
void drawWeekBusyMap(int y, int m, int d, int startx, int starty, int width, int height);
void calendarTools(int y, int m, int d);
void repairCalendarDatabase();
void trimCalendarDatabase();
void importCalendarEvents();

#endif