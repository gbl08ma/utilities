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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>

#include "constantsProvider.hpp"
#include "timeProvider.hpp"
#include "chronoGUI.hpp"
#include "settingsProvider.hpp"
#include "graphicsProvider.hpp"
#include "selectorGUI.hpp"
#include "calendarGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "calendarProvider.hpp" 
#include "keyboardProvider.hpp"
#include "stringsProvider.hpp"
#include "fileProvider.hpp"
#include "settingsGUI.hpp"
#include "debugGUI.hpp"

int bufmonth = 0; //this is global so that it's easier to make the events count refresh
int searchValid = 0; // whether the last search results are valid or not. Set to zero when modifying events in any way.
int sy, sm, sd = 0; //date to which to switch the calendar, or result of date selection
int dateselRes = 0; //whether user aborted date selection (0) or not (1)
void viewCalendar(int dateselection) {
  int res = 0;
  int type = GetSetting(SETTING_DEFAULT_CALENDAR_VIEW);
  if(dateselection) type = 1; // week view can't act as date selector for now
  while(1) {
    if(!dateselection) {
      char message[100];
      stringToMini(message, (char*)"Press OPTN for more options");
      DefineStatusMessage((char*)message, 1, 0, 0);
    }
    if(type) res = viewMonthCalendar(dateselection);
    else res = viewWeekCalendar();
    if(res) type=!type; else return;
  }
}

int viewMonthCalendar(int dateselection) {
  //returns 1 to switch to weekly view
  //returns 0 to exit calendar
  int y = getCurrentYear();
  int m = getCurrentMonth();
  int d = getCurrentDay();
  if(!sy || !sm || !sd) {} else {
    y = sy;
    m = sm;
    d = sd;
    sy=0; sm=0; sd=0;
  }
  
  int menu = 1;
  int eventcount[32];
  int busydays[32];
  int bufyear = 0;
  while (1) {
    if(getDBneedsRepairFlag()) repairCalendarDatabase();
    if(y>9999||y<0) { y=getCurrentYear(); m=getCurrentMonth(); d=getCurrentDay(); } //protection: reset to today's date if somehow we're trying to access the calendar for an year after 9999, or before 0.
    drawCalendar(y,m,d, GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT), eventcount, busydays, &bufmonth, &bufyear);
    switch (menu) {
    case 1:
      // JUMP, VIEW, INSERT, DEL-ALL, SEARCH, SWAP [white]
      drawFkeyLabels(0x01FC);
      if(!dateselection) {
        drawFkeyLabels(-1, -1, 0x03B4, -1, 0x0187, 0x0102);
        if(!GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT) || eventcount[d]>0) {
          drawFkeyLabels(-1, 0x049F, -1, 0x0104);
        }
      }
      break;
    case 2:
      // |<< , << , >> , >>| , ORIGINAL, DATE
      drawFkeyLabels(0x0408, 0x0409, 0x040B, 0x040C, 0x0238, 0x015F);
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
          if(!(y==0 && m == 1)) m--;
          if (m == 0) {
              m = 12;
              y--;
          }
        } else if (menu == 1 && !dateselection && (!GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT) || eventcount[d]>0)) {
          viewEvents(y, m, d);
        }
        break;
      case KEY_CTRL_F3:
        if (menu == 2) {
          if (!(y == 9999 && m == 12)) m++;
          if (m == 13) {
              m = 1;
              y++;
          }
        } else if (menu == 1 && !dateselection) {
          int eventsOnDay = 0;
          if(GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT)) eventsOnDay = eventcount[d];
          else {
            EventDate thisday;
            thisday.day = d; thisday.month = m; thisday.year = y;
            eventsOnDay = GetEventsForDate(&thisday, CALENDARFOLDER, NULL);
          }
          if(eventsOnDay >= MAX_DAY_EVENTS) {
            AUX_DisplayErrorMessage( 0x2E );
          } else {
            if(EVENTEDITOR_RETURN_CONFIRM == eventEditor(y, m, d)) {
              bufmonth=0;//force calendar events to reload
            }
          }
        }
        break;
      case KEY_CTRL_F4:
        if (menu == 2) {
          if (y != 9999) y++;
        } else if (menu == 1 && !dateselection && (!GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT) || eventcount[d]>0)) {
          if(EVENTDELETE_RETURN_CONFIRM == deleteAllEventUI(y, m, d)) {
            bufmonth=0;//force calendar events to reload
          }
        }
        break;
      case KEY_CTRL_F5:
        if (menu == 2) {
          y = getCurrentYear();
          m = getCurrentMonth();
          d = getCurrentDay();
        } else if (menu == 1 && !dateselection) {
          searchEventsGUI(y, m, d);
          if(!sy || !sm || !sd) {} else {
            y = sy;
            m = sm;
            d = sd;
            sy=0; sm=0; sd=0;
          }
        }
        break;       
      case KEY_CTRL_F6:
        if (menu == 2) {
          DefineStatusMessage((char*)"", 1, 0, 0);
          //only update calendar if selection is clean
          if (0 == chooseCalendarDate(&ny, &nm, &nd, (char*)"Jump to specific date", (char*)"")) { y=ny;m=nm;d=nd; }
        }
        else if (menu == 1 && !dateselection) {
          sy = y;
          sm = m;
          sd = d;
          return 1;
        }
        break;
      case KEY_CTRL_OPTN:
        if(!dateselection) calendarTools(y, m, d);
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
            d += getMonthDaysWithLeap(m, y);
        }
        break;
      case KEY_CTRL_DOWN:
        d+=7;
        if (d > getMonthDaysWithLeap(m, y))
        {
            d -= getMonthDaysWithLeap(m, y);
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
            d = getMonthDaysWithLeap(m, y);
        }
        break;
      case KEY_CTRL_RIGHT:
        d++;
        if (d > getMonthDaysWithLeap(m, y))
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
        if(dateselection) {
          sy = y;
          sm = m;
          sd = d;
          dateselRes = 1;
          return 0;
        } else viewEvents(y, m, d);
        break;
      case KEY_CTRL_EXIT:
        if (menu == 2) { menu = 1; }
        else { dateselRes = 0; return 0; }
        break;
    }
    if (menu == 2) {
      if (d > getMonthDaysWithLeap(m, y)) d = getMonthDaysWithLeap(m, y);
    } 
  }
  return 0;
}

int viewWeekCalendar() {
  //returns 1 to switch to montly view
  //returns 0 to exit calendar
  int res=1;
  int y=0, m=0, d=0;
  Menu menu;
  
  menu.scrollout=1;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  menu.returnOnInfiniteScrolling=1;
  int jumpToSel=1;
  int keepMenuSel=0;
  while(res) {
    if(getDBneedsRepairFlag()) repairCalendarDatabase();
    if(!sy || !sm || !sd) {
      if(!y || !m || !d) {
        y = getCurrentYear();
        m = getCurrentMonth();
        d = getCurrentDay();
        jumpToSel=1;
      }
    } else {
      y = sy;
      m = sm;
      d = sd;
      sy=0; sm=0; sd=0;
      jumpToSel=1;
    }
    res = viewWeekCalendarSub(&menu, &y, &m, &d, &jumpToSel, &keepMenuSel);
    if(res==2) return 1;
  }
  return 0;
}

int viewWeekCalendarSub(Menu* menu, int* y, int* m, int* d, int* jumpToSel, int* keepMenuSel) {
  //returns 1 when it wants to be restarted (refresh tasks)
  //returns 0 if the idea really is to exit the screen
  //returns 2 to switch to month view
  int oy=*y,om=*m,od=*d; //backup originally requested date before modifying it
  // get first date of the week
  long int ddays=DateToDays(*y, *m, *d);
  while(dow(*y,*m,*d) != GetSetting(SETTING_WEEK_START_DAY)) { //find sunday/monday before provided date
    ddays = DateToDays(*y, *m, *d) - 1; //decrease day by 1
    long int ny, nm, nd;
    DaysToDate(ddays, &ny, &nm, &nd);
    *y=ny; *m=nm; *d=nd;
  }
  // y,m,d now has first Sunday/Monday of the week, depending on user setting
  // ddays has the corresponding amount of days
  // calculate week number for menu title accoring to the Sunday/Monday date
  char buffer[10];
  int wkn = getWeekNumber(*y,*m,*d);
  itoa(wkn, (unsigned char*)buffer);
  char menutitle[70];
  strcpy(menutitle, "Week ");
  strcat(menutitle, buffer);
  itoa(*y, (unsigned char*)buffer);
  strcat(menutitle, (char*)" of ");
  strcat(menutitle, buffer);
  menu->title = menutitle;
   
  unsigned int curday = 0; unsigned int numevents = 0;
  unsigned int fevcount[7];
  // get event count only, so we know how many SimpleCalendarEvents to alloc
  while(curday < 7) {
    long int ny, nm, nd;
    DaysToDate(ddays, &ny, &nm, &nd);
    EventDate date;
    date.year=ny; date.month=nm; date.day=nd;
    if(!isDateValid(ny,nm,nd)) {
      *y=0; // one of the dates we're trying to view is not valid (probably because the year is not valid, i.e. below 0 or above 9999).
      return 1; // reload with *y==0, this makes it go to today's date.
    }
    fevcount[curday] = GetEventsForDate(&date, CALENDARFOLDER, NULL, MAX_DAY_EVENTS_WEEKVIEW);
    numevents += fevcount[curday];
    ddays++;
    curday++;
  }
  // allocate SimpleCalendarEvents
  SimpleCalendarEvent* events = (SimpleCalendarEvent*)alloca(numevents*sizeof(SimpleCalendarEvent));
  // allocate MenuItems:
  MenuItem menuitems[numevents+7];
  
  // read events this time
  unsigned int curmenu = 0;
  unsigned int cursce = 0; // current SimpleCalendarEvent, in the end it will have the SimpleCalendarEvent count
  ddays=DateToDays(*y, *m, *d);
  char menuitemtext[7][42];
  for(curday=0; curday < 7; curday++) {
    // one menuitem for day header
    long int ny, nm, nd;
    DaysToDate(ddays, &ny, &nm, &nd);
    char buffer[15];
    dateToString(buffer, ny, nm, nd);
    // the following string only fits in the menuitemtext because:
    //  1 - graphically, it's printed with PrintMini
    //  2 - the text variable has a size of 42 bytes, to account for the possibility that all the items are multibyte characters.
    //  (in this case there are no MB chars, so the full 42 bytes can be used)
    if(fevcount[curday]) {
      if(GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT)) {
        char buffer[10];
        itoa(fevcount[curday], (unsigned char*)buffer);
        strcpy(menuitemtext[curday], buffer);
        strcat(menuitemtext[curday], " event");
        if(fevcount[curday]>1) strcat(menuitemtext[curday], "s");
        strcat(menuitemtext[curday], " on ");
      } else {
        strcpy(menuitemtext[curday], "Events for ");
      }
    }
    else strcpy(menuitemtext[curday], "No events for ");
    strcat(menuitemtext[curday], buffer);
    strcat(menuitemtext[curday], (char*)" (");
    strcat(menuitemtext[curday], getDOWAsString(dow(ny, nm, nd)+1));
    strcat(menuitemtext[curday], (char*)")");
    menuitems[curmenu].text = menuitemtext[curday];
    menuitems[curmenu].type = MENUITEM_SEPARATOR;
    if(ny==oy&&nm==om&&nd==od&&*jumpToSel==1) menu->selection=curmenu+1;
    curmenu++;
    if(fevcount[curday]) {
      EventDate date;
      date.year=ny; date.month=nm; date.day=nd;
      // read directly into SimpleCalendarEvents
      GetEventsForDate(&date, CALENDARFOLDER, NULL, MAX_DAY_EVENTS_WEEKVIEW, events, cursce);
      for(unsigned int k = 0; k < fevcount[curday]; k++) {
        // build menuitem
        menuitems[curmenu].text = (char*)events[cursce].title;
        menuitems[curmenu].color = events[cursce].category-1;
        curmenu++;
        cursce++;
      }
    }
    ddays++;
  }
  if(GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT) && GetSetting(SETTING_SHOW_CALENDAR_BUSY_MAP)) {
    // SETTING_SHOW_CALENDAR_BUSY_MAP is checked because the title only becomes minimini if the busy map is shown
    // if the title is not minimini, there's not enough space for the events count
    strcat(menutitle, " (");
    if(cursce) {
      char buffer[10];
      itoa((int)cursce, (unsigned char*)buffer);
      strcat(menutitle, buffer);
    } else strcat(menutitle, "No");
    strcat(menutitle, " event");
    if(cursce != 1) strcat(menutitle, "s");
    strcat(menutitle, ")");
  }
  menu->numitems = curmenu;
  menu->items = menuitems;
  if(*jumpToSel==0) {
    if(*keepMenuSel) *keepMenuSel=0;
    else menu->selection = 1;
  }
  int hasBusyMap = 0;
  unsigned short busyMapBuffer[LCD_WIDTH_PX*12];
  if(GetSetting(SETTING_SHOW_CALENDAR_BUSY_MAP)) {
    menu->miniMiniTitle = 1;
    clearLine(1, 1); //clear background for the map
    drawWeekBusyMap(*y, *m, *d, 0, 24+14, LCD_WIDTH_PX, 10);
    // backup busy map so we don't need to redraw it again every time its VRAM location gets overwritten.
    MsgBoxMoveWB(busyMapBuffer, 0, 12, LCD_WIDTH_PX-1, 23, 1);
    hasBusyMap=1;
  } else {
    menu->miniMiniTitle = 0;
  }
  while(1) {
    Bdisp_AllClr_VRAM();
    if(hasBusyMap) MsgBoxMoveWB(busyMapBuffer, 0, 12, LCD_WIDTH_PX-1, 23, 0);
    if(menu->fkeypage==0) {
      // JUMP, VIEW, DATE, INSERT, SEARCH, SWAP [white]
      drawFkeyLabels(0x01FC, 0x049F, 0x015F, 0x03B4, 0x0187, 0x0102);
    } else if(menu->fkeypage==1) {
      // |<< , << , >> , >>| , ORIGINAL, DATE
      drawFkeyLabels(0x0408, 0x0409, 0x040B, 0x040C, 0x0238, 0x015F);
    }
    int res = doMenu(menu);
    int msel = getMenuSelectionSeparators(menu, 1);
    int ssel = getMenuSelectionSeparators(menu, 0);
    switch(res) {
      case MENU_RETURN_EXIT:
        if(menu->fkeypage == 0) return 0;
        else menu->fkeypage = 0;
        break;
      case MENU_RETURN_SELECTION:
        if(msel>0) viewNthEventOnDay(&events[msel-1].startdate, events[msel-1].origpos);
        else if(ssel>0) {
          long int dd = DateToDays(*y, *m, *d) + ssel-1;
          long int ny, nm, nd;
          DaysToDate(dd, &ny, &nm, &nd);
          EventDate date;
          date.year = ny; date.month = nm; date.day = nd;
          searchValid = 1;
          viewEvents(date.year, date.month, date.day);
          if(!searchValid) {
            *y=ny; *m=nm; *d=nd;
            *jumpToSel=1;
            return 1;
          }
        }
        break;
      case MENU_RETURN_SCROLLING:
        if(menu->selection == 1) {
          ddays = DateToDays(*y, *m, *d) - 1; // decrease by one day (go to another week and jump to menu bottom)
          long int ny, nm, nd;
          DaysToDate(ddays, &ny, &nm, &nd);
          *y=ny; *m=nm; *d=nd; *jumpToSel=1;
          return 1;
        } else {
          ddays = DateToDays(*y, *m, *d) + 7; // increase by one week
          long int ny, nm, nd;
          DaysToDate(ddays, &ny, &nm, &nd);
          *y=ny; *m=nm; *d=nd; *jumpToSel=1;
          return 1;
        }
        break;
      case KEY_CTRL_F1:
        if(menu->fkeypage == 0) {
          menu->fkeypage = 1;
        } else if (menu->fkeypage == 1) {
          ddays = DateToDays(*y, *m, *d) - 23; // decrease by three weeks
          // do not decrease by one month as we will end up going back 5 weeks instead of four
          // (the algorithm looks for the latest Sunday/Monday before the selected date...)
          long int ny, nm, nd;
          DaysToDate(ddays, &ny, &nm, &nd);
          *y=ny; *m=nm; *d=nd; *jumpToSel=0;
          return 1;
        }
        break;
      case KEY_CTRL_F2:
        if(menu->fkeypage == 0) {
          if(menu->numitems > 0) {
            if(msel>0) viewNthEventOnDay(&events[msel-1].startdate, events[msel-1].origpos);
            else if(ssel>0) {
              long int dd = DateToDays(*y, *m, *d) + ssel-1;
              long int ny, nm, nd;
              DaysToDate(dd, &ny, &nm, &nd);
              EventDate date;
              date.year = ny; date.month = nm; date.day = nd;
              searchValid = 1;
              viewEvents(date.year, date.month, date.day);
              if(!searchValid) {
                *y=ny; *m=nm; *d=nd;
                *jumpToSel=1;
                return 1;
              }
            }
          } 
        } else if (menu->fkeypage == 1) {
          ddays = DateToDays(*y, *m, *d) - 7; // decrease by one week
          long int ny, nm, nd;
          DaysToDate(ddays, &ny, &nm, &nd);
          *y=ny; *m=nm; *d=nd; *jumpToSel=0;
          return 1;
        }
        break;
      case KEY_CTRL_F3:
        if(menu->fkeypage == 0) {
          if(msel>0) {
            searchValid = 1;
            viewEvents(events[msel-1].startdate.year, events[msel-1].startdate.month, events[msel-1].startdate.day);
            if(!searchValid) {
              *y=events[msel-1].startdate.year; *m=events[msel-1].startdate.month; *d=events[msel-1].startdate.day;
              *jumpToSel=1;
              return 1;
            }
          } else if(ssel>0) {
            long int dd = DateToDays(*y, *m, *d) + ssel-1;
            long int ny, nm, nd;
            DaysToDate(dd, &ny, &nm, &nd);
            EventDate date;
            date.year = ny; date.month = nm; date.day = nd;
            searchValid = 1;
            viewEvents(date.year, date.month, date.day);
            if(!searchValid) {
              *y=ny; *m=nm; *d=nd;
              *jumpToSel=1;
              return 1;
            }
          }
        } else if (menu->fkeypage == 1) {
          ddays = DateToDays(*y, *m, *d) + 7; // increase by one week
          long int ny, nm, nd;
          DaysToDate(ddays, &ny, &nm, &nd);
          *y=ny; *m=nm; *d=nd;
          return 1;
        }
        break;
      case KEY_CTRL_F4:
        if(menu->fkeypage == 0) {
          EventDate* date = NULL;
          EventDate rdate;
          if(msel>0) date = &events[msel-1].startdate;
          else if(ssel>0) {
            long int dd = DateToDays(*y, *m, *d) + ssel-1;
            long int ny, nm, nd;
            DaysToDate(dd, &ny, &nm, &nd);
            rdate.year = ny; rdate.month = nm; rdate.day = nd;
            date = &rdate;
          }
          int existcount = GetEventsForDate(date, CALENDARFOLDER, NULL);
          if(existcount >= MAX_DAY_EVENTS) {
            AUX_DisplayErrorMessage( 0x2E );
          } else {
            if(EVENTEDITOR_RETURN_CONFIRM == eventEditor(date->year, date->month, date->day)) {
              *jumpToSel=0;
              *keepMenuSel=1;
              return 1;
            }
          }
        } else if (menu->fkeypage == 1) {
          ddays = DateToDays(*y, *m, *d) + 30; // increase by one month.
          long int ny, nm, nd;
          DaysToDate(ddays, &ny, &nm, &nd);
          *y=ny; *m=nm; *d=nd; *jumpToSel=0;
          return 1;
        }
        break;
      case KEY_CTRL_F5:
        if(menu->fkeypage == 0) {
          int sey=0,sem=0,sed=0;
          if(msel>0) {
            sey=events[msel-1].startdate.year;
            sem=events[msel-1].startdate.month;
            sed=events[msel-1].startdate.day;
          } else if(ssel>0) {
            long int dd = DateToDays(*y, *m, *d) + ssel-1;
            long int ny, nm, nd;
            DaysToDate(dd, &ny, &nm, &nd);
            sey=ny;
            sem=nm;
            sed=nd;
          } else return 1; //this should never happen
          searchValid=1;
          searchEventsGUI(sey, sem, sed);
          if(!sy || !sm || !sd) {
            if(!searchValid) return 1;
          } else {
            *jumpToSel=1;
            return 1;
          }
        } else if (menu->fkeypage == 1) {
          *y=0; // forces a return to today's values
          return 1;
        }
        break;
      case KEY_CTRL_F6:
        if(menu->fkeypage == 0) {
          if(msel>0) { // do not lose selection precision if possible
            sy = events[msel-1].startdate.year;
            sm = events[msel-1].startdate.month;
            sd = events[msel-1].startdate.day;
          } else {
            if(ssel>0) {
              long int dd = DateToDays(*y, *m, *d) + ssel-1;
              long int ny, nm, nd;
              DaysToDate(dd, &ny, &nm, &nd);
              sy = ny;
              sm = nm;
              sd = nd;
            }
          }
          return 2;
        } else if (menu->fkeypage == 1) {
          int ny=0,nm=0,nd=0;
          DefineStatusMessage((char*)"", 1, 0, 0);
          if (0 == chooseCalendarDate(&ny, &nm, &nd, (char*)"Jump to specific date", (char*)"")) {
            *y=ny;*m=nm;*d=nd;
            *jumpToSel=1; //if user inserted a specific day, it makes sense to display that day without scrolling.
            return 1;
          } 
        }
        break;
      case KEY_CTRL_FORMAT:
        if(menu->numitems > 0 && msel>0) {
          int ne = GetEventsForDate(&events[msel-1].startdate, CALENDARFOLDER, NULL);
          // we can use alloca here, as we're going to return right after
          CalendarEvent* ce = (CalendarEvent*)alloca(ne*sizeof(CalendarEvent));
          GetEventsForDate(&events[msel-1].startdate, CALENDARFOLDER, ce);
          if(EVENTEDITOR_RETURN_CONFIRM == changeEventCategory(&ce[events[msel-1].origpos])) {
            ReplaceEventFile(&events[msel-1].startdate, ce, CALENDARFOLDER, ne);
          }
          *jumpToSel=0;
          *keepMenuSel=1;
          return 1; // return even if user aborted, because we used alloca inside a loop (leak waiting to happen)
        }
        break;
      case KEY_CTRL_OPTN:
        long int ny, nm, nd;
        if(msel>0) { // do not lose selection precision if possible
          ny = events[msel-1].startdate.year;
          nm = events[msel-1].startdate.month;
          nd = events[msel-1].startdate.day;
        } else {
          if(ssel>0) {
            long int dd = DateToDays(*y, *m, *d) + ssel-1;
            DaysToDate(dd, &ny, &nm, &nd);
          }
        }
        searchValid = 1;
        calendarTools(ny, nm, nd);
        if(!searchValid) {
          *y=ny; *m=nm; *d=nd;
          *jumpToSel=1;
          return 1;
        }
        break;
    }
  }
  return 1;
}

void viewEvents(int y, int m, int d) {
  int res=1;
  Menu menu;
  
  menu.scrollout=1;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  menu.nodatamsg = (char*)"No events - press F2";
  char buffer[15];
  dateToString(buffer, y, m, d);
  menu.statusText = (char*)""; // to clear "Press OPTN..." message
  char menutitle[50];
  strcpy(menutitle, "Events for ");
  strcat(menutitle, buffer);
  strcat(menutitle, " (");
  strcat(menutitle, getDOWAsString(dow(y,m,d)+1)); // this will not show when the menu title is not minimini; this is intended
  strcat(menutitle, ")");
  menu.title = menutitle;
  while(res) {
    res = viewEventsSub(&menu, y, m, d);
  }
}

int viewEventsSub(Menu* menu, int y, int m, int d) {
  //returns 1 when it wants to be restarted (refresh tasks)
  //returns 0 if the idea really is to exit the screen
  EventDate thisday;
  thisday.day = d; thisday.month = m; thisday.year = y;  
  
  menu->numitems = GetEventsForDate(&thisday, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* events = (CalendarEvent*)alloca(menu->numitems*sizeof(CalendarEvent));
  MenuItem* menuitems = (MenuItem*)alloca(menu->numitems*sizeof(MenuItem));
  menu->numitems = GetEventsForDate(&thisday, CALENDARFOLDER, events);
  int curitem = 0;
  while(curitem < menu->numitems) {
    menuitems[curitem].text = (char*)events[curitem].title;
    menuitems[curitem].type = MENUITEM_NORMAL;
    menuitems[curitem].color = events[curitem].category-1;
    curitem++;
  }
  menu->items=menuitems;
  int hasBusyMap = 0;
  unsigned short busyMapBuffer[LCD_WIDTH_PX*12];
  if(GetSetting(SETTING_SHOW_CALENDAR_BUSY_MAP)) {
    menu->miniMiniTitle = 1;
    clearLine(1, 1); //clear background for the map
    drawDayBusyMap(&thisday, 0, 24+14, LCD_WIDTH_PX, 10, 2,0,0);
    // backup busy map so we don't need to redraw it again every time its VRAM location gets overwritten.
    MsgBoxMoveWB(busyMapBuffer, 0, 12, LCD_WIDTH_PX-1, 23, 1);
    hasBusyMap=1;
  } else {
    menu->miniMiniTitle = 0;
  }
  while(1) {
    Bdisp_AllClr_VRAM();
    if(hasBusyMap) MsgBoxMoveWB(busyMapBuffer, 0, 12, LCD_WIDTH_PX-1, 23, 0);
    
    if(!menu->numitems) menu->fkeypage = 0; //because if there are no events, 2nd menu is empty.
    if (menu->fkeypage == 0) {
      // VIEW, INSERT, EDIT, DELETE, DEL-ALL, Rotate FKey menu arrow
      drawFkeyLabels(-1, 0x03B4);
      if(menu->numitems>0) {
        drawFkeyLabels(0x049F, -1, 0x0185, 0x0038, 0x0104, 0x0006);
      }
    } else if (menu->fkeypage == 1) {
      if(menu->numitems>0) {
        // COPY, MOVE, MEMO
        drawFkeyLabels(0x038D, 0x04D2, 0x0470, -1, -1, 0x0006);
        // "hack" the MOVE label
        unsigned short fgc = Bdisp_GetPoint_VRAM(66, 195); // get color in which function keys are drawn
        VRAMReplaceColorInRect(67, 195, 58, 18, COLOR_WHITE, COLOR_GREEN);
        VRAMReplaceColorInRect(67, 195, 58, 18, fgc, COLOR_WHITE);
        VRAMReplaceColorInRect(67, 195, 58, 18, COLOR_GREEN, fgc);
        for(int x = 0; x < 5; x++) {
          drawLine(125-x, 214, 126, 213-x, COLOR_WHITE);
        }
      }
    }
    if(menu->selection > menu->numitems) menu->selection = menu->numitems;
    if(menu->selection < 1) menu->selection = 1;
    int res = doMenu(menu);
    switch(res) {
      case MENU_RETURN_EXIT:
        return 0;
        break;
      case KEY_CTRL_F1:
        if(menu->fkeypage == 0) { if(menu->numitems > 0) viewEvent(&events[menu->selection-1]); }
        else if (menu->fkeypage == 1) {
          if(moveEvent(events, menu->numitems, menu->selection-1, 1) == EVENTEDITOR_RETURN_CONFIRM) {
            bufmonth = 0; searchValid = 0;
            return 1;
          }
        }
        break;
      case MENU_RETURN_SELECTION:
        viewEvent(&events[menu->selection-1]);
        break;
      case KEY_CTRL_F2:
        if(menu->fkeypage == 0) {
          if(menu->numitems >= MAX_DAY_EVENTS) {
            AUX_DisplayErrorMessage( 0x2E );
          } else {
            if(EVENTEDITOR_RETURN_CONFIRM == eventEditor(y, m, d)) {
              bufmonth=0; searchValid = 0; return 1;
            }
          }
        } else if (menu->fkeypage == 1) {
          if(moveEvent(events, menu->numitems, menu->selection-1) == EVENTEDITOR_RETURN_CONFIRM) {
            bufmonth=0; searchValid = 0;
            return 1;
          }
        }
        break;
      case KEY_CTRL_F3:
        if(menu->fkeypage == 0) {
          if(menu->numitems > 0) {
            if(eventEditor(y, m, d, EVENTEDITORTYPE_EDIT, &events[menu->selection-1]) == EVENTEDITOR_RETURN_CONFIRM) {
              ReplaceEventFile(&events[menu->selection-1].startdate, events, CALENDARFOLDER, menu->numitems);
              searchValid = 0; bufmonth = 0;
            }
            return 1; //even if the user didn't confirm the changes, we have them in our event list. so we need to reload it to its unmodified state.
          }
        } else if (menu->fkeypage == 1) {
          setEventChrono(&events[menu->selection-1]);
        }
        break;
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
        if(menu->fkeypage == 0 && menu->numitems > 0) {
          if(EVENTDELETE_RETURN_CONFIRM == \
            (res == KEY_CTRL_F4 ? deleteEventUI(y, m, d, events, menu->numitems, menu->selection-1) : deleteAllEventUI(y, m, d, 0))) {
            bufmonth = 0; searchValid = 0; return 1;
          }
        }
        break;
      case KEY_CTRL_F6:
        if(menu->numitems > 0) menu->fkeypage = !menu->fkeypage;
        break;
      case KEY_CTRL_FORMAT:
        //the "FORMAT" key is used in many places in the OS to format e.g. the color of a field,
        //so on this add-in it is used to change the category (color) of a task/calendar event.
        if(menu->numitems > 0 && EVENTEDITOR_RETURN_CONFIRM == changeEventCategory(&events[menu->selection-1])) {
          ReplaceEventFile(&events[menu->selection-1].startdate, events, CALENDARFOLDER, menu->numitems);
          searchValid = 0; return 1;
        }
        break;
    }
  }
  return 1;
}

void viewEvent(CalendarEvent* event, int istask) {
  DefineStatusMessage((char*)"", 1, 0, 0); // clear "press OPTN for more options" message
  char catbuffer[10]="";
  textArea text;
  text.title = (char*)event->title;
  
  textElement elem[15];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor
  
  elem[text.numelements].text = (char*)"Location:";
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  elem[text.numelements].text = (char*)event->location;
  text.numelements++;
  
  if(!istask) {
    elem[text.numelements].text = (char*)"Starts on";
    elem[text.numelements].newLine = 1;
    elem[text.numelements].lineSpacing = 8;
    elem[text.numelements].spaceAtEnd=1;
    elem[text.numelements].color=COLOR_LIGHTGRAY;
    text.numelements++;
    
    unsigned char startson[50];
    dateToString((char*)startson, event->startdate.year, event->startdate.month, event->startdate.day);
    strcat((char*)startson, (char*)" ");
    
    if(event->timed) {
      char buffer[15]="";
      timeToString((char*)buffer, event->starttime.hour, event->starttime.minute, event->starttime.second);
      strcat((char*)startson, (char*)buffer);
    } else {
      strcat((char*)startson, (char*)"(all day)");
    }
    
    elem[text.numelements].text = (char*)startson;
    text.numelements++;
    
    elem[text.numelements].text = (char*)"Ends on";
    elem[text.numelements].newLine = 1;
    elem[text.numelements].spaceAtEnd=1;
    elem[text.numelements].color=COLOR_LIGHTGRAY; 
    text.numelements++;
    
    unsigned char endson[50];
    dateToString((char*)endson, event->enddate.year, event->enddate.month, event->enddate.day);
    
    if(event->timed) {
      char buffer[15]="";
      timeToString((char*)buffer, event->endtime.hour, event->endtime.minute, event->endtime.second);
      strcat((char*)endson, (char*)" ");
      strcat((char*)endson, (char*)buffer);
    }
    
    elem[text.numelements].text = (char*)endson;
    text.numelements++;
  }
  
  elem[text.numelements].text = (char*)"Category:";
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].newLine = 1;
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  itoa(event->category, (unsigned char*)catbuffer);
  elem[text.numelements].text = (char*)catbuffer;
  elem[text.numelements].color = textColorToFullColor((event->category == 0 ? 7 : event->category-1));
  text.numelements++;
  
  elem[text.numelements].text = (char*)"Description:";
  elem[text.numelements].newLine = 1;
  elem[text.numelements].lineSpacing = 8;
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  text.numelements++;
  
  elem[text.numelements].text = (char*) event->description;
  elem[text.numelements].newLine = 1;
  text.numelements++;

  doTextArea(&text);
}

void fillInputDate(int yr, int m, int d, char* buffer) {
  if(yr || m || d) {
    char buffer2[8];
    char day[5] = "";
    char month[5] = "";
    char year[5] = "";
    if (d < 10) { strcat(day, "0"); }
    itoa(d, (unsigned char*) buffer2);
    strcat(day, buffer2);
    
    if (m < 10) { strcat(month, "0"); }
    itoa(m, (unsigned char*) buffer2);
    strcat(month, buffer2);
    
    if (yr < 1000) { strcat(year, "0"); }
    if (yr < 100) { strcat(year, "0"); }
    if (yr < 10) { strcat(year, "0"); }
    itoa(yr, (unsigned char*) buffer2);
    strcat(year, buffer2);

    switch(GetSetting(SETTING_DATEFORMAT)) {
      case 0:
        strcpy(buffer, day);
        strcat(buffer, month);
        strcat(buffer, year);
        break;
      case 1:
        strcpy(buffer, month);
        strcat(buffer, day);
        strcat(buffer, year);
        break;
      case 2:
        strcpy(buffer, year);
        strcat(buffer, month);
        strcat(buffer, day);
        break;
    }
  }
}

void fillInputTime(int h, int m, int s, char* buffer) {
  buffer[0] = '\0';
  if(h || m || s) {
    char buffer2[8];
    if (h < 10) { strcat(buffer, "0"); }
    itoa(h, (unsigned char*) buffer2);
    strcat(buffer, buffer2);

    if (m < 10) { strcat(buffer, "0"); }
    itoa(m, (unsigned char*) buffer2);
    strcat(buffer, buffer2);
    
    if (s < 10) { strcat(buffer, "0"); }
    itoa(s, (unsigned char*) buffer2);
    strcat(buffer, buffer2);
  }
}
int eventEditor(int y, int m, int d, int type, CalendarEvent* event, int istask) {
  DefineStatusMessage((char*)"", 1, 0, 0); // clear "press OPTN for more options" message
  if(type == EVENTEDITORTYPE_ADD) {
    event = (CalendarEvent*)alloca(sizeof(CalendarEvent));
    
    event->startdate.day = d;
    event->startdate.month = m;
    event->startdate.year = y;
    
    event->enddate.day = 0;
    event->enddate.month = 0;
    event->enddate.year = 0;
    
    event->starttime.hour = 0;
    event->starttime.minute = 0;
    event->starttime.second = 0;
    
    event->endtime.hour = 0;
    event->endtime.minute = 0;
    event->endtime.second = 0;
    
    event->timed=0;
    
    //clean buffers:
    strcpy((char*)event->title, "");
    strcpy((char*)event->location, "");
    strcpy((char*)event->description, "");
  }
  if(istask) event->timed=0;
  int curstep = 0;
  while(1) {
    if (type == EVENTEDITORTYPE_ADD) {
      SetBackGround(0x0A);
      drawScreenTitle(istask ? (char*)"Add New Task" : (char*)"Add New Event");
    } else {
      SetBackGround(12);
      drawScreenTitle(istask ? (char*)"Edit Task" : (char*)"Edit Event");
    }
    clearLine(1,8); // SetBackGround already took care of filling the lines 1 through 7, and DisplayStatusArea will fill line 0
    if(curstep >= 3) {
      // disable keyboard modifiers, as user may have come from a text input field
      // where alpha-lock was enabled, not being disabled on F6.
      SetSetupSetting( (unsigned int)0x14, 0);
    }
    // < (first label) and Next or Finish (last label)
    drawFkeyLabels((curstep>0 ? 0x036F : -1), -1, -1, -1, -1, (curstep==6 ? 0x04A4 : 0x04A3));
    switch(curstep) {
      case 0:
        {
          drawScreenTitle(NULL, (char*)"Title:");
          
          textInput input;
          input.forcetext=1; //force text so title must be at least one char.
          input.charlimit=21;
          input.acceptF6=1;
          input.buffer = (char*)event->title;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) { curstep=curstep+1; break;} // continue to next step
          }
        }
        break;
      case 1:
        {
          drawScreenTitle(NULL, (char*)"Location:");
          
          textInput input;
          input.charlimit=128;
          input.acceptF6=1;
          input.buffer = (char*)event->location;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) { curstep=curstep+1; break;} // continue to next step
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
          }
        }
        break;
      case 2:
        {
          drawScreenTitle(NULL, (char*)"Description:");
          
          textInput input;
          input.charlimit=1024;
          input.acceptF6=1;
          input.buffer = (char*)event->description;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) {
              // continue to next step
              if(istask) curstep=6;
              else curstep=curstep+1;
              break;
            }
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
          }
        }
        break;
      case 3: {   
        drawScreenTitle(NULL, (char*)"Start time:");
        mPrintXY(8, 4, (char*)"HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        int textX = 0;
        int textY = 5*24;
        PrintMini(&textX, &textY, (char*)"If left blank, the event will be a", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        textX=0; textY += 17;
        PrintMini(&textX, &textY, (char*)"full-day event.", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        
        textInput input;
        input.x=8;
        input.width=6;
        input.charlimit=6;
        input.acceptF6=1;
        input.type=INPUTTYPE_TIME;
        char stbuffer[15] = "";
        if(event->timed) fillInputTime(event->starttime.hour, event->starttime.minute, event->starttime.second, stbuffer);
        input.buffer = (char*)stbuffer;
        while(1) {
          input.key=0;
          int res = doTextInput(&input);
          if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
          else if (res==INPUT_RETURN_CONFIRM) {
            int len = strlen(stbuffer);
            if(len == input.charlimit) {
                int h, m, s;
                stringToTime(stbuffer, &h, &m, &s);
                if(isTimeValid(h, m, s)) {
                  event->timed = 1;
                  event->starttime.hour = h;
                  event->starttime.minute = m;
                  event->starttime.second = s;
                  curstep=curstep+1; break; // continue to next step
                } else invalidFieldMsg(1);
            } else if (!len) {
              // user wants all-day event
              event->timed = 0;
              curstep=curstep+1; break; // next step
            } else invalidFieldMsg(1);
          } 
          else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
        }
        break;
      }
      case 4: {
        drawScreenTitle(NULL, (char*)"End date:");
        mPrintXY(7, 4, (char*)dateSettingToInputDisplay(), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        int textX = 0;
        int textY = 5*24;
        PrintMini(&textX, &textY, (char*)"If left blank, the event will end on", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        textX=0; textY += 17;
        PrintMini(&textX, &textY, (char*)"the same day that it starts.", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        
        textInput input;
        input.x=7;
        input.width=8;
        input.charlimit=8;
        input.acceptF6=1;
        input.type=INPUTTYPE_DATE;
        char edbuffer[15] = "";
        fillInputDate(event->enddate.year, event->enddate.month, event->enddate.day, edbuffer);
        input.buffer = (char*)edbuffer;
        while(1) {
          input.key=0;
          int res = doTextInput(&input);
          if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
          else if (res==INPUT_RETURN_CONFIRM) {
            int len = strlen(edbuffer);
            if(len == input.charlimit) {
              int yr,m,d;
              stringToDate(edbuffer, &yr, &m, &d);
              if(isDateValid(yr, m, d)) {
                long int datediff = DateToDays(yr, m, d) - DateToDays(event->startdate.year, event->startdate.month, event->startdate.day);
                if(datediff>=0) {
                  event->enddate.year = yr;
                  event->enddate.month = m;
                  event->enddate.day = d;
                  curstep=curstep+1; break; // continue to next step
                } else invalidFieldMsg(0);
              } else invalidFieldMsg(0);
            } else if (!len) {
              // user wants end date to be the same as the start date
              event->enddate.year = event->startdate.year;
              event->enddate.month = event->startdate.month;
              event->enddate.day = event->startdate.day;
              curstep=curstep+1; break; // next step
            } else invalidFieldMsg(0);
          } 
          else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
        }
        break;
      }
      case 5: {
        if(event->timed) {
          drawScreenTitle(NULL, (char*)"End time:");
          mPrintXY(8, 4, (char*)"HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          
          textInput input;
          input.x=8;
          input.width=6;
          input.charlimit=6;
          input.acceptF6=1;
          input.type=INPUTTYPE_TIME;
          char etbuffer[15] = "";
          fillInputTime(event->endtime.hour, event->endtime.minute, event->endtime.second, etbuffer);
          input.buffer = (char*)etbuffer;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) {
              if((int)strlen(etbuffer) == input.charlimit) {
                int h, m, s;
                stringToTime(etbuffer, &h, &m, &s);
                if(isTimeValid(h, m, s)) {
                  long int timediff = (h*60*60+m*60+s) - (event->starttime.hour*60*60+event->starttime.minute*60+event->starttime.second);
                  long int datediff = DateToDays(event->enddate.year, event->enddate.month, event->enddate.day) - DateToDays(event->startdate.year, event->startdate.month, event->startdate.day);
                  if(datediff > 0 || timediff >= 0) {
                    event->endtime.hour = h;
                    event->endtime.minute = m;
                    event->endtime.second = s;
                    curstep=curstep+1; break; // continue to next step
                  } else invalidFieldMsg(1);
                } else invalidFieldMsg(1);
              } else invalidFieldMsg(1);
            } 
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
          }
        } else curstep=6;
        break;
      }
      case 6: {
        drawScreenTitle(NULL, (char*)"Select category");
        mPrintXY(5, 4, (char*)"\xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
        mPrintXY(5, 6, (char*)"\xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
        int inscreen=1; int key = 0;
        char buffer[20];
        if(type == EVENTEDITORTYPE_ADD) event->category = 1;
        while(inscreen)
        {
          mPrintXY(5, 5, (char*)" ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
          itoa(event->category, (unsigned char*)buffer);
          mPrintXY(5, 5, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, (event->category <= 6 ? event->category-1 : TEXT_COLOR_YELLOW));
          mGetKey(&key);
          switch(key)
          {
            case KEY_CTRL_DOWN:
              if (event->category == 0) event->category = 7;
              else event->category--;
              break;
            case KEY_CTRL_UP:
              if (event->category == 7) event->category = 0;
              else event->category++;
              break;
            case KEY_CTRL_F1:
              if(istask) curstep = 2; // go to description
              else curstep=curstep-(event->timed?1:2);
              goto endofloop;
              break;
            case KEY_CTRL_EXE:
            case KEY_CTRL_F6:
              inscreen = 0;
              break;
            case KEY_CTRL_EXIT: return EVENTEDITOR_RETURN_EXIT;
          }
        }
        //event->daterange = 0;
        //event->dayofweek = dow(event->startdate.day, event->startdate.month, event->startdate.year);
        if(type == EVENTEDITORTYPE_ADD) {
          event->repeat = 0;
          int res = AddEvent(event, CALENDARFOLDER);
          if(res) {
            mMsgBoxPush(4);
            if (res == 4) {
              mPrintXY(3, 2, (char*)"Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            } else {
              if(istask) mPrintXY(3, 2, (char*)"Task add. ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              else mPrintXY(3, 2, (char*)"Event add. ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            }
            if(istask) mPrintXY(3, 3, (char*)"Task could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            else mPrintXY(3, 3, (char*)"Event could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 4, (char*)"be added.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            closeMsgBox();
            return EVENTEDITOR_RETURN_EXIT; // error is like exit
          }
          return EVENTEDITOR_RETURN_CONFIRM;
        } else {
          // if we're editing an event, do not save anything to storage and just act as an editor for what's in RAM.
          // saving to storage is something that must be dealt with after this function returns.
          return EVENTEDITOR_RETURN_CONFIRM;
        }
        endofloop:
        break;
      }
      default:
        return EVENTEDITOR_RETURN_EXIT; // non-existent step specified, return
    }
  }
}
inline static void PrintMiniFix( int x, int y, const char*Msg, const int flags, const short color, const short bcolor ) {
  int i=0;
  while (Msg[i]) {
    int textX = x;
    char sb[2] = {Msg[i], '\0'};
    PrintMini(&textX, &y, sb, 0, 0xFFFFFFFF, 0, 0, color, bcolor, 1, 0);
    x += 12;
    i++;
  }
}
void drawCalendar(int year, int month, int d, int show_event_count, int* eventcount, int* busydays, int* bufmonth, int* bufyear) {
  Bdisp_AllClr_VRAM();
  int textX = 0;
  int textY = 0;
#define TOP 25
#define BOTTOM (LCD_HEIGHT_PX - TOP-2)
#define LEFT 25
#define RIGHT (LCD_WIDTH_PX - 26)
#define THICKNESS 20
#define TOPOFFSET 22
#define WIDTH 47
  int categoryBackColors[10] = {0xffd8, 0xffd8, 0x7bff, 0x7fef, 0x7fff, 0xfbef, 0xfbff, 0xffef};
  drawLine(LEFT,TOP,RIGHT-1,TOP,COLOR_BLACK);
  drawLine(LEFT,TOP,LEFT,BOTTOM-1,COLOR_BLACK);
  drawLine(LEFT,BOTTOM-1,RIGHT-1,BOTTOM-1,COLOR_BLACK);
  drawLine(RIGHT-1,BOTTOM-1,RIGHT-1,TOP,COLOR_BLACK);
  drawRectangle(LEFT+2,TOP+2,RIGHT-2-2-LEFT,THICKNESS,COLOR_BLACK);
  textX=LEFT+5; textY= TOP+2-TOPOFFSET;
  PrintMini(&textX, &textY, getMonthAsString(month), 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
  int x,y = 0; int k = 1;
  drawRectangle(LEFT+2,TOP+2+THICKNESS,RIGHT-2-2-LEFT,THICKNESS,COLOR_LIGHTGRAY);
  drawRectangle(RIGHT-2-WIDTH,TOP+2+2*THICKNESS,WIDTH,THICKNESS*6,COLOR_LIGHTBLUE);
  drawRectangle(LEFT+2,TOP+2+2*THICKNESS,WIDTH,THICKNESS*6,COLOR_LIGHTBLUE);
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
    textX = x+2;
    textY = TOP+2+THICKNESS-TOPOFFSET;
    PrintMini(&textX, &textY, getDOWAsShortString(k++), 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_LIGHTGRAY, 1, 0);
  }

  int startingday = dow(year,month,1),day = 1;
  int prevstart = getMonthDays((month == 1 ? 12 : month - 1)) - (startingday == 0 ? 7 : startingday) + ((month == 3 && isLeap(year)) ? 2 : 1);
  char buffer[10];
  for (x = 0; x < startingday; x++)
  {
      itoa(prevstart++,(unsigned char*)buffer);
      PrintMiniFix(LEFT+2+x*WIDTH+2,TOP+2+2*THICKNESS-TOPOFFSET,buffer,0,(x == 0 ? COLOR_LIGHTSLATEGRAY : COLOR_LIGHTGRAY),(x == 0 ? COLOR_LIGHTBLUE : COLOR_WHITE));
  }
  x = startingday;
  y = 2;
  while (day <= getMonthDaysWithLeap(month, year))
  {
      itoa(day,(unsigned char*)buffer);
      if (day == d) {
        drawRectangle(LEFT+2+WIDTH*x+1,TOP+1+2+y*THICKNESS,WIDTH-1,THICKNESS-1,COLOR_RED);
        if(x == 0) drawRectangle(LEFT+2+WIDTH*x,TOP+1+2+y*THICKNESS,WIDTH,THICKNESS-1,COLOR_RED); //make sure the little pixels row on sundays is filled
      }
      PrintMiniFix(LEFT+2+WIDTH*x+2,TOP+2+y*THICKNESS-TOPOFFSET,buffer,0,(day == d ? COLOR_WHITE : COLOR_BLACK),(day == d ? COLOR_RED : (x == 0 || x == 6) ? COLOR_LIGHTBLUE : COLOR_WHITE));
      //events indicator:            
      if (show_event_count) {
        if (*bufmonth!=month || *bufyear!=year) { //events in buffer are not for this month, refresh.
          GetEventCountsForMonth(year, month, eventcount, busydays);
          *bufmonth = month; //update which month is now in buffer
          *bufyear = year; //update which year is now in buffer
        }
        if(eventcount[day] > 0) {
          int textX = LEFT+2+WIDTH*x+2+12*2+2; //12+2 to have space to write the day and some padding
          int textY = TOP+2+y*THICKNESS-TOPOFFSET+1; //+1 to have some padding
          char eventstr[10];
          if (eventcount[day] < 100) {
            itoa(eventcount[day], (unsigned char*)eventstr); 
          } else {
            strcpy(eventstr, (char*)"++");
          }
          PrintMiniMini( &textX, &textY, eventstr, 0, TEXT_COLOR_BLACK, 0 );
          if(day == d) {
            // color replacements for when background is red
            VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+1+24, 8*2, 12, COLOR_WHITE, COLOR_RED);
            VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+1+24, 8*2, 12, COLOR_BLACK, COLOR_WHITE);
          }
        }
        if(busydays[day] > 0 && day != d) {
          if(x == 0 || x == 6) {
            VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+1+24, 8*2, 12, COLOR_WHITE, categoryBackColors[busydays[day]]);
            VRAMReplaceColorInRect(LEFT+2+WIDTH*x, TOP+1+2+y*THICKNESS, WIDTH, THICKNESS-1, COLOR_LIGHTBLUE, categoryBackColors[busydays[day]]);
          } else {
            VRAMReplaceColorInRect(LEFT+2+WIDTH*x, TOP+1+2+y*THICKNESS, WIDTH, THICKNESS-1, COLOR_WHITE, categoryBackColors[busydays[day]]);
          }
        }
      }
      //end of events indicator
      
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
      PrintMiniFix(LEFT+2+WIDTH*x+2,TOP+2+y*THICKNESS-TOPOFFSET,buffer,0,((x == 0) | (x == 6)) ? COLOR_LIGHTSLATEGRAY : COLOR_LIGHTGRAY,(x == 0 || x == 6) ? COLOR_LIGHTBLUE : COLOR_WHITE);
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
  PrintMini(&textX, &textY, buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 0, 0);
  textX = RIGHT-4-textX;
  PrintMini(&textX, &textY, buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
}

int deleteEventUI(int y, int m, int d, CalendarEvent* events, int count, int pos, int istask) {
  EventDate date; date.day = d; date.month = m; date.year = y;
  mMsgBoxPush(4);
  mPrintXY(3, 2, (char*)"Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 3, (istask ? (char*)"Selected Task?" : (char*)"Selected Event?"), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  if(closeMsgBox(1, 4)) {
    RemoveEvent(&date, events, CALENDARFOLDER, count, pos);
    return EVENTDELETE_RETURN_CONFIRM;
  }
  return EVENTDELETE_RETURN_EXIT;
}

int deleteAllEventUI(int y, int m, int d, int istask) {
  EventDate date; date.day = d; date.month = m; date.year = y;
  mMsgBoxPush(4);
  if (istask) {
    mPrintXY(3, 2, (char*)"Delete All Tasks?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    mPrintXY(3, 2, (char*)"Delete All Events", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, (char*)"on Selected Day?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  if(closeMsgBox(1, 4)) {
    RemoveDay(&date, CALENDARFOLDER);
    return EVENTDELETE_RETURN_CONFIRM;
  }
  return EVENTDELETE_RETURN_EXIT;
}

int chooseCalendarDate(int *yr, int *m, int *d, char* message, char* message2, int graphical)
{ //returns 0 on success, 1 on user abort
  if(graphical) {
    DefineStatusMessage(message, 1, 0, 0);
    sy=*yr;
    sm=*m;
    sd=*d;
    viewCalendar(1);
    DefineStatusMessage((char*)"", 1, 0, 0);
    if(dateselRes) {
      *yr=sy;
      *m=sm;
      *d=sd;
    }
    sy = 0; //avoid jumping again
    return !dateselRes;
  } else {
    Bdisp_AllClr_VRAM();
    SetSetupSetting( (unsigned int)0x14, 0); //we only accept numbers, so switch off alpha/shift
    drawScreenTitle(message, message2);
    mPrintXY(1, 3, (char*)"Date: ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);  
    mPrintXY(6, 4, (char*)dateSettingToInputDisplay(), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    
    textInput input;
    input.x=6;
    input.width=8;
    input.charlimit=8;
    input.acceptF6=0;
    input.type=INPUTTYPE_DATE;
    char buffer[15] = "";
    fillInputDate(*yr, *m, *d, buffer);
    input.buffer = (char*)buffer;
    while(1) {
      input.key=0;
      int res = doTextInput(&input);
      if (res==INPUT_RETURN_EXIT) return 1; // user aborted
      else if (res==INPUT_RETURN_CONFIRM) {
        if((int)strlen(buffer) == input.charlimit) {
          stringToDate(buffer, yr, m, d);
          if(isDateValid(*yr, *m, *d)) {
            return 0;
          } else {
            invalidFieldMsg(0);
          }
        } else invalidFieldMsg(0);
      } 
    }
  }
}

int moveEvent(CalendarEvent* events, int count, int pos, int isCopy) {
  int ey=0, em=0, ed=0;
  if(!chooseCalendarDate(&ey, &em, &ed, (isCopy ? (char*)"Copy Event" : (char*)"Move Event"), (char*)"To following day:")) {
    if(ey == (signed)events[pos].startdate.year && em == (signed)events[pos].startdate.month && ed == (signed)events[pos].startdate.day) {
      return EVENTEDITOR_RETURN_EXIT; //destination date is same as current event date
    }
    EventDate oldstartdate;
    oldstartdate.day = events[pos].startdate.day;
    oldstartdate.month = events[pos].startdate.month;
    oldstartdate.year = events[pos].startdate.year;
    // update start date:
    events[pos].startdate.day = ed;
    events[pos].startdate.month = em;
    events[pos].startdate.year = ey;
    // calculate new end date based on the difference between the old begin date and the new begin date
    long int datediff = DateToDays(ey, em, ed) - DateToDays(oldstartdate.year, oldstartdate.month, oldstartdate.day);
    long int odatedays = DateToDays(events[pos].enddate.year, events[pos].enddate.month, events[pos].enddate.day);
    long int newdatedays = odatedays + datediff;
    //update end date with difference:
    long int nd,nm,ny;
    DaysToDate(newdatedays, &ny, &nm, &nd);
    events[pos].enddate.day = nd;
    events[pos].enddate.month = nm;
    events[pos].enddate.year = ny;
    // add event on new day
    if(GetEventsForDate(&events[pos].startdate, CALENDARFOLDER, NULL)+1 > MAX_DAY_EVENTS) {
      AUX_DisplayErrorMessage( 0x2E );
      return EVENTEDITOR_RETURN_EXIT; // do not keep running, as we'd delete the original event, even though the move was not sucessful.
    } else {
      //already checked if passes num limit
      int res = AddEvent(&events[pos], CALENDARFOLDER);
      if(res > 0) {
        mMsgBoxPush(4);
        if (res == 4) { //error on size check
          mPrintXY(3, 2, (char*)"Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        } else {
          mPrintXY(3, 2, (char*)"Event move ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        }
        mPrintXY(3, 3, (char*)"Event could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(3, 4, (isCopy ? (char*)"be copied." : (char*)"be moved."), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        closeMsgBox();
        return EVENTEDITOR_RETURN_EXIT;
      }
    }
    // delete event on current (old) day
    if(!isCopy) RemoveEvent(&oldstartdate, events, CALENDARFOLDER, count, pos);
    return EVENTEDITOR_RETURN_CONFIRM;
  }
  return EVENTEDITOR_RETURN_EXIT;
}

void invalidFieldMsg(int istime) {
  mMsgBoxPush(3);
  if(istime) mPrintXY(3, 3, (char*)"Invalid time.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  else mPrintXY(3, 3, (char*)"Invalid date.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  closeMsgBox(); 
}

void setEventChrono(CalendarEvent* event) {
  // set a downwards chrono that ends on the start time of the event
  // like a event reminder
  
  // ask user which chrono to set
  Selector sel;
  sel.title = (char*)"Set event reminder";
  sel.subtitle = (char*)"Select chrono";
  sel.value = 1;
  sel.min = 1;
  sel.max = NUMBER_OF_CHRONO;
  int res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  // get unix*1000 time of the event's start time/date
  long long int estart = DateTime2Unix(event->startdate.year, event->startdate.month, event->startdate.day, event->starttime.hour, event->starttime.minute, event->starttime.second, 0);
  // get chrono duration (difference between event start time and current time)
  
  long long int duration = estart - currentUnixTime();
  mMsgBoxPush(4);
  if(duration < 0) {
    // event is in the past, abort
    mPrintXY(3, 2, (char*)"Event starts in", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, (char*)"the past.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    // set downwards chrono with the calculated duration
    res = setChronoExternal(sel.value-1, duration, CHRONO_TYPE_DOWN);
    if(res) {
      // success setting a chrono
      mPrintXY(3, 2, (char*)"Event reminder", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 3, (char*)"set successfully.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    } else {
      // timer is busy
      mPrintXY(3, 2, (char*)"Selected chrono", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 3, (char*)"is not clear.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    }
  }
  closeMsgBox();
}

int changeEventCategory(CalendarEvent* event) {
  //Allows for changing the category of a event/task with a color select dialog.
  //Returns 1 on success and 0 on user abort
  //Does not save the changes to SMEM (like eventEditor)
  unsigned char selcolor = (unsigned char) 0xFF; //just so it isn't uninitialized
  int initcolor = 0;
  if(event->category!=0) initcolor = event->category-1;
  else initcolor = 7;
  selcolor = ColorIndexDialog1( initcolor, 0 );
  if(selcolor != (unsigned char)0xFF) {
    //user didn't press EXIT, QUIT or AC/ON. input is validated.
    selcolor != 7 ? event->category = selcolor+1 : event->category = 0;
    bufmonth = 0; //because colors will change in month view
    return EVENTEDITOR_RETURN_CONFIRM;
  }
  return EVENTEDITOR_RETURN_EXIT;
}

void viewNthEventOnDay(EventDate* date, int pos) {
  int num = GetEventsForDate(date, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* tmpevents = (CalendarEvent*)alloca(num*sizeof(CalendarEvent));
  num = GetEventsForDate(date, CALENDARFOLDER, tmpevents);
  viewEvent(&tmpevents[pos]); 
}

void searchEventsGUI(int y, int m, int d) {
  mMsgBoxPush(5);
  MenuItem smallmenuitems[5];
  smallmenuitems[0].text = (char*)"Selected year";
  smallmenuitems[1].text = (char*)"Selected month";
  smallmenuitems[2].text = (char*)"Selected day";
  smallmenuitems[3].text = (char*)"Year range";
  
  Menu smallmenu;
  smallmenu.items=smallmenuitems;
  smallmenu.numitems=4;
  smallmenu.width=17;
  smallmenu.height=5;
  smallmenu.startX=3;
  smallmenu.startY=2;
  smallmenu.scrollbar=0;
  smallmenu.statusText = (char*)""; // to clear "Press OPTN..." message
  smallmenu.title = (char*)"Search on:";
  int sres = doMenu(&smallmenu);
  mMsgBoxPop();
  
  if(sres != MENU_RETURN_SELECTION) return;
  char needle[55] = "";
  
  SetBackGround(9);
  clearLine(1,8);
  drawScreenTitle((char*)"Event Search", (char*)"Search for:");
  drawFkeyLabels(-1,-1,-1,-1,-1,0x00A5); // SEARCH (white)
  
  textInput input;
  input.forcetext=1; //force text so title must be at least one char.
  input.charlimit=50;
  input.acceptF6=1;
  input.buffer = needle;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) break; // continue to search
  }
  SimpleCalendarEvent* events;
  MenuItem* menuitems;
  
  Menu menu;
  menu.scrollout=1;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  menu.nodatamsg = (char*)"No events found";
  menu.title = (char*)"Search results";

  switch(smallmenu.selection) {
    case 1:
    case 2:
      menu.numitems = SearchEventsOnYearOrMonth(y, (smallmenu.selection==1? 0:m), CALENDARFOLDER, NULL, needle, 200); //get event count
      events = (SimpleCalendarEvent*)alloca(menu.numitems*sizeof(SimpleCalendarEvent));
      menu.numitems = SearchEventsOnYearOrMonth(y, (smallmenu.selection==1? 0:m), CALENDARFOLDER, events, needle, 200);
      break;
    case 3:
    { EventDate sday;
      sday.day = d; sday.month = m; sday.year = y;
      menu.numitems = SearchEventsOnDay(&sday, CALENDARFOLDER, NULL, needle, MAX_DAY_EVENTS); //get event count
      events = (SimpleCalendarEvent*)alloca(menu.numitems*sizeof(SimpleCalendarEvent));
      menu.numitems = SearchEventsOnDay(&sday, CALENDARFOLDER, events, needle, MAX_DAY_EVENTS);
      break;
    }
    //case 4:
    default: //avoids compiler warning
    { int userStartYear = y-2;
      int userEndYear = y+2;
      Selector sel;
      sel.title = (char*)"Search on year range";
      sel.subtitle = (char*)"Start year";
      sel.value = userStartYear;
      sel.min = 1;
      sel.max = 9999;
      if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
      userStartYear = sel.value;
      
      sel.subtitle = (char*)"End year";
      sel.value = userEndYear;
      sel.min = userStartYear;
      sel.max = (userStartYear+254 > 9999 ? 9999 : userStartYear+254); //do not allow for more than 255 years, otherwise maximum will be less than an event per year
      // also, if more than 255 years it would definitely take too much time, and user would certainly reboot the calculator
      sel.cycle = 0;
      if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
      userEndYear = sel.value;
      
      int yc = userEndYear-userStartYear+1;
      int pc = 0;
      int ic = 0, firstYwithEvents = 0, lastYwithEvents = 0;
      int maxPerYear = 250/yc; // calculate maximum number of events we can get on each year
      progressMessage((char*)" Searching...", 0, yc);
      for(int i=userStartYear; i<=userEndYear; i++) {
        int c = SearchEventsOnYearOrMonth(i, 0, CALENDARFOLDER, NULL, needle, maxPerYear, ic); //get event count
        if(!firstYwithEvents && c>0) firstYwithEvents = i;
        if(c>0) lastYwithEvents = i;
        pc++;
        if(pc <= yc) progressMessage((char*)" Searching...", pc, yc);
        ic += c;
      }
      closeProgressMessage();
      events = (SimpleCalendarEvent*)alloca(ic*sizeof(SimpleCalendarEvent));
      ic=0;
      yc = lastYwithEvents-firstYwithEvents+1;
      pc=0;
      progressMessage((char*)" Searching...", 0, yc);
      for(int i=firstYwithEvents; i<=lastYwithEvents; i++) {
        ic += SearchEventsOnYearOrMonth(i, 0, CALENDARFOLDER, events, needle, maxPerYear, ic);
        pc++;
        if(pc <= yc) progressMessage((char*)" Searching...", pc, yc);
      }
      closeProgressMessage();
      menu.numitems = ic;
      break;
    }
  }
  menuitems = (MenuItem*)alloca(menu.numitems*sizeof(MenuItem));
  int curitem = 0;
  while(curitem < menu.numitems) {
    menuitems[curitem].text = (char*)events[curitem].title;
    menuitems[curitem].type = MENUITEM_NORMAL;
    menuitems[curitem].color = events[curitem].category-1;
    curitem++;
  }
  menu.items=menuitems;
  while(1) {
    Bdisp_AllClr_VRAM();
    if(menu.numitems>0) {
      // VIEW, DATE, JUMP
      drawFkeyLabels(0x049F, 0x015F, 0x01FC);
    }
    switch(doMenu(&menu)) {
      case MENU_RETURN_EXIT:
        return;
        break;
      case KEY_CTRL_F1:
      case MENU_RETURN_SELECTION:
        viewNthEventOnDay(&events[menu.selection-1].startdate, events[menu.selection-1].origpos);
        break;
      case KEY_CTRL_F2:
        if(menu.numitems>0) {
          searchValid = 1;
          viewEvents(events[menu.selection-1].startdate.year, events[menu.selection-1].startdate.month, events[menu.selection-1].startdate.day);
          if(!searchValid) return;
        }
        break;
      case KEY_CTRL_F3:
        if(menu.numitems>0) {
          sy=events[menu.selection-1].startdate.year;
          sm=events[menu.selection-1].startdate.month;
          sd=events[menu.selection-1].startdate.day;
          return;
        }
        break;
    }
  }
}

void drawDayBusyMap(EventDate* thisday, int startx, int starty, int width, int height, int showHourMarks, int isWeek, int maxx) {
  if(!isWeek) drawRectangle(startx, starty, width, height, COLOR_LIGHTGRAY);
  if(showHourMarks) {
    for(int i = 0; i < 24; i++) {
      int tx=(width*i*60*60)/(24*60*60);
      plot(startx+tx,(showHourMarks==2? starty-1 : starty+height),COLOR_GRAY);
      if(showHourMarks==2) plot(startx+tx, starty-2,COLOR_GRAY);
    }
  }
  int count = GetEventsForDate(thisday, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* events = (CalendarEvent*)alloca(count*sizeof(CalendarEvent));
  count = GetEventsForDate(thisday, CALENDARFOLDER, events);
  int categoryColors[10] = {COLOR_GRAY, COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN, COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW};
  int curitem = 0;
  while(curitem < count) {
    long int daysduration = DateToDays(events[curitem].enddate.year, events[curitem].enddate.month, events[curitem].enddate.day) - DateToDays(events[curitem].startdate.year, events[curitem].startdate.month, events[curitem].startdate.day);
    long int bwidth = 0;
    if(events[curitem].timed) {
      long int start = events[curitem].starttime.hour*60*60 + events[curitem].starttime.minute*60 + events[curitem].starttime.second;        
      
      long int x = (width*start)/(24*60*60);
      long int duration = daysduration*24*60*60 + events[curitem].endtime.hour*60*60 + events[curitem].endtime.minute*60 + events[curitem].endtime.second - start;
      if(duration<0) setDBneedsRepairFlag(1); //event ends before it starts, user should repair the DB
      bwidth = (width*duration)/(24*60*60);
      if(bwidth <= 0) bwidth = 1; // always make an event visible
      if((!isWeek) && x+bwidth > width) {
        bwidth = width-x;
      }
      if(isWeek) {
        int weekx = startx+x;
        if(weekx+bwidth > maxx) {
          bwidth = maxx-weekx;
        }
      }
      drawRectangle(startx+x, starty, bwidth, height, categoryColors[events[curitem].category]);
    } else if(!events[curitem].timed && isWeek) {
      if(daysduration<0) setDBneedsRepairFlag(1); //event ends before it starts, user should repair the DB
      bwidth = width*(daysduration+1);
      if(startx+bwidth > maxx) {
        bwidth = maxx-startx;
      }
      drawRectangle(startx, starty, bwidth, height, categoryColors[events[curitem].category]);
    }
    curitem++;
  }
}
void drawWeekBusyMap(int y, int m, int d, int startx, int starty, int width, int height) {
  long int ddays=DateToDays(y, m, d);
  unsigned int curday = 0;
  int daywidth = width/7;
  drawRectangle(startx, starty, daywidth*7, height, COLOR_LIGHTGRAY);
  while(curday < 7) {
    long int ny, nm, nd;
    DaysToDate(ddays, &ny, &nm, &nd);
    EventDate date;
    date.year=ny; date.month=nm; date.day=nd;
    if(!isDateValid(ny,nm,nd)) {
      // one of the dates we're trying to view is not valid (probably because the year is not valid, i.e. below 0 or above 9999).
      return; // NOTE abort
    }
    plot(startx+daywidth*curday,starty-1,COLOR_GRAY);
    plot(startx+daywidth*curday,starty-2,COLOR_GRAY);
    if(date.year == (unsigned)getCurrentYear() && date.month == (unsigned)getCurrentMonth() && date.day == (unsigned)getCurrentDay())
      drawRectangle(startx+daywidth*curday+1, starty-2, daywidth-1, 2, COLOR_YELLOW);
    drawDayBusyMap(&date, startx+curday*daywidth, starty, daywidth, height, 0,curday+1,startx+daywidth*7);
    ddays++;
    curday++;
  }
}

void calendarTools(int y, int m, int d) {
  mMsgBoxPush(6);
  MenuItem smallmenuitems[5];
  smallmenuitems[0].text = (char*)"Count days";
  smallmenuitems[1].text = (char*)"Repair database";
  smallmenuitems[2].text = (char*)"Trim database";
  smallmenuitems[3].text = (char*)"Import events";
  smallmenuitems[4].text = (char*)"Calendar settings";
  
  Menu smallmenu;
  smallmenu.items=smallmenuitems;
  smallmenu.numitems=5;
  smallmenu.width=17;
  smallmenu.height=6;
  smallmenu.startX=3;
  smallmenu.startY=2;
  smallmenu.scrollbar=0;
  smallmenu.statusText = (char*)""; // to clear "Press OPTN..." message
  smallmenu.title = (char*)"Calendar tools";
  int sres = doMenu(&smallmenu);
  mMsgBoxPop();
  
  if(sres == MENU_RETURN_SELECTION) {
    switch(smallmenu.selection) {
      case 1: {
        int y1 = y;
        int m1 = m;
        int d1 = d;
        if(chooseCalendarDate(&y1, &m1, &d1, (char*)"Select first date", NULL, 1)) return;
        int y2=y1, m2=m1, d2=d1;
        if(chooseCalendarDate(&y2, &m2, &d2, (char*)"Select second date", NULL, 1)) return;

        long int daysdiff = DateToDays(y2, m2, d2) - DateToDays(y1, m1, d1);
        long int businessdiff = 0;
        long int weekdays[7] = {0,0,0,0,0,0,0};
        for(long int i = 0; (daysdiff < 0 ? i > daysdiff : i < daysdiff); (daysdiff < 0 ? i-- : i++)) {
          long int ny, nm, nd;
          DaysToDate(DateToDays(y1, m1, d1)+i, &ny, &nm, &nd);
          int dw = dow(ny, nm, nd);
          if(dw != 0 && dw != 6) (daysdiff < 0 ? businessdiff-- : businessdiff++ );
          weekdays[dw]++;
        }
        textArea text;
        text.title = (char*)"Date difference";

        textElement elem[15];
        text.elements = elem;
        
        char line1[50];
        strcpy(line1, (char*)"Between ");
        char buffer[20];
        dateToString(buffer, y1, m1, d1);
        strcat(line1, buffer);
        strcat(line1, (char*)" and ");
        dateToString(buffer, y2, m2, d2);
        strcat(line1, buffer);
        strcat(line1, (char*)":");
        elem[0].text = line1;
        
        
        elem[1].newLine = 1;
        char line2[50];
        itoa((int)daysdiff, (unsigned char*)line2);
        strcat(line2, (char*)" days");
        elem[1].text = line2;
        
        elem[2].newLine = 1;
        char line3[50];
        itoa((int)businessdiff, (unsigned char*)line3);
        strcat(line3, (char*)" business days");
        elem[2].text = line3;

        elem[3].newLine = 1;
        char line4[50];
        itoa((int)daysdiff/365, (unsigned char*)line4);
        strcat(line4, (char*)" years");
        elem[3].text = line4;

        elem[4].newLine = 1;
        char line5[50];
        itoa((int)daysdiff/30, (unsigned char*)line5);
        strcat(line5, (char*)" months");
        elem[4].text = line5;

        elem[5].newLine = 1;
        char line6[50];
        itoa((int)daysdiff/7, (unsigned char*)line6);
        strcat(line6, (char*)" weeks");
        elem[5].text = line6;
        text.numelements = 6;

        char weeklines[7][50];
        for(int i=0; i<7; i++) {
          elem[text.numelements].newLine = 1;
          itoa((int)weekdays[i], (unsigned char*)weeklines[i]);
          strcat(weeklines[i], (char*)" ");
          strcat(weeklines[i], (char*)getDOWAsString(i+1));
          if(weekdays[i] != 1) strcat(weeklines[i], (char*)"s");
          elem[text.numelements].text = weeklines[i];
          text.numelements++;
        }
        
        doTextArea(&text);
        break;
      }
      case 2: repairCalendarDatabase(); break;
      case 3: trimCalendarDatabase(); break;
      case 4: importCalendarEvents(); break;
      case 5: {
        setmGetKeyMode(MGETKEY_MODE_RESTRICT_SETTINGS);
        calendarSettingsMenu();
        setmGetKeyMode(MGETKEY_MODE_NORMAL);
        break;
      }
    }
  }  
}

void repairCalendarDatabase() {
  textArea text;
  text.title = (char*)"Database repair";

  textElement elem[8];
  text.elements = elem;
  if(getDBneedsRepairFlag()) {
    elem[0].text = (char*)"Hey! Sorry to interrupt you like this, but inconsistent data was detected in the calendar database. This often happens when a version of Utilities prior to v1.1 has been used, or when events with invalid data are imported.";
    elem[1].text = (char*)"Repairing the database now, to avoid further problems, is very important. After doing it, you should not see this message again.";
  } else {
    elem[0].text = (char*)"Repairing the calendar events' database will fix any inconsistent data, such as events with an end time preceding their start time.";
    elem[1].text = (char*)"Doing this is highly recommended if you used a version of Utilities prior to v1.1, or if you are experiencing problems viewing and manipulating calendar events.";
  } 
  elem[1].newLine = 1;
  elem[1].lineSpacing = 5;
  elem[2].newLine = 1;
  elem[2].lineSpacing = 5;
  elem[2].text = (char*)"Repairing the database should not result in data loss, except for any corrupt entries that despite their state, are still partially readable (these will be deleted). You may want to create backups of such entries using pen and paper - never use Utilities to perform operations on corrupt events or others in their start date!";
  elem[3].newLine = 1;
  elem[3].lineSpacing = 5;
  elem[3].text = (char*)"If there are many events stored, this operation may take a long time. Press F1 to start or EXIT to cancel.";
  
  text.allowF1 = 1;
  text.numelements = 4;
  if(!doTextArea(&text)) return;
  
  setDBneedsRepairFlag(0);
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.allowF1 = 0;
  text.scrollbar=0;
  elem[0].text = (char*)"Checking and repairing problems in the calendar events' database. This may take a long time, please wait and do not turn off the calculator...";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 20;
  elem[1].text = (char*)"Number of files checked:";
  elem[1].spaceAtEnd = 1;
  // elem[2] is set in loop
  elem[2].lineSpacing = 0;
  elem[2].newLine = 0;
  elem[3].lineSpacing = 0;
  elem[3].newLine = 1;
  elem[3].text = (char*)"Number of events checked:";
  elem[3].spaceAtEnd = 1;
  //elem[4] is set in loop
  elem[5].newLine = 1;
  elem[5].text = (char*)"Number of problems fixed:";
  elem[5].spaceAtEnd = 1;
  //elem[6] is set in loop
  text.numelements = 1; // for security, in case no file is found
  int checkedfiles = 0;
  int checkedevents = 0;
  int problemsfound = 0;
  
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  char buffer[MAX_FILENAME_SIZE+1];

  // make the buffer
  strcpy(buffer, CALENDARFOLDER"\\*.pce");
  
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  int loopc = 9;
  char buffer1[20], buffer2[20], buffer3[20];
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0) && fileinfo.fsize != 0) {     
      if (loopc>=9) {
        itoa((int)checkedfiles, (unsigned char*)buffer1);
        elem[2].text = buffer1;
        
        itoa((int)checkedevents, (unsigned char*)buffer2);
        elem[4].text = buffer2;
        
        itoa((int)problemsfound, (unsigned char*)buffer3);
        elem[6].text = buffer3;
        text.numelements = 7;
        doTextArea(&text);
        Bdisp_PutDisp_DD();
        loopc = 0;
      } else loopc++;
      repairEventsFile((char*)buffer, CALENDARFOLDER, &checkedevents, &problemsfound);
      checkedfiles++;
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  // update totals, because they do not include the last file checked
  itoa((int)checkedfiles, (unsigned char*)buffer1);
  elem[2].text = buffer1;
  
  itoa((int)checkedevents, (unsigned char*)buffer2);
  elem[4].text = buffer2;
  
  itoa((int)problemsfound, (unsigned char*)buffer3);
  elem[6].text = buffer3;
  
  text.type = TEXTAREATYPE_NORMAL;
  elem[0].text = (char*)"Done repairing the calendar database. Press EXIT.";
  doTextArea(&text);
  bufmonth = 0; // because apart from editing dates, database repair also deletes invalid files that may influence event counts.
  searchValid = 0; // invalidate week view results
}

void trimCalendarDatabase() {
  textArea text;
  text.title = (char*)"Trim database";
  text.type = TEXTAREATYPE_INSTANT_RETURN;

  textElement elem[4];
  text.elements = elem;
  text.scrollbar=0;
  
  elem[0].text = (char*)"You can reduce the size of the calendar events' database, by";
  elem[0].spaceAtEnd = 1;
  elem[1].text = (char*)"deleting";
  elem[1].spaceAtEnd = 1;
  elem[1].color = COLOR_RED;
  elem[2].text = (char*)"events you do not need anymore. Select what events to delete, or press EXIT to cancel.";

  text.numelements = 3;
  doTextArea(&text);
  
  MenuItem menuitems[4];
  menuitems[0].text = (char*)"Older than 6 months";
  menuitems[1].text = (char*)"Older than 1 month";
  menuitems[2].text = (char*)"Events in the past";
  menuitems[3].text = (char*)"All calendar events";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=4;
  menu.scrollout=1;
  menu.startY = 6;
  menu.height = 3;
  
  int res = doMenu(&menu);
  if(res == MENU_RETURN_EXIT) return;
  else if(res == MENU_RETURN_SELECTION) {
    if(menu.selection == 4) {
      mMsgBoxPush(5);
      mPrintXY(3, 2, (char*)"DELETE / RESET", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 3, (char*)"ALL the calendar", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 4, (char*)"events?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      if(!closeMsgBox(1)) return;
    }
    unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
    char buffer[MAX_FILENAME_SIZE+1];

    // make the buffer
    strcpy(buffer, CALENDARFOLDER"\\*.pce");
    
    file_type_t fileinfo;
    int findhandle;
    Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
    int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
    while(!ret) {
      Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
      // the 00000.pce strcmp is there so we don't delete the tasks file
      if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 || strcmp((char*)buffer, "00000.pce") == 0) && fileinfo.fsize != 0) {
        // select what to do depending on menu selection
        int deleteThisFile = 0;
        if(menu.selection == 4) {
          // user wants to delete all events
          deleteThisFile = 1;
        } else {
          // user wants to do something that depends on the start date, so we need to get it from the filename
          char mainname[20];
          int nlen = strlen((char*)buffer);
          strncpy(mainname, (char*)buffer, nlen-4); //strip the file extension out
          // strcpy will not add a \0 at the end if the limit is reached, let's add it ourselves
          mainname[nlen-4] = '\0';
          nlen = strlen(mainname);
          
          // verify that it only contains numbers
          for(int i = 0; i < nlen; i++) {
            if(!isdigit(mainname[i])) {
              // some character is not a number, delete file (DB repair would do this, anyway)
              deleteThisFile = 1;
            }
          }
          EventDate thisday;
          if(!deleteThisFile) {
            char tmpbuf[10] = "";
            for(int i = 0; i<8-nlen; i++) {
              strcat(tmpbuf, "0");
            }
            strcat(tmpbuf, mainname);
            strcpy(mainname, tmpbuf);
            int y, m, d;
            stringToDate(mainname, &y, &m, &d, 2);
            thisday.year=y; thisday.month=m; thisday.day=d;
            
            // see if the date in the filename is valid
            if(!isDateValid(thisday.year,thisday.month,thisday.day)) {
              // oops, date is not valid, and this is not the tasks file
              deleteThisFile = 1; // DB repair would do this anyway
            }
          }
          if(!deleteThisFile) {
            long int datediff = DateToDays(getCurrentYear(), getCurrentMonth(), getCurrentDay()) - DateToDays(thisday.year, thisday.month, thisday.day);
            switch(menu.selection) {
              case 1:
                if(datediff > 30+31+30+31+30+31) deleteThisFile=1;
                break;
              case 2:
                if(datediff > 30) deleteThisFile=1;
                break;
              case 3:
                if(datediff > 0) deleteThisFile=1;
                break;
            }
          }
        }
        if(deleteThisFile) {
          char delfname[MAX_FILENAME_SIZE+1];
          strcpy(delfname, CALENDARFOLDER"\\");
          strcat(delfname, (char*)buffer);
          unsigned short path2[MAX_FILENAME_SIZE+1];
          Bfile_StrToName_ncpy(path2, delfname, MAX_FILENAME_SIZE+1);
          Bfile_DeleteEntry( path2 );
        }
      }
      ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
    }
    Bfile_FindClose(findhandle);
  }
  mMsgBoxPush(4);
  mPrintXY(3, 2, (char*)"DB trimming", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 3, (char*)"completed", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 4, (char*)"successfully.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  closeMsgBox();
  bufmonth = 0;
  searchValid = 0;
}
int importHelper(EventDate* date, int count, textArea* text, textElement* elem, int initSuc) {
  CalendarEvent* events = (CalendarEvent*)alloca(count*sizeof(CalendarEvent));
  GetEventsForDate(date, CALENDARFOLDER, events);
  int successful = initSuc;
  for(int curitem = 0; curitem < count; curitem++) {
    char buffer1[20];
    itoa((int)successful, (unsigned char*)buffer1);
    elem[2].text = buffer1;
    doTextArea(text);
    Bdisp_PutDisp_DD();
    // add the event. this will automatically put it in the correct file in the database.
    if(!AddEvent(&events[curitem], CALENDARFOLDER)) successful++;
  }
  return successful;
}
void importCalendarEvents() {
  textArea text;
  text.title = (char*)"Import events";

  textElement elem[4];
  text.elements = elem;
  
  elem[0].text = (char*)"This tool is responsible for the last step in importing events into the calendar database.";
  elem[1].newLine = 1;
  elem[1].text = (char*)"It assumes you have already used desktop software (for example, the web app at http://pce.tny.im) to convert events into an appropriate format, and that you followed that software's instructions as to where the resulting files should be in this calculator.";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 5;
  elem[2].text = (char*)"This operation may take a long time, if there are many events to import. Press F1 to start or EXIT to cancel.";
  
  text.allowF1 = 1;
  text.numelements = 3;
  if(!doTextArea(&text)) return;
  
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.allowF1 = 0;
  text.scrollbar=0;
  elem[0].text = (char*)"Importing calendar events. This may take a long time, please wait and do not turn off the calculator...";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 20;
  elem[1].text = (char*)"Number of events imported:";
  elem[1].spaceAtEnd = 1;
  // elem[2] is set in loop
  elem[2].lineSpacing = 0;
  elem[2].newLine = 0;
  
  // User accepted, let the importing operation begin.
  // the import tool assumes the events to import to be in up to 99 files, up to 100 events each
  // this means that one can import 9900 events at a time (but that would take hours).
  // files should be in the PCE format, and be in the @UTILS folder with the other calendar files.
  // the files should be numbered from 00001.pce to 00099.pce
  // they do not collide with existing event files, because the month 0 is reserved for non-calendar files (tasks and files to import).
  EventDate thisday;
  thisday.month = 0; thisday.year = 0;
  int successful = 0;
  for(thisday.day = 1; thisday.day<100; thisday.day++) {
    int count = GetEventsForDate(&thisday, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
    if(count && count<=MAX_DAY_EVENTS) {
      successful = importHelper(&thisday, count, &text, elem, successful);
      // delete so we don't import this day again, if user calls the import function again.
      RemoveDay(&thisday, CALENDARFOLDER);
    }
  }
  text.type = TEXTAREATYPE_NORMAL;
  char buffer1[20];
  itoa((int)successful, (unsigned char*)buffer1);
  elem[0].text = buffer1;
  elem[0].spaceAtEnd = 1;
  elem[1].newLine = 0;
  elem[1].text = (char*)"events imported successfully.";
  elem[2].lineSpacing = 5;
  elem[2].newLine = 1;
  elem[2].text = (char*)"Press EXIT.";
  doTextArea(&text);
  bufmonth = 0; // because apart from editing dates, database repair also deletes invalid files that may influence event counts.
  searchValid = 0; // invalidate week view results
}
