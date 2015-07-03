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
int viewWeekCalendarChild(Menu* menu, int* y, int* m, int* d, int* jumpToSel, int* keepMenuSel);
void viewEvents(int y, int m, int d);
void viewEvent(CalendarEvent* event);
int viewEventsChild(Menu* menu, int y, int m, int d);
void fillInputDate(EventDate* date, char* buffer);
void fillInputTime(EventTime* t, char* buffer);
int eventEditor(int y, int m, int d, int type=EVENTEDITORTYPE_ADD, CalendarEvent* event=NULL, int istask=0);
void drawCalendar(int year, int month, int d, int show_event_count, int* eventcount, int* busydays, int* bufmonth, int* bufyear);
int moveEventScreen(CalendarEvent* events, int count, int pos, int isCopy=0);
int deleteEventPrompt(CalendarEvent* events, int count, int pos, int istask=0);
int deleteAllEventsPrompt(int y, int m, int d, int istask=0);
int selectDateScreen(int *yr, int *m, int *d, char* message, char* message2, int graphical = 0);
void invalidFieldMsg(int istime);
void setReminderScreen(CalendarEvent* event);
int setCategoryPrompt(CalendarEvent* event);
void viewEventAtPos(EventDate* date, int pos);
void searchEventsScreen(int y, int m, int d);
void drawBusymapDay(EventDate* thisday, int startx, int starty, int width, int height, int showHourMarks, int isWeek, int maxx);
void drawBusymapWeek(int y, int m, int d, int startx, int starty, int width, int height);
void calendarToolsMenu(int y, int m, int d);
void databaseRepairScreen();
void databaseTrimScreen();
void eventImportScreen();

#endif