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
#include "chronoProvider.hpp"
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "stringsProvider.hpp"
#include "selectorGUI.hpp"
#include "calendarProvider.hpp" 
#include "fileProvider.hpp" 
#include "graphicsProvider.hpp" 

char* eventToString(CalendarEvent* calEvent, char* buf) {
  /* Parses a CalendarEvent struct and turns it into a string which can be written to a file.
     The resulting string is appended to the end of buf.
     The first field (category) begins with no separator.
     An event doesn't begin with any separators, and the last field ends with a field separator followed by an event separator.
     An event (as bytes) can take at most 1,3 KiB. The lengthy fields are obviously the title, the location and mainly the description.
     Returns a pointer to the end of the buffer. */
  char smallbuf[50]; 
  int zero = 0;
  itoa(calEvent->category, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 2); *(buf++) = FIELD_SEPARATOR;
  /*itoa(calEvent->daterange, (unsigned char*)smallbuf);*/ itoa(zero, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 2); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->startdate.day, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->startdate.month, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->startdate.year, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 5); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->enddate.day, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->enddate.month, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->enddate.year, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 5); *(buf++) = FIELD_SEPARATOR;
  /*itoa(calEvent->dayofweek, (unsigned char*)smallbuf);*/ itoa(zero, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 2); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->repeat, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 2); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->timed, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 2); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->starttime.hour, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->starttime.minute, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->starttime.second, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->endtime.hour, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->endtime.minute, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  itoa(calEvent->endtime.second, (unsigned char*)smallbuf);
  buf += strncpy_retlen(buf, smallbuf, 3); *(buf++) = FIELD_SEPARATOR;
  buf += strncpy_retlen(buf, calEvent->title, 21); *(buf++) = FIELD_SEPARATOR;
  buf += strncpy_retlen(buf, calEvent->location, 128); *(buf++) = FIELD_SEPARATOR;
  buf += strncpy_retlen(buf, calEvent->description, 1024); *(buf++) = EVENT_SEPARATOR;
  //the last field ends with an event separator (EVENT_SEPARATOR), without a field separator.
  *buf = 0; // null-terminate string
  return buf;
}

void stringToEvent(const char* src, CalendarEvent* calEvent) {
  /* Parses a string containing a single event and turns it into a CalendarEvent which the program can work with.
  */
  int curfield = 0; //field we are parsing currently. starts at the category, which is 0.
  char token[1024+6];
  src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  while (curfield < 20) {
    switch (curfield) {
      case 0: //category
        calEvent->category = atoi((const char*)token);
        break;
      case 1: //daterange
        //calEvent->daterange = atoi((const char*)token);
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
        //calEvent->dayofweek = atoi((const char*)token);
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
        strncpy((char*)calEvent->title, (char*)token, 21);
        calEvent->title[21] = '\0';
        break;
      case 18: //location
        strncpy_replace((char*)calEvent->location, (char*)token, 128, '\r', '\n');
        calEvent->location[128] = '\0';
        break;
      case 19: //description
        strncpy_replace((char*)calEvent->description, (char*)token, 1024, '\r', '\n');
        calEvent->description[1024] = '\0';
        break;
      default: //unknown field. may add special handling later.
        break;
    }
    curfield++;
    src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  }
}

void stringToSimpleEvent(const char* src, SimpleCalendarEvent* calEvent) {
  /* Parses a string containing a single event and turns it into a SimpleCalendarEvent which the program can work with.
     Skips all the fields not necessary to a SimpleCalendarEvent
  */
  int curfield = 0; //field we are parsing currently. starts at the category, which is 0.
  char token[1024];
  src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  while (curfield < 18) {
    switch (curfield) {
      case 0: //category
        calEvent->category = atoi((const char*)token);
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
      case 11: //starttime.hour
        calEvent->starttime.hour = atoi((const char*)token);
        break;
      case 12: //starttime.minute
        calEvent->starttime.minute = atoi((const char*)token);
        break;
      case 13: //starttime.second
        calEvent->starttime.second = atoi((const char*)token);
        break;
      case 17: //title
        strncpy((char*)calEvent->title, (char*)token, 21);
        calEvent->title[21] = '\0';
        break;
      default: //some field that doesn't matter to us
        break;
    }
    curfield++;
    src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  }
}

void eventDateToFilename(EventDate* date, unsigned short* shortfn, const char* folder) {
  char filename[MAX_FILENAME_SIZE];
  sprintf(filename, "%s\\%d%s%d%s%d.pce", folder, date->year, date->month < 10 ? "0" : "", date->month, date->day < 10 ? "0" : "", date->day); //filenameFromDate does not include file extension, so add it
  Bfile_StrToName_ncpy(shortfn, filename, MAX_FILENAME_SIZE); 
}

int compareEventDateTimes(EventDate* date1, EventTime* time1, EventDate* date2, EventTime* time2) {
  if     (date1->year   <  date2->year)   return -1;
  else if(date1->year   >  date2->year)   return 1;
  else if(date1->month  <  date2->month)  return -1;
  else if(date1->month  >  date2->month)  return 1;
  else if(date1->day    <  date2->day)    return -1;
  else if(date1->day    >  date2->day)    return 1;
  else if(time1->hour   <  time2->hour)   return -1;
  else if(time1->hour   >  time2->hour)   return 1;
  else if(time1->minute <  time2->minute) return -1;
  else if(time1->minute >  time2->minute) return 1;
  else if(time1->second <  time2->second) return -1;
  else if(time1->second >  time2->second) return 1;
  else return 0;
}

void sortCalendarEvents(CalendarEvent events[], int count) {
  int i, j;
  CalendarEvent temp;

  for(i = 1; i < count; i++) {
    temp = events[i];
    for (j = i - 1; j >= 0 && compareEventDateTimes(&events[j].startdate, &events[j].starttime, &temp.startdate, &temp.starttime) > 0; j--) {
      events[j + 1] = events[j];
    }
    events[j + 1] = temp;
  }
}

void sortSimpleCalendarEvents(SimpleCalendarEvent events[], int count) {
  int i, j;
  SimpleCalendarEvent temp;
  for(i = 1; i < count; i++) {
    temp = events[i];
    for (j = i - 1; j >= 0 && compareEventDateTimes(&events[j].startdate, &events[j].starttime, &temp.startdate, &temp.starttime) > 0; j--) {
      events[j + 1] = events[j];
    }
    events[j + 1] = temp;
  }
}

int addEvent(CalendarEvent* calEvent, const char* folder, int secondCall) {
  //Saves a calendar event on an existing calendar with specified file name.
  //If the specified file doesn't exist, it is created and the event is added to it.
  //Returns 0 on success, other values on error.
  char newevent[2048];
  int eventsize = eventToString(calEvent, newevent) - newevent;
  unsigned short pFile[MAX_FILENAME_SIZE];
  eventDateToFilename(&calEvent->startdate, pFile, folder);
  int hAddFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hAddFile < 0) // Check if it opened
  {
    size_t size = sizeof(FILE_HEADER) - 1 + eventsize;
    // Returned error, file might not exist, so create it
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres >= 0) // Did it create?
    {
      //open in order to write header and new event
      hAddFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // always open, since file did not exist so it must be closed
      if(hAddFile < 0) // Still failing?
      {
        setDBcorruptFlag(1);
        return 1;
      }
      char finalcontents[2060];
      strcpy(finalcontents, FILE_HEADER);
      strcat(finalcontents, newevent);
      //Write header and event
      Bfile_WriteFile_OS(hAddFile, finalcontents, size);
      Bfile_CloseFile_OS(hAddFile);
      return 0;
    } else {
      // file doesn't exist, but can't be created?
      // if we're calling ourselves for the second time, return:
      if(secondCall) {
        setDBcorruptFlag(1);
        return 2;
      }
      // it's probably because the DB has never been used and is not initialized (i.e. we haven't created the calendar folder).
      // create the folder:
      createFolderRecursive(folder);
      // now let's call ourselves again, then according to the return value of our second instance, decide if there was an error or not.
      if(addEvent(calEvent, folder,1)) {
        setDBcorruptFlag(1);
        return 2; // another error ocurred
      } else return 0; //event was added without error, now that we created the folder
    }
  } else {
    /*File exists and is open. Check its size and if its OK, append new event to it.
    (one does not need to recreate the file with new size when increasing the size) */
    int oldsize = Bfile_GetFileSize_OS(hAddFile);
    if (oldsize > MAX_EVENT_FILESIZE || oldsize+eventsize > MAX_EVENT_FILESIZE) { //file bigger than we can handle
      Bfile_CloseFile_OS(hAddFile);
      if(oldsize > MAX_EVENT_FILESIZE) setDBcorruptFlag(1);
      return 4;
    }
    Bfile_SeekFile_OS(hAddFile, oldsize); //move cursor to end
    Bfile_WriteFile_OS(hAddFile, newevent, eventsize); // write new event
    Bfile_CloseFile_OS(hAddFile);
  }
  return 0;
}

int replaceEventFile(EventDate *startdate, CalendarEvent* newEvents, const char* folder, int count) {
  // This basically deletes the events of startdate and replaces them with newEvents.
  // Allows for a way to edit an event on a day, when all the events for that day are in memory (including the edited one).
  // count: number of events in newEvents (number of events in old file doesn't matter). starts at 1.

  //convert the calevents back to char. assuming each event, as string, doesn't take more than 1250 bytes
  char newfilecontents[7+count*1250];
  strcpy(newfilecontents, (char*)FILE_HEADER); //we need to initialize the char, take the opportunity to add the file header
  char* contentptr = newfilecontents + sizeof(FILE_HEADER) - 1;
  for(int j = 0; j < count; j++) {
    contentptr = eventToString(&newEvents[j], contentptr);
  }
  size_t newsize = contentptr - newfilecontents;

  unsigned short pFile[MAX_FILENAME_SIZE];
  eventDateToFilename(startdate, pFile, folder);
  int hAddFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hAddFile < 0) {
    setDBcorruptFlag(1);
    return 1;
  }
  size_t oldsize = Bfile_GetFileSize_OS(hAddFile);
  if(newsize < oldsize) {
    // new file is smaller than old file; we must recreate with new size.
    Bfile_CloseFile_OS(hAddFile);
    Bfile_DeleteEntry(pFile);
    if(Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &newsize) < 0) return 2;
    hAddFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle again
    if(hAddFile < 0) {
      setDBcorruptFlag(1);
      return 3;
    }
  }
  
  // write new contents. WriteFile will take care of expanding the file if needed.
  Bfile_WriteFile_OS(hAddFile, newfilecontents, newsize);
  Bfile_CloseFile_OS(hAddFile);
  return 0;
}

void removeEvent(EventDate *startdate, CalendarEvent* events, const char* folder, int count, int calEventPos) {
  // removes SMEM event starting on startdate.
  // events: array from where to remove the event
  // count: current number of events in file (before deletion)
  // calEventPos: index (zero based) of the event to delete.
  // after the event is removed from the array, the file for the startdate is replaced with replaceEventFile
  if (count == 1) {
    removeDay(startdate, folder);
    return;
  }
  if (calEventPos < count) {
    for (int k = calEventPos; k < count - 1; k++)
            events[k] = events[k+1];
    count = count - 1; //this "deletes" the event
    replaceEventFile(startdate, events, folder, count);
  }
}

void removeDay(EventDate* date, const char* folder) {
  //remove all SMEM events for the day
  unsigned short pFile[MAX_FILENAME_SIZE];
  eventDateToFilename(date, pFile, folder);
  Bfile_DeleteEntry(pFile);
}

int getEvents(EventDate* startdate, const char* folder, CalendarEvent* calEvents, int limit, SimpleCalendarEvent* simpleCalEvents) {
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
  unsigned short pFile[MAX_FILENAME_SIZE];
  eventDateToFilename(startdate, pFile, folder);
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  // Check for file existence
  if(hFile >= 0) // Check if it opened
  {
    // Returned no error, file exists and is open
    int size = Bfile_GetFileSize_OS(hFile);
    // File exists and has size 'size'
    // Read file into a buffer which is then parsed and broke in multiple event strings.
    // These event strings are then turned into a CalendarEvent.
    if ((unsigned int)size > MAX_EVENT_FILESIZE) {
      //file too big, set repair flag and return error.
      Bfile_CloseFile_OS(hFile);
      setDBcorruptFlag(1);
      return -1;
    }
    char asrc[MAX_EVENT_FILESIZE] = "";
    Bfile_ReadFile_OS(hFile, asrc, size, 0);
    Bfile_CloseFile_OS(hFile); //we got file contents, close it
    // Parse for events
    int curevent = 0; //current event number/array index (zero based)
    char token[2048];
    const char* src = asrc;
    src = toksplit(src, EVENT_SEPARATOR , token, 2048);
    while (1) {
      //pass event to the parser and store it in the string event array
      if(calEvents != NULL) stringToEvent(curevent==0? token+sizeof(FILE_HEADER)-1 : token, &calEvents[curevent]); //convert to a calendar event. if is first event on file, it comes with a header that needs to be skipped.
      // we don't want full CalendarEvents, but do we want SimpleCalendarEvents?
      else if(simpleCalEvents != NULL) {
        stringToSimpleEvent(curevent==0? token+sizeof(FILE_HEADER)-1 : token, &simpleCalEvents[curevent]);
      }
      curevent++;
      if (strlen(src) < 5) { //5 bytes is not enough space to hold an event, so that means there are no more events to process... right?
        break; //stop now. strtok can't happen again, otherwise token will be null and a system error is approaching!
      }
      if (curevent >= MAX_DAY_EVENTS || (limit > 0 && curevent >= limit)) { //check if we aren't going over the limit
        //we are, return now. events past this point will be ignored.
        return curevent;
      }
      src = toksplit(src, EVENT_SEPARATOR , token, 2048);
    }
    if(curevent > 1 && startdate->month) { // only sort if there's more than one element and only if this is not a "special" file (tasks, wallet, TOTP, event import...)
      if(calEvents != NULL) sortCalendarEvents(calEvents, curevent);
      else if(simpleCalEvents != NULL) sortSimpleCalendarEvents(simpleCalEvents, curevent); // also sets origpos
    }
    if(simpleCalEvents != NULL) {
      // sets origpos (assumes sortCalendarEvents and sortSimpleCalendarEvents give equivalent results for equivalent arrays)
      for(int i = 0; i < curevent; i++) simpleCalEvents[i].origpos = i;
    }
    return curevent; //return the number of events
  } else {
    // error returned, file doesn't exist, so no events. return 0.
    return 0;
  }
}

void getEventCountsForMonthHelper(EventDate* date, int count, int* busydays) {
  CalendarEvent* events = (CalendarEvent*)alloca(count*sizeof(CalendarEvent));
  count = getEvents(date, CALENDARFOLDER, events);
  for(int curitem = 0; curitem < count; curitem++) {
    long int datediff = dateToDays(events[curitem].enddate.year, events[curitem].enddate.month, events[curitem].enddate.day) - dateToDays(events[curitem].startdate.year, events[curitem].startdate.month, events[curitem].startdate.day);
    if(datediff == 0) {
      busydays[date->day] = events[curitem].category;
    } else if(datediff > 0) {
      unsigned int endday = 0;
      if(events[curitem].enddate.month > date->month || events[curitem].enddate.year > date->year)
        // event ends after this month. which means the current month days are all busy past this day.
        endday = 31;
      else // events past this day up to the end day are all busy.
        endday =  events[curitem].enddate.day;
      for(unsigned int k = date->day; k<=endday; k++) {
        if(!busydays[k]) busydays[k] = events[curitem].category;
      }
    } else setDBcorruptFlag(1); //end date is before start date, which is invalid. user should repair DB...
  }
}
void getEventCountsForMonth(int year, int month, int* dbuffer, int* busydays) {
  memset(dbuffer, 0, 31*sizeof(int));
  memset(busydays, 0, 31*sizeof(int));

  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  char buffer[MAX_FILENAME_SIZE+1];

  // make the buffer
  sprintf(buffer, CALENDARFOLDER"\\%d%s%d*.pce", year, month < 10 ? "0" : "", month);
  
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 )) {      
      // get the start day from the filename
      int nlen = strlen((char*)buffer);
      if(isdigit(buffer[nlen-4-2]) && isdigit(buffer[nlen-4-1])) {
        char day[5] = {(char)buffer[nlen-4-2], (char)buffer[nlen-4-1], '\0'};
        int fd = sys_atoi(day);
        if(fd >= 1 && fd <= 31) {
          EventDate thisday;
          thisday.year=year; thisday.month=month; thisday.day=fd;
          dbuffer[fd] = getEvents(&thisday, CALENDARFOLDER, NULL); //NULL means it will only count and not parse
          if(dbuffer[fd] > 0) getEventCountsForMonthHelper(&thisday, dbuffer[fd], busydays);
        }
      }
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
}

int SearchHelper(EventDate* date, SimpleCalendarEvent* calEvents, int daynumevents, const char* folder, char* needle, int limit) {
  // limit is the maximum number of results
  int index = 0;
  CalendarEvent* dayEvents = (CalendarEvent*)alloca(daynumevents*sizeof(CalendarEvent));
  daynumevents = getEvents(date, folder, dayEvents);
  for(int curitem = 0; curitem < daynumevents; curitem++) {
    if(NULL != strcasestr((char*)dayEvents[curitem].title, needle) || \
      NULL != strcasestr((char*)dayEvents[curitem].location, needle) || \
      NULL != strcasestr((char*)dayEvents[curitem].description, needle)) {
      if(calEvents != NULL) {
        strncpy((char*)calEvents[index].title, (char*)dayEvents[curitem].title, 24);
        calEvents[index].title[24] = '\0';
        calEvents[index].startdate.day = dayEvents[curitem].startdate.day;
        calEvents[index].startdate.month = dayEvents[curitem].startdate.month;
        calEvents[index].startdate.year = dayEvents[curitem].startdate.year;
        calEvents[index].category = dayEvents[curitem].category;
        calEvents[index].origpos = curitem;
      }
      index++;
      if(index >= limit) return index;
    }
  }
  return index;
}

int searchEventsOnDay(EventDate* date, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit) {
  /* reads the events on storage memory for a certain day
   * returns in calEvents the ones that contain needle (calEvents is a simplified events array, only contains event title and start date)
   * if calEvents is NULL simply returns the number of results
   * returns the search results count */
  int daynumevents = getEvents(date, folder, NULL); //get event count only so we know how much to alloc
  if(daynumevents==0) return 0;
  return SearchHelper(date, calEvents, daynumevents, folder, needle, limit);
}

int searchEventsOnYearOrMonth(int y, int m, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit) {
  // if m is zero, will search on year y
  // limit is the maximum number of results
  int index = 0;
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  char buffer[MAX_FILENAME_SIZE+1];

  // make the buffer
  char mbuf[10]; mbuf[0] = 0;
  if(m) itoa_zeropad(m, mbuf, 2);
  sprintf(buffer, "%s\\%d%s*.pce", folder, y, mbuf);

  // calculate length of desired matches so we don't see events for year 1234 when searching in the year 123
  int correctLen = 12; // 8 chars (4 for year + 2 for month + 2 for day) + dot + extension
  if(y < 1000) correctLen--;
  if(y < 100) correctLen--;
  if(y < 10) correctLen--;
  
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if ((int)strlen((char*)buffer) == correctLen) {
      // get the start date from the filename
      int len = correctLen-4;
      // strip the file extension out:
      buffer[len] = '\0';
      int isValid = 1;
      // verify that it only contains digits
      for(int i = 0; i < len; i++) {
        if(!isdigit(buffer[i])) {
          // some character is not a digit, this file is invalid, do nothing on it
          isValid = 0;
        }
      }
      
      if(isValid) {
        char tmpbuf[10];
        memset(&tmpbuf, '0', sizeof(int)*(8-len));
        strcpy(tmpbuf+8-len, buffer);
        EventDate thisday;
        int signedYear, signedMonth, signedDay;
        stringToDate(tmpbuf, &signedYear, &signedMonth, &signedDay, 2);
        thisday.year = signedYear;
        thisday.month = signedMonth;
        thisday.day = signedDay;
        
        // see if the date in the filename is valid
        if(isDateValid(thisday.year,thisday.month,thisday.day)) {
          int daynumevents = getEvents(&thisday, folder, NULL); //get event count only so we know how much to alloc
          if(daynumevents > 0) {
            index += SearchHelper(&thisday, calEvents+index, daynumevents, folder, needle, limit);
            if(index >= limit) {
              Bfile_FindClose(findhandle);
              return index;
            }
          }
        }
      }
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  return index;
}

void fixEventString(unsigned char* asrc, int field_length, int max, int* len, int* i) {
  int excess = field_length - max;
  for (int k = *i - field_length + max; k < *len - excess; k++)
    asrc[k] = asrc[k+excess];
  *len -= excess; *i -= excess;
  asrc[*len] = '\0';
}

void repairEventFile(char* name, const char* folder, int* checkedevents, int* problemsfound) {
  // Repairs an event file when possible, or deletes it straight away if it has major problems.
  // Makes A LOT of assumptions about the format of files and their names, so one must check that files are
  // still taken as valid by this function after making changes to the file/storage format, and change the
  // code if necessary.
  // name should only contain the latest part of the name, e.g. 20130415.pce
  
  // before anything, build a complete filename for the file
  char filename[MAX_FILENAME_SIZE];
  strcpy(filename, folder);
  strcat(filename, "\\");
  strcat(filename, name);
  
  unsigned short pFile[MAX_FILENAME_SIZE];
  Bfile_StrToName_ncpy(pFile, filename, MAX_FILENAME_SIZE); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hFile < 0) return;
  // file opened
  int size = Bfile_GetFileSize_OS(hFile);
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
    Bfile_DeleteEntry(pFile);
    *problemsfound = *problemsfound + 1;
    return;
  }
  // before we give the file to getEvents, let's check if it has the right amount of field separators
  // in relation to the amount of event separators, and check that the length of certain fields is within bounds.
  int fieldsep = 0;
  int eventsep = 0;
  int fieldsep_this_event = 0;
  int field_length = 0;
  int invalid_field_length = 0; // if asrc is modified to fix a lengthy field, this becomes 1
  int len = strlen((char*)asrc);
  for(int i = 0; i<len; i++) {
    if(asrc[i] == FIELD_SEPARATOR && (i > 0 && !isMBsecond(asrc[i-1]))) { // skip the second char of multi-byte chars
      // end of a field, check length
      if (fieldsep_this_event == 17 && field_length > 21) {
        // invalid title length
        fixEventString(asrc, field_length, 21, &len, &i);
        invalid_field_length = 1;
        *problemsfound = *problemsfound + 1;
      } else if (fieldsep_this_event == 18 && field_length > 128) {
        // invalid location length
        fixEventString(asrc, field_length, 128, &len, &i);
        invalid_field_length = 1;
        *problemsfound = *problemsfound + 1;
        break;
      }
      fieldsep++;
      fieldsep_this_event++;
      field_length = 0;
    } else if(asrc[i] == EVENT_SEPARATOR && (i > 0 && !(asrc[i-1]&128))) {  // skip the second char of multi-byte chars
      if (field_length > 1024) {
        // invalid description length
        fixEventString(asrc, field_length, 1024, &len, &i);
        invalid_field_length = 1;
        *problemsfound = *problemsfound + 1;
        break;
      }
      eventsep++;
      fieldsep_this_event = 0;
      field_length = 0;
    } else field_length++;
  }
  // there should be 19 field separators per event.
  // each event has a field separator in the end, even if it is the last event on file
  // so dividing the total amount of field separators by the total amount of event separators should return 19.
  if(!fieldsep || !eventsep || fieldsep/eventsep != 19) {
    // file corrupt / invalid format, delete it.
    Bfile_DeleteEntry(pFile);
    *problemsfound = *problemsfound + 1;
    return;
  } else if (invalid_field_length) {
    // delete original file and recreate with fixed contents
    Bfile_DeleteEntry(pFile);
    unsigned int size = (unsigned int)len;
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres >= 0) {
      int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
      if(hFile >= 0) {
        Bfile_WriteFile_OS(hFile, asrc, size);
        Bfile_CloseFile_OS(hFile);
      }
    }
  }
  
  // if we got to this point, one could assume that the file doesn't suffer from corruption and has a valid format
  // but this is not the case, as we still need to check one thing, see:
  // apparently, we can now pass the file to getEvents, to check for "nicer" problems
  // but before we can pass the file to getEvents, we must figure out what start date it belongs to.
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
  nlen -= 4;
  name[nlen] = '\0'; //strip the file extension out
  
  // last step in checking the file name: verify that it only contains numbers
  for(int i = 0; i < nlen; i++) {
    if(!isdigit(name[i])) {
      // some character is not a number, delete file
      Bfile_DeleteEntry(pFile);
      *problemsfound = *problemsfound + 1;
      return;
    }
  }
  char tmpbuf[10];
  memset(&tmpbuf, '0', sizeof(int)*(8-nlen));
  strcpy(tmpbuf+8-nlen, name);
  EventDate thisday;
  int y, m, d;
  stringToDate(tmpbuf, &y, &m, &d, 2);
  thisday.year=y; thisday.month=m; thisday.day=d;
  
  // final step on filename checking: see if the date in the filename is valid
  // and that year, month and date are not zero (so we don't delete the tasks file)
  if(!isDateValid(thisday.year,thisday.month,thisday.day) && (thisday.year || thisday.month || thisday.day)) {
    // oops, date is not valid, and this is not the tasks file
    Bfile_DeleteEntry(pFile);
    *problemsfound = *problemsfound + 1;
    return;
  }
  
  int numitems = getEvents(&thisday, folder, NULL); //get event count only so we know how much to alloc
  *checkedevents = *checkedevents + numitems;
  CalendarEvent* events = (CalendarEvent*)alloca(numitems*sizeof(CalendarEvent));
  numitems = getEvents(&thisday, folder, events);
  int curitem = 0;
  int doneEdits = 0;
  while(curitem < numitems) {
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
      if(compareEventDateTimes(&events[curitem].enddate, &events[curitem].endtime, &events[curitem].startdate, &events[curitem].starttime) < 0) {
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
    replaceEventFile(&thisday, events, folder, numitems);
  }
}
int dbRepairFlag = 0; // 1 when needs repairing
void setDBcorruptFlag(int value) {
  dbRepairFlag=value;
}

int getDBcorruptFlag() {
  return dbRepairFlag;
}