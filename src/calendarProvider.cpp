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

#include "constantsProvider.hpp"
#include "chronoProvider.hpp"
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "stringsProvider.hpp"
#include "selectorGUI.hpp"
#include "calendarProvider.hpp" 
#include "fileProvider.hpp" 
#include "debugGUI.hpp" 

void append(unsigned char* s, char c) //beacuse strcat seems to have problems with 1-byte non-null-terminated chars like the field and event separators
{
  int len = strlen((char*)s);
  s[len] = c;
  s[len + 1] = '\0';
}

void calEventToChar(CalendarEvent* calEvent, unsigned char* buf) {
  /* Parses a CalendarEvent struct and turns it into a string which can be written to a file.
     The first field (category) begins with no separator.
     An event doesn't begin with any separators, and the last field ends with a field separator followed by an event separator.
     An event can have at least 4KB size. The lengthy fields are obviously the title, the location and mainly the description.*/
  unsigned char smallbuf[50] = ""; 

  itoa(calEvent->category, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 2); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->daterange, (unsigned char*)smallbuf);  strncat((char*)buf, (char*)smallbuf,2); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->startdate.day, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->startdate.month, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->startdate.year, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 5); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->enddate.day, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->enddate.month, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->enddate.year, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 5); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->dayofweek, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 2); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->repeat, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 2); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->timed, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 2); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->starttime.hour, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->starttime.minute, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->starttime.second, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->endtime.hour, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->endtime.minute, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->endtime.second, (unsigned char*)smallbuf); strncat((char*)buf, (char*)smallbuf, 3); append(buf, FIELD_SEPARATOR);
  strncat((char*)buf, (char*)calEvent->title,25); append(buf, FIELD_SEPARATOR);
  strncat((char*)buf, (char*)calEvent->location,135); append(buf, FIELD_SEPARATOR);
  strncat((char*)buf, (char*)calEvent->description,1030); append(buf, EVENT_SEPARATOR);
  //the last field ends with a field separator followed by an event separator. both are contained in EVENT_SEPARATOR
  
  /*sprintf((char*)buf, "%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%s%c%s%c%s%c" ,
        calEvent->category,                             FIELD_SEPARATOR,
        calEvent->daterange,                            FIELD_SEPARATOR,
        calEvent->startdate.day,                        FIELD_SEPARATOR,
        calEvent->startdate.month,                      FIELD_SEPARATOR,
        calEvent->startdate.year,                       FIELD_SEPARATOR,
        calEvent->enddate.day,                          FIELD_SEPARATOR,
        calEvent->enddate.month,                        FIELD_SEPARATOR,
        calEvent->enddate.year,                         FIELD_SEPARATOR,
        calEvent->dayofweek,                            FIELD_SEPARATOR,
        calEvent->repeat,                               FIELD_SEPARATOR,
        calEvent->timed,                                FIELD_SEPARATOR,
        calEvent->starttime.hour,                       FIELD_SEPARATOR,
        calEvent->starttime.minute,                     FIELD_SEPARATOR,
        calEvent->starttime.second,                     FIELD_SEPARATOR,
        calEvent->endtime.hour,                         FIELD_SEPARATOR,
        calEvent->endtime.minute,                       FIELD_SEPARATOR,
        calEvent->endtime.second,                       FIELD_SEPARATOR,
        (char*)calEvent->title,                         FIELD_SEPARATOR,
        (char*)calEvent->location,                      FIELD_SEPARATOR,
        (char*)calEvent->description,                   EVENT_SEPARATOR
  );*/
}

void charToCalEvent(unsigned char* src, CalendarEvent* calEvent) {
  /* Parses a string containing a single event and turns it into a CalendarEvent which the program can work with.
  */
  int curfield = 0; //field we are parsing currently. starts at the category, which is 0.
  unsigned char token[1024];
  strcpy((char*)token, "");
  src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  int notfinished = 1;
  while (notfinished) {
    switch (curfield) {
      case 0: //category
        calEvent->category = atoi((const char*)token);
        break;
      case 1: //daterange
        calEvent->daterange = atoi((const char*)token);
        break;
      case 2: //startdate.day
        calEvent->startdate.day = atoi((const char*)token);
        break;
      case 3: //startdate.month
        calEvent->startdate.month = atoi((const char*)token);
        break;
      case 4: //startdate.year
        calEvent->startdate.year = atoi((const char*)token);
        break;
      case 5: //enddate.day
        calEvent->enddate.day = atoi((const char*)token);
        break;
      case 6: //enddate.month
        calEvent->enddate.month = atoi((const char*)token);
        break;
      case 7: //enddate.year
        calEvent->enddate.year = atoi((const char*)token);
        break;
      case 8: //dayofweek
        calEvent->dayofweek = atoi((const char*)token);
        break;
      case 9: //repeat
        calEvent->repeat = atoi((const char*)token);
        break;
      case 10: //timed
        calEvent->timed = atoi((const char*)token);
        break;
      case 11: //starttime.hour
        calEvent->starttime.hour = atoi((const char*)token);
        break;
      case 12: //starttime.minute
        calEvent->starttime.minute = atoi((const char*)token);
        break;
      case 13: //starttime.second
        calEvent->starttime.second = atoi((const char*)token);
        break;
      case 14: //endtime.hour
        calEvent->endtime.hour = atoi((const char*)token);
        break;
      case 15: //endtime.minute
        calEvent->endtime.minute = atoi((const char*)token);
        break;
      case 16: //endtime.second
        calEvent->endtime.second = atoi((const char*)token);
        break;
      case 17: //title
        strncpy((char*)calEvent->title, (char*)token, 25);
        break;
      case 18: //location
        strncpy((char*)calEvent->location, (char*)token, 135);
        break;
      case 19: //description
        strncpy((char*)calEvent->description, (char*)token, 1030);
        notfinished = 0;
        break;
      default: //unknown field. may add special handling later.
        break;
    }
    curfield++;
    src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  }
}

void filenameFromDate(EventDate* date, char* filename) {
  char smallbuf[8] = "";
  itoa(date->year, (unsigned char*)smallbuf); strcpy(filename, smallbuf);
  itoa(date->month, (unsigned char*)smallbuf);
  if (date->month < 10) strcat(filename, "0");  //if month below 10, add leading 0
  strcat(filename, smallbuf);
  itoa(date->day, (unsigned char*)smallbuf);
  if (date->day < 10) strcat(filename, "0"); //if day below 10, add leading 0
  strcat(filename, smallbuf);
}
void smemFilenameFromDate(EventDate* date, char* filename, const char* folder) {
  char buffer[10] = "";
  strcpy(filename, "\\\\fls0\\");
  strcat(filename, folder);
  strcat(filename, "\\");
  filenameFromDate(date, buffer);
  strcat(filename, buffer);
  strcat(filename, ".pce"); //filenameFromDate does not include file extension, so add it
}

int AddEvent(CalendarEvent* calEvent, const char* folder) {
  //Saves a calendar event on an existing calendar with specified file name.
  //If the specified file doesn't exist, it is created and the event is added to it.
  //Returns 0 on success, other values on error.
  /*char foldername[128] = "";
  unsigned short pFolder[256];
  strcpy(foldername, "\\\\fls0\\");
  strcat(foldername, folder);
  Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file*/
  char filename[128] = "";
  smemFilenameFromDate(&calEvent->startdate, filename, folder);
  
  char newevent[2048] = "";
  calEventToChar(calEvent, (unsigned char*)newevent);
  int size = strlen(FILE_HEADER) + strlen(newevent);
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1);
  int hAddFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hAddFile < 0) // Check if it opened
  {
    // Returned error, file might not exist, so create it
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres >= 0) // Did it create?
    {
      //open in order to write header and new event
      hAddFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // always open, since file did not exist so it must be closed
      if(hAddFile < 0) // Still failing?
      {
        return 1;
      }
      char finalcontents[2060] = "";
      strcpy(finalcontents, FILE_HEADER);
      strcat(finalcontents, newevent);
      //Write header and event
      Bfile_WriteFile_OS(hAddFile, finalcontents, strlen(finalcontents));
      Bfile_CloseFile_OS(hAddFile);
      return 0;
    }
    else
    {
      // file doesn't exist, but can't be created?
      // it's probably because the DB has never been used and is not initialized (i.e. we haven't created the calendar folder).
      // create the folder:
      char foldername[128] = "";
      unsigned short pFolder[256];
      strcpy(foldername, "\\\\fls0\\");
      strcat(foldername, folder);
      Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
      Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0);
      // now let's call ourselves again, then according to the return value of our second instance, decide if there was an error or not.
      int ret = AddEvent(calEvent, folder);
      if(ret) return 2; // another error ocurred
      else return 0; //event was added without error, now that we created the folder
    }
  } else {
    /*File exists and is open.
      0. Hope there's enough heap to store everything throughout the process.
      1. Read its contents and size and save them.
      2. Close, delete the file.
      3. Create the same file with the previous size plus the size for the new event.
      4. Open the new file.
      5. Write header and previous contents.
      6. Write new contents (new event).
      7. Close file.
      It must be done this way because once a file is created, its size cannot be changed...*/
    int oldsize = Bfile_GetFileSize_OS(hAddFile);
    int newsize = oldsize + strlen(newevent);
    if (oldsize > MAX_EVENT_FILESIZE || newsize > MAX_EVENT_FILESIZE) { //file bigger than we can handle
      Bfile_CloseFile_OS(hAddFile);
      return 4;
    }
    unsigned char* oldcontents = (unsigned char*)alloca(newsize);
    if(oldsize) {
          Bfile_ReadFile_OS(hAddFile, oldcontents, oldsize, 0);
    }
    Bfile_CloseFile_OS(hAddFile); // always close even if not openclose, because we're going to recreate next
    Bfile_DeleteEntry(pFile);
    // we already read the previous contents and size, closed the file and deleted it.
    // now recreate it with new size and write new contents to it.

    int nBCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &newsize);
    if(nBCEres >= 0) // Did it create?
    {
      hAddFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // we always closed above
      Bfile_WriteFile_OS(hAddFile, oldcontents, oldsize);
      Bfile_WriteFile_OS(hAddFile, newevent, strlen(newevent));
      Bfile_CloseFile_OS(hAddFile);
    } else {
      return 3;
    }
  }
  return 0;
}

int ReplaceEventFile(EventDate *startdate, CalendarEvent* newEvents, const char* folder, int count) {
  // This basically deletes the events of startdate and replaces them with newEvents.
  // Allows for a way to edit an event on a day, when all the events for that day are in memory (including the edited one).
  // count: number of events in newEvents (number of events in old file doesn't matter). starts at 1.
  int res = RemoveDay(startdate, folder);
  if(res != 0) {
    return 1;
  }
  
  //convert the calevents back to char
  unsigned char eventbuf[2048] = ""; 
  unsigned char* newfilecontents = (unsigned char*)alloca(count*2048); //because the new file size can't be any bigger than the previous size, since we deleted a event
  strcpy((char*)newfilecontents, (char*)FILE_HEADER); //we need to initialize the char, take the opportunity to add the file header
  for(int j = 0; j < count; j++) {
    strcpy((char*)eventbuf, "");
    calEventToChar(&newEvents[j], eventbuf);
    strcat((char*)newfilecontents,(char*)eventbuf);
  }
  
  char filename[128] = "";
  smemFilenameFromDate(startdate, filename, folder);
  
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1);
  
  // we already deleted the file for this day on RemoveDay
  // now recreate it with new size and write new contents to it.
  int newsize = strlen((char*)newfilecontents);
  int nBCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &newsize);
  if(nBCEres >= 0) // Did it create?
  {
    int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0);
    Bfile_WriteFile_OS(hFile, newfilecontents, newsize);
    Bfile_CloseFile_OS(hFile);
  } else {
    return 2;
  }
  return 0;
}

void RemoveEvent(EventDate *startdate, CalendarEvent* events, const char* folder, int count, int calEventPos) {
  // removes SMEM event starting on startdate.
  // events: array from where to remove the event
  // count: current number of events in file (before deletion)
  // calEventPos: index (zero based) of the event to delete.
  // after the event is removed from the array, the file for the startdate is replaced with ReplaceEventFile
  if (count == 1) {
    RemoveDay(startdate, folder);
    return;
  }
  if (calEventPos >= count) {} // safety check
  else
  {
    int k;
    for (k = calEventPos; k < count - 1; k++)
            events[k] = events[k+1];
    count = count - 1; //this "deletes" the event
  }
  ReplaceEventFile(startdate, events, folder, count);
}

int RemoveDay(EventDate* date, const char* folder) {
  //remove all SMEM events for the day
  char filename[128] = "";
  smemFilenameFromDate(date, filename, folder);
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle to check if exists

  if(hFile < 0) { //error returned, file doesn't exist
    return 1;
  } else {
    //file exists and is open, close it and delete.
    Bfile_CloseFile_OS(hFile);
    Bfile_DeleteEntry(pFile);
    return 0;
  }
  return 0;
}

int GetEventsForDate(EventDate* startdate, const char* folder, CalendarEvent* calEvents, int limit, SimpleCalendarEvent* simpleCalEvents, int startArray) {
/*reads the storage memory searching for events starting on specified date.
  folder is where events will be searched for (useful for multiple calendar support)
  if calEvents is not NULL:
  puts events in the array passed as calEvents, which will be empty on error (no events for the day).
  if calEvents is NULL:
  does not parse the inner of events, only parses to count them.
  "limit" is the maximum number of events to parse. the number of events "found" will be at maximum this number,
  even if there are more events in the day. set to zero for no limit.
  returns number of events found for the specified day, 0 if error or no events. */

  // Generate filename from given date
  char filename[128] = "";
  smemFilenameFromDate(startdate, filename, folder);
  
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  // Check for file existence
  if(hFile >= 0) // Check if it opened
  {
    // Returned no error, file exists, open it
    int size = Bfile_GetFileSize_OS(hFile);
    // File exists and has size 'size'
    // Read file into a buffer which is then parsed and broke in multiple event strings.
    // These event strings are then turned into a CalendarEvent.
    if ((unsigned int)size > MAX_EVENT_FILESIZE) {
      Bfile_CloseFile_OS(hFile);
      return -1;
    } //file too big, return error.
    unsigned char asrc[MAX_EVENT_FILESIZE] = "";
    Bfile_ReadFile_OS(hFile, asrc, size, 0);
    Bfile_CloseFile_OS(hFile); //we got file contents, close it
    // Parse for events
    int curevent = 0; //current event number/array index (zero based)
    unsigned char token[2048];
    unsigned char* src = asrc;
    src = toksplit(src, EVENT_SEPARATOR , token, 2048);
    int notfinished = 1;
    while (notfinished) {
      //pass event to the parser and store it in the string event array
      if(calEvents != NULL) charToCalEvent(curevent==0? token+strlen(FILE_HEADER) : token, &calEvents[startArray+curevent]); //convert to a calendar event. if is first event on file, it comes with a header that needs to be skipped.
      // we don't want full CalendarEvents, but do we want SimpleCalendarEvents?
      else if(simpleCalEvents != NULL) {
        // calEvents is null, so we need to get our own CalendarEvent to hold the results
        CalendarEvent cEvt;
        charToCalEvent(curevent==0? token+strlen(FILE_HEADER) : token, &cEvt);
        strcpy((char*)simpleCalEvents[startArray+curevent].title, (char*)cEvt.title);
        simpleCalEvents[startArray+curevent].startdate.day = cEvt.startdate.day;
        simpleCalEvents[startArray+curevent].startdate.month = cEvt.startdate.month;
        simpleCalEvents[startArray+curevent].startdate.year = cEvt.startdate.year;
        simpleCalEvents[startArray+curevent].category = cEvt.category;
        simpleCalEvents[startArray+curevent].origpos = curevent;
      }
      curevent++;
      if (strlen((char*)src) < 5) { //5 bytes is not enough space to hold an event, so that means there are no more events to process... right?
        notfinished = 0;
        break; //force it to stop. strtok can't happen again, otherwise token will be null and a system error is approaching!
      }
      if (curevent >= MAX_DAY_EVENTS || (limit > 0 && curevent >= limit)) { //check if we aren't going over the limit
        //we are, return now. events past this point will be ignored.
        return curevent;
      }
      src = toksplit(src, EVENT_SEPARATOR , token, 2048);
    }
    return curevent; //return the number of events
  } else {
    // error returned, file doesn't exist, so no events. return 0.
    return 0;
  }
}

void GetEventCountsForMonthHelper(EventDate* date, int count, int* busydays) {
  CalendarEvent* events = (CalendarEvent*)alloca(count*sizeof(CalendarEvent));
  count = GetEventsForDate(date, CALENDARFOLDER, events);
  for(int curitem = 0; curitem <= count - 1; curitem++) {
    long int datediff = DateToDays(events[curitem].enddate.year, events[curitem].enddate.month, events[curitem].enddate.day) - DateToDays(events[curitem].startdate.year, events[curitem].startdate.month, events[curitem].startdate.day);
    if(datediff == 0) {
      busydays[date->day] = events[curitem].category;
    } else if(datediff > 0) {
      if(events[curitem].enddate.month > date->month || events[curitem].enddate.year > date->year) {
        // event ends after this month. which means the current month days are all busy past this day.
        for(unsigned int k = date->day; k<=31; k++) busydays[k] = events[curitem].category;
      } else {
        // events past this day up to the end day are all busy.
        for(unsigned int k = date->day; k<=events[curitem].enddate.day; k++) busydays[k] = events[curitem].category;
      }
    } // else: end date is before start date, which is invalid. user should repair DB some day...
  }
}
void GetEventCountsForMonth(int year, int month, int* buffer, int* busydays) {
  int day = 1;
  for(unsigned int k = 0; k<=31; k++) {
    busydays[k] = 0; //clean array
  }
  while (day <= getMonthDays(month) + ((month == 2 && isLeap(year)) ? 1 : 0))
  {
    EventDate thisday;
    thisday.day = day; thisday.month = month; thisday.year = year;
    buffer[day] = GetEventsForDate(&thisday, CALENDARFOLDER, NULL); //NULL means it will only count and not parse
    if(buffer[day] > 0) GetEventCountsForMonthHelper(&thisday, buffer[day], busydays);
    day++;
  }
}

void toggleTaskActivity(CalendarEvent* events, int pos, int count) {  
  //use the repeat setting as a task activity indicator. 1 is active/done (check), 0 is unchecked.
  if(events[pos].repeat == 1) {
    events[pos].repeat = 0;
  } else {
    events[pos].repeat = 1;
  }
  EventDate sdate;
  sdate.year = events[pos].startdate.year;
  sdate.month = events[pos].startdate.month;
  sdate.day = events[pos].startdate.day;
  ReplaceEventFile(&sdate, events, CALENDARFOLDER, count);
}

int SearchEventsOnDay(EventDate* date, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit) {
  /* reads the events on storage memory for a certain day
   * returns in calEvents the ones that contain needle (calEvents is a simplified events array, only contains event title and start date)
   * if calEvents is NULL simply returns the number of results
   * returns the search results count */
  int daynumevents = GetEventsForDate(date, folder, NULL); //get event count only so we know how much to alloc
  if(daynumevents==0) return 0;
  CalendarEvent* dayEvents = (CalendarEvent*)alloca(daynumevents*sizeof(CalendarEvent));
  daynumevents = GetEventsForDate(date, folder, dayEvents);
  int curitem = 0;
  int resCount = 0;
  while(curitem <= daynumevents-1) {
    int match = 0;
    if(NULL != strcasestr((char*)dayEvents[curitem].title, needle)) {
      match = 1;
    }
    if(NULL != strcasestr((char*)dayEvents[curitem].location, needle)) {
      match = 1;
    }
    if(NULL != strcasestr((char*)dayEvents[curitem].description, needle)) {
      match = 1;
    }
    if(match) {
      if(calEvents != NULL) {
        strcpy((char*)calEvents[resCount].title, (char*)dayEvents[curitem].title);
        calEvents[resCount].startdate = *date;
        calEvents[resCount].category = dayEvents[curitem].category;
        calEvents[resCount].origpos = curitem;
      }
      resCount++;
    }
    if(resCount == limit) break;
    curitem++;
  }
  return resCount;
}

void SearchYearHelper(EventDate* date, SimpleCalendarEvent* calEvents, int* resCount, int daynumevents, const char* folder, char* needle, int limit, int* curfpos) {
  CalendarEvent* dayEvents = (CalendarEvent*)alloca(daynumevents*sizeof(CalendarEvent));
  daynumevents = GetEventsForDate(date, folder, dayEvents);
  int curitem = 0;
  while(curitem <= daynumevents-1) {
    int match = 0;
    if(NULL != strcasestr((char*)dayEvents[curitem].title, needle)) {
      match = 1;
    }
    if(NULL != strcasestr((char*)dayEvents[curitem].location, needle)) {
      match = 1;
    }
    if(NULL != strcasestr((char*)dayEvents[curitem].description, needle)) {
      match = 1;
    }
    if(match) {
      if(calEvents != NULL) {
        strcpy((char*)calEvents[*curfpos].title, (char*)dayEvents[curitem].title);
        calEvents[*curfpos].startdate = *date;
        calEvents[*curfpos].category = dayEvents[curitem].category;
        calEvents[*curfpos].origpos = curitem;
      }
      *resCount = *resCount+1;
      *curfpos = *curfpos+1;
    }
    if(*resCount >= limit) return;
    curitem++;
  }
}

int SearchEventsOnYearOrMonth(int y, int m, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit, int arraystart) {
  // if m is zero, will search on year y
  int resCount = 0;
  int curfpos = arraystart;
  
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  unsigned char buffer[MAX_FILENAME_SIZE+1];
  unsigned char* filter = (unsigned char*)"*.pce";

  // make the buffer
  strcpy((char*)buffer, "\\\\fls0\\");
  strcat((char*)buffer, folder);
  strcat((char*)buffer, "\\");
  strcat((char*)buffer, "*");
  
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  Bfile_StrToName_ncpy(path, filter, MAX_FILENAME_SIZE+1);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    // the 00000.pce strcmp is there so we don't search on the tasks file
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 || strcmp((char*)buffer, "00000.pce") == 0) &&
      (fileinfo.fsize == 0 || Bfile_Name_MatchMask((const short int*)path, (const short int*)found)))
    {      
      // get the start date from the filename
      char mainname[20] = "";
      int nlen = strlen((char*)buffer);
      strncpy(mainname, (char*)buffer, nlen-4); //strip the file extension out
      // strcpy will not add a \0 at the end if the limit is reached, let's add it ourselves
      mainname[nlen-4] = '\0';
      nlen = strlen(mainname);
      
      int isValid = 1;
      // verify that it only contains numbers
      for(int i = 0; i < nlen; i++) {
        if(!isdigit(mainname[i])) {
          // some character is not a number, this file is invalid, do nothing on it
          isValid = 0;
        }
      }
      if(isValid) {
        char tmpbuf[10] = "";
        strcpy(tmpbuf, "");
        for(int i = 0; i<8-nlen; i++) {
          strcat(tmpbuf, "0");
        }
        strcat(tmpbuf, mainname);
        strcpy(mainname, tmpbuf);
        
        char datebuffer[10] = "";

        datebuffer[0] = mainname[0];
        datebuffer[1] = mainname[1];
        datebuffer[2] = mainname[2];
        datebuffer[3] = mainname[3];
        datebuffer[4] = '\0';
        EventDate thisday;
        thisday.year = atoi((const char*)datebuffer);
        datebuffer[0] = mainname[4];
        datebuffer[1] = mainname[5];
        datebuffer[2] = '\0';
        thisday.month = atoi((const char*)datebuffer);
        datebuffer[0] = mainname[6];
        datebuffer[1] = mainname[7];
        datebuffer[2] = '\0';
        thisday.day = atoi((const char*)datebuffer);
        
        // see if the date in the filename is valid, and that it is in the year we are searching in
        if(isDateValid(thisday.year,thisday.month,thisday.day) && thisday.year==(unsigned int)y && (m==0?1:thisday.month==(unsigned int)m)) {
          int daynumevents = GetEventsForDate(&thisday, folder, NULL); //get event count only so we know how much to alloc
          if(daynumevents>0) {
            SearchYearHelper(&thisday, calEvents, &resCount, daynumevents, folder, needle, limit, &curfpos);
            if(resCount >= limit) {
              Bfile_FindClose(findhandle);
              return resCount;
            }
          }
        }
      }
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  return resCount;
}

void repairEventsFile(char* name, const char* folder, int* checkedevents, int* problemsfound) {
  // name should only contain the latest part of the name, e.g. 20130415.pce
  // folder does not include \\fls0\ or anything, just the user friendly name for the folder
  
  // before anything, build a complete filename for the file
  char filename[128] = "";
  strcpy(filename, "\\\\fls0\\");
  strcat(filename, folder);
  strcat(filename, "\\");
  strcat(filename, name);
  
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  // Check if file opened
  int size = 0;
  if(hFile >= 0)
  { //opened
    size = Bfile_GetFileSize_OS(hFile);
    // File exists and we have its size, let's see if it is above the minimum
    if(size < 51) {
      Bfile_CloseFile_OS(hFile);
      Bfile_DeleteEntry(pFile);
      *problemsfound = *problemsfound + 1;
      return;
    }
    unsigned char asrc[MAX_EVENT_FILESIZE] = "";
    // Read file into a buffer
    if ((unsigned int)size > MAX_EVENT_FILESIZE) {
      Bfile_CloseFile_OS(hFile);
      // TODO do something with files that are too big.
      // delete them or tell the user to send them to the developers to see if any data can be recovered?
      // for now let's delete them
      Bfile_DeleteEntry(pFile);
      *problemsfound = *problemsfound + 1;
      return;
    }
    Bfile_ReadFile_OS(hFile, asrc, size, 0);
    Bfile_CloseFile_OS(hFile); //we got file contents, close it
    
    // Let's see if the file header is OK
    if(strncmp((char*)asrc, FILE_HEADER, 7)) {
      // strncmp was not zero, so the header is invalid.
      // delete file.
      Bfile_DeleteEntry(pFile);
      *problemsfound = *problemsfound + 1;
      return;
    }
    // before we give the file to GetEventsForDate, let's check if it has the right amount of field separators
    // in relation to the amount of event separators.
    int fieldsep = 0;
    int eventsep = 0;
    int len = strlen((char*)asrc);
    for(int i = 0; i<len; i++) {
      if(asrc[i] == FIELD_SEPARATOR) fieldsep++;
      if(asrc[i] == EVENT_SEPARATOR) eventsep++;
    }
    // there should be 19 field separators per event.
    // each event has a field separator in the end, even if it is the last event on file
    // so dividing the total amount of field separators by the total amount of event separators should return 19.
    if(fieldsep/eventsep != 19 || !fieldsep || !eventsep) {
      // file corrupt / invalid format, delete it.
      Bfile_DeleteEntry(pFile);
      *problemsfound = *problemsfound + 1;
      return;
    }
  } else return; // it should have opened...
  
  // if we got to this point, one could assume that the file doesn't suffer from corruption and has a valid format
  // but this is not the case, as we still need to check one thing, see:
  // apparently, we can now pass the file to GetEventsForDate, to check for "nicer" problems
  // but before we can pass the file to GetEventsForDate, we must figure out what start date it belongs to.
  // we must figure this out from the filename... problem: we haven't checked yet that the filename is valid!
  
  // following figures will exclude the file extension (".pce") - the file extension has already been checked by the GUI when listing files.
  // a valid filename will contain at least 5 numeric characters.
  // why five and not eight? Simple: due to a grandfathered bug (to keep compatibility), the year is not appended zeros when making up the filename...
  // so, a file for events starting on the 31st of January of year 5 will look like this: 50131
  // for year 100: 1000131, for year 2000: 20000131.
  // min strlen for name (with extension): 9
  // max strlen for name (with extension): 12
  int nlen = strlen(name);
  if(nlen < 9 || nlen > 12) {
    // invalid file name. while this should not mess with the remaining database system, let's delete it anyway.
    Bfile_DeleteEntry(pFile);
    *problemsfound = *problemsfound + 1;
    return;
  }
  char mainname[20] = "";
  strncpy(mainname, name, nlen-4); //strip the file extension out
  // strcpy will not add a \0 at the end if the limit is reached, let's add it ourselves
  mainname[nlen-4] = '\0';
  nlen = strlen(mainname);
  
  // last step in checking the file name: verify that it only contains numbers
  for(int i = 0; i < nlen; i++) {
    if(!isdigit(mainname[i])) {
      // some character is not a number, delete file
      Bfile_DeleteEntry(pFile);
      *problemsfound = *problemsfound + 1;
      return;
    }
  }
  char tmpbuf[10] = "";
  strcpy(tmpbuf, "");
  for(int i = 0; i<8-nlen; i++) {
    strcat(tmpbuf, "0");
  }
  strcat(tmpbuf, mainname);
  strcpy(mainname, tmpbuf);
  
  char datebuffer[10] = "";

  datebuffer[0] = mainname[0];
  datebuffer[1] = mainname[1];
  datebuffer[2] = mainname[2];
  datebuffer[3] = mainname[3];
  datebuffer[4] = '\0';
  EventDate thisday;
  thisday.year = atoi((const char*)datebuffer);
  datebuffer[0] = mainname[4];
  datebuffer[1] = mainname[5];
  datebuffer[2] = '\0';
  thisday.month = atoi((const char*)datebuffer);
  datebuffer[0] = mainname[6];
  datebuffer[1] = mainname[7];
  datebuffer[2] = '\0';
  thisday.day = atoi((const char*)datebuffer);
  
  // final step on filename checking: see if the date in the filename is valid
  // and that year is not zero (so we don't delete the tasks file)
  if(!isDateValid(thisday.year,thisday.month,thisday.day) && thisday.year) {
    // oops, date is not valid, and this is not the tasks file
    Bfile_DeleteEntry(pFile);
    *problemsfound = *problemsfound + 1;
    return;
  }
  
  int numitems = GetEventsForDate(&thisday, folder, NULL); //get event count only so we know how much to alloc
  CalendarEvent* events = (CalendarEvent*)alloca(numitems*sizeof(CalendarEvent));
  numitems = GetEventsForDate(&thisday, folder, events);
  *checkedevents = *checkedevents + numitems;
  int curitem = 0;
  int doneEdits = 0;
  while(curitem <= numitems-1) {
    // first of all, check that this is not a task:
    if(events[curitem].startdate.year || events[curitem].startdate.month || events[curitem].startdate.day) {
      // check validity of start date, start time, end date and end time individually
      if(!isDateValid(events[curitem].startdate.year,events[curitem].startdate.month,events[curitem].startdate.day) ||
        events[curitem].startdate.year != thisday.year || events[curitem].startdate.month != thisday.month ||
        events[curitem].startdate.day != thisday.day)
      {
        // start date is not valid, or does not belong on this file.
        //fix this by setting the start date to be the same as on the filename.
        doneEdits = 1;
        events[curitem].startdate.year = thisday.year;
        events[curitem].startdate.month = thisday.month;
        events[curitem].startdate.day = thisday.day;
        *problemsfound = *problemsfound + 1;
      }
      if(!isDateValid(events[curitem].enddate.year,events[curitem].enddate.month,events[curitem].enddate.day)) {
        // end date is not valid. set it to be the start date.
        doneEdits = 1;
        events[curitem].enddate.year = events[curitem].startdate.year;
        events[curitem].enddate.month = events[curitem].startdate.month;
        events[curitem].enddate.day = events[curitem].startdate.day;
        *problemsfound = *problemsfound + 1;
      }
      if(!isTimeValid(events[curitem].starttime.hour,events[curitem].starttime.minute,events[curitem].starttime.second)) {
        // the start time is not valid. set it to midnight.
        doneEdits = 1;
        events[curitem].starttime.second = 0;
        events[curitem].starttime.minute = 0;
        events[curitem].starttime.hour = 0;
        *problemsfound = *problemsfound + 1;
      }
      if(!isTimeValid(events[curitem].endtime.hour,events[curitem].endtime.minute,events[curitem].endtime.second)) {
        // the end time is not valid. set it to midnight.
        doneEdits = 1;
        events[curitem].endtime.second = 0;
        events[curitem].endtime.minute = 0;
        events[curitem].endtime.hour = 0;
        *problemsfound = *problemsfound + 1;
      }
      
      // check if end datetime is past start datetime. if not, modify end datetime to be the same as start datetime.
      long int timediff = (events[curitem].endtime.hour*60*60+events[curitem].endtime.minute*60+events[curitem].endtime.second) - (events[curitem].starttime.hour*60*60+events[curitem].starttime.minute*60+events[curitem].starttime.second);
      long int datediff = DateToDays(events[curitem].enddate.year, events[curitem].enddate.month, events[curitem].enddate.day) - DateToDays(events[curitem].startdate.year, events[curitem].startdate.month, events[curitem].startdate.day);
      int dwrong = 0;
      if(0<=datediff) {
        if(datediff==0) {
          if(0>timediff) dwrong=1;
        }
      } else {
        dwrong=1;
      }
      if(dwrong) {
        doneEdits=1;
        events[curitem].enddate.year = events[curitem].startdate.year;
        events[curitem].enddate.month = events[curitem].startdate.month;
        events[curitem].enddate.day = events[curitem].startdate.day;
        events[curitem].endtime.second = events[curitem].starttime.second;
        events[curitem].endtime.minute = events[curitem].starttime.minute;
        events[curitem].endtime.hour = events[curitem].starttime.hour;
        *problemsfound = *problemsfound + 1;
      }
    }
    // check if category is in allowed range
    if(events[curitem].category < 0 || events[curitem].category > 7) {
      doneEdits = 1;
      events[curitem].category = 1; // reset to black category
      *problemsfound = *problemsfound + 1;
    }
    curitem++;
  }
  if(doneEdits) {
    // replace event file...
    ReplaceEventFile(&thisday, events, folder, numitems);
  }
}