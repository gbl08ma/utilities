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
void viewCalendar();
void viewEvents(int y, int m, int d);
void viewEvent(CalendarEvent* event);
int viewEventsSub(Menu* menu, int y, int m, int d);
void fillInputDate(int yr, int m, int d, char* buffer);
void fillInputTime(int h, int m, int s, char* buffer);
int eventEditor(int y, int m, int d, int type=EVENTEDITORTYPE_ADD, CalendarEvent* event=NULL, int istask=0);
void drawCalendar(int year, int month, int d, int show_event_count, int* eventcount, int* bufmonth, int* bufyear);
void copyEvent(CalendarEvent* event);
void moveEvent(CalendarEvent* event, int pos);
void deleteEventUI(int y, int m, int d, int pos, int istask=0);
void deleteAllEventUI(int y, int m, int d, int istask=0);
int chooseCalendarDate(int *yr, int *m, int *d, char* message, char* message2);
void invalidFieldMsg(int istime);
void setEventChrono(CalendarEvent* event);

#endif