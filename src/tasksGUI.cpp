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

#include "tasksGUI.hpp"
#include "menuGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "calendarProvider.hpp"
#include "calendarGUI.hpp"
#include "constantsProvider.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "timeProvider.hpp"

void viewTasks() {
  int res=1;
  Menu menu;
  
  menu.scrollout=1;
  menu.showtitle=1;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  strcpy(menu.nodatamsg, "No Tasks - press F2");
  strcpy(menu.title, "Tasks");
  strcpy(menu.statusText, "");
  while(res) {
    res = viewTasksSub(&menu);
  }
}

int viewTasksSub(Menu* menu) {
  //returns 1 when it wants to be restarted (refresh tasks)
  //returns 0 if the idea really is to exit the screen
  EventDate taskday;
  taskday.day = 0; taskday.month = 0; taskday.year = 0;
  
  menu->numitems = GetEventsForDate(&taskday, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* tasks = (CalendarEvent*)alloca(menu->numitems*sizeof(CalendarEvent));
  MenuItem* menuitems = (MenuItem*)alloca(menu->numitems*sizeof(MenuItem));
  menu->numitems = GetEventsForDate(&taskday, CALENDARFOLDER, tasks);
  int curitem = 0;
  while(curitem <= menu->numitems-1) {
    strcpy(menuitems[curitem].text, (char*)tasks[curitem].title);
    menuitems[curitem].type = MENUITEM_CHECKBOX;
    menuitems[curitem].value = tasks[curitem].repeat;
    menuitems[curitem].color = tasks[curitem].category-1;
    curitem++;
  }
  menu->items=menuitems; 

  Bdisp_AllClr_VRAM();
  DisplayStatusArea(); 
  
  int iresult;
  if (menu->fkeypage == 0) {
    if(menu->numitems>0) {
      GetFKeyPtr(0x049F, &iresult); // VIEW
      FKey_Display(0, (int*)iresult);
      GetFKeyPtr(0x0185, &iresult); // EDIT
      FKey_Display(2, (int*)iresult);
      GetFKeyPtr(0x0038, &iresult); // DELETE
      FKey_Display(3, (int*)iresult);
      GetFKeyPtr(0x0104, &iresult); // DEL-ALL
      FKey_Display(4, (int*)iresult);
      GetFKeyPtr(0x049D, &iresult); // Switch [white]
      FKey_Display(5, (int*)iresult);
    }
    GetFKeyPtr(0x03B4, &iresult); // INSERT
    FKey_Display(1, (int*)iresult);
  }
  if(menu->selection > menu->numitems) menu->selection = menu->numitems;
  if(menu->selection < 1) menu->selection = 1;
  int res = doMenu(menu);
  switch(res) {
    case MENU_RETURN_EXIT:
      return 0;
      break;
    case KEY_CTRL_F1:
    case MENU_RETURN_SELECTION:
      if(menu->numitems > 0) viewTask(&tasks[menu->selection-1]);
      break;
    case KEY_CTRL_F2:
      if(menu->numitems >= MAX_DAY_EVENTS) {
        AUX_DisplayErrorMessage( 0x2E );
      } else {
        eventEditor(0, 0, 0, EVENTEDITORTYPE_ADD, NULL, 1);
      }
      break;
    case KEY_CTRL_F3:
      if(menu->numitems > 0)
        if(eventEditor(0, 0, 0, EVENTEDITORTYPE_EDIT, &tasks[menu->selection-1], 1) == EVENTEDITOR_RETURN_CONFIRM) {
          ReplaceEventFile(&tasks[menu->selection-1].startdate, tasks, CALENDARFOLDER, menu->numitems);
        }
      break;
    case KEY_CTRL_F4:
      if(menu->numitems > 0) deleteEventUI(0, 0, 0, tasks, menu->numitems, menu->selection-1, 1);
      break;
    case KEY_CTRL_F5:
      if(menu->numitems > 0) deleteAllEventUI(0, 0, 0, 1);
      break;
    case KEY_CTRL_F6:
      if(menu->numitems > 0) toggleTaskActivity(tasks, menu->selection-1, menu->numitems);
      break;
    case KEY_CTRL_FORMAT:
      if(menu->numitems > 0) {
        //the "FORMAT" key is used in many places in the OS to format e.g. the color of a field,
        //so on this add-in it is used to change the category (color) of a task/calendar event.
        if(changeEventCategory(&tasks[menu->selection-1])) {
          ReplaceEventFile(&tasks[menu->selection-1].startdate, tasks, CALENDARFOLDER, menu->numitems);
        }
      }
      break;
  }
  return 1;
}

void viewTask(CalendarEvent* task) {
  textArea text;
  strcpy(text.title, (char*)task->title);
  
  textElement elem[10];
  text.elements = elem;
  
  elem[0].text = (char*)"Location:";
  elem[0].color=COLOR_LIGHTGRAY;
  elem[0].spaceAtEnd=1;
  
  elem[1].text = (char*)task->location;
  
  elem[2].text = (char*)"Category:";
  elem[2].color=COLOR_LIGHTGRAY;
  elem[2].newLine = 1;
  elem[2].spaceAtEnd=1;
  
  char buffer[10];
  itoa(task->category, (unsigned char*)buffer);
  elem[3].text = (char*)buffer;
  elem[3].color = textColorToFullColor((task->category == 0 ? 7 : task->category-1));
  
  elem[4].text = (char*)"Description:";
  elem[4].newLine = 1;
  elem[4].lineSpacing = 8;
  elem[4].color=COLOR_LIGHTGRAY;
  
  elem[5].text = (char*) task->description;
  elem[5].newLine = 1;
  
  text.numelements = 6;
  doTextArea(&text);
}