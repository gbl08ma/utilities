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

#include "constantsProvider.hpp"
#include "chronoProvider.hpp"
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "selectorGUI.hpp"
#include "calendarProvider.hpp" 

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

  itoa(calEvent->category, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->daterange, (unsigned char*)smallbuf);  strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->startdate.day, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->startdate.month, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->startdate.year, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->enddate.day, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->enddate.month, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->enddate.year, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->dayofweek, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->repeat, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->timed, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->starttime.hour, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->starttime.minute, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->starttime.second, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->endtime.hour, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->endtime.minute, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent->endtime.second, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  strcat((char*)buf, (char*)calEvent->title); append(buf, FIELD_SEPARATOR);
  strcat((char*)buf, (char*)calEvent->location); append(buf, FIELD_SEPARATOR);
  strcat((char*)buf, (char*)calEvent->description); append(buf, EVENT_SEPARATOR);
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
        //calEvent->title = (unsigned char*)token;
        strcpy((char*)calEvent->title, (char*)token);
        break;
      case 18: //location
        //calEvent->location = (unsigned char*)token;
        strcpy((char*)calEvent->location, (char*)token);
        break;
      case 19: //description
        strcpy((char*)calEvent->description, (char*)token);
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
  // open and close: should always be 1. If 0, it will be assumed that the file is already open/closed and global var hAddFile will be used as handle
  char foldername[128] = "";
  unsigned short pFolder[256];
  strcpy(foldername, "\\\\fls0\\");
  strcat(foldername, folder);
  Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file
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
      return 2;
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
  if (count == 1) RemoveDay(startdate, folder);
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

int GetEventsForDate(EventDate* startdate, const char* folder, CalendarEvent* calEvents, int limit) {
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
      if(calEvents != NULL) charToCalEvent(curevent==0? token+strlen(FILE_HEADER) : token, &calEvents[curevent]); //convert to a calendar event. if is first event on file, it comes with a header that needs to be skipped.
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

void GetEventCountsForMonth(int year, int month, int* buffer) {
  int day = 1;
  while (day <= getMonthDays(month) + ((month == 2 && isLeap(year)) ? 1 : 0))
  {
    EventDate thisday;
    thisday.day = day; thisday.month = month; thisday.year = year;
    buffer[day] = GetEventsForDate(&thisday, CALENDARFOLDER, NULL); //NULL means it will only count and not parse
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
int SearchEventsOnMonth(int y, int m, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit) {
  /* goes through the events on storage memory for a certain month
   * returns in calEvents the ones that contain needle (calEvents is a simplified events array, only contains event title and start date)
   * if calEvents is NULL simply returns the number of results
   * returns the search results count */
  int d = 1;
  int resCount = 0;
  int mdays = getMonthDays(m);
  if(m == 2) if(isLeap(y)) mdays = 29;
  while(d<=mdays) {
    EventDate date;
    date.year = y; date.month = m; date.day = d;
    int daynumevents = GetEventsForDate(&date, folder, NULL); //get event count only so we know how much to alloc
    if(daynumevents==0) { d++; continue; }
    CalendarEvent* dayEvents = (CalendarEvent*)alloca(daynumevents*sizeof(CalendarEvent));
    daynumevents = GetEventsForDate(&date, folder, dayEvents);
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
          strcpy((char*)calEvents[resCount].title, (char*)dayEvents[curitem].title);
          calEvents[resCount].startdate = date;
          calEvents[resCount].category = dayEvents[curitem].category;
          calEvents[resCount].origpos = curitem;
        }
        resCount++;
      }
      if(resCount == limit) return resCount;
      curitem++;
    }
    d++;
  }
  return resCount;
}

int SearchEventsOnYear(int y, const char* folder, SimpleCalendarEvent* calEvents, char* needle, int limit, int arraystart) {
  /* goes through the events on storage memory for a certain month
   * returns in calEvents the ones that contain needle (calEvents is a simplified events array, only contains event title and start date)
   * if calEvents is NULL simply returns the number of results
   * returns the search results count */
  int m = 1;
  int resCount = 0;
  int curfpos = arraystart;
  while(m<=12) {
    int d = 1;
    int mdays = getMonthDays(m);
    if(m == 2) if(isLeap(y)) mdays = 29;
    while(d<=mdays) {
      EventDate date;
      date.year = y; date.month = m; date.day = d;
      int daynumevents = GetEventsForDate(&date, folder, NULL); //get event count only so we know how much to alloc
      if(daynumevents==0) { d++; continue; }
      CalendarEvent* dayEvents = (CalendarEvent*)alloca(daynumevents*sizeof(CalendarEvent));
      daynumevents = GetEventsForDate(&date, folder, dayEvents);
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
            strcpy((char*)calEvents[curfpos].title, (char*)dayEvents[curitem].title);
            calEvents[curfpos].startdate = date;
            calEvents[curfpos].category = dayEvents[curitem].category;
            calEvents[curfpos].origpos = curitem;
          }
          resCount++;
          curfpos++;
        }
        if(resCount == limit) return resCount;
        curitem++;
      }
      d++;
    }
    m++;
  }
  return resCount;
}

char tolower(char c)
{
  if (c >= 'A' && c <= 'Z')
          c += 32;
  return c;
}
int strncasecmp(const char *s1, const char *s2, size_t n)
{
        if (n != 0) {
                const unsigned char *us1 = (const unsigned char *)s1;
                const unsigned char *us2 = (const unsigned char *)s2;

                do {
                        if (tolower(*us1) != tolower(*us2++))
                                return (tolower(*us1) - tolower(*--us2));
                        if (*us1++ == '\0')
                                break;
                } while (--n != 0);
        }
        return (0);
}
char *strcasestr(const char *s, const char *find)
{
        char c, sc;
        size_t len;

        if ((c = *find++) != 0) {
                c = tolower((unsigned char)c);
                len = strlen(find);
                do {
                        do {
                                if ((sc = *s++) == 0)
                                        return (NULL);
                        } while ((char)tolower((unsigned char)sc) != c);
                } while (strncasecmp(s, find, len) != 0);
                s--;
        }
        return ((char *)s);
}


/* copy over the next token from an input string, after
skipping leading blanks (or other whitespace?). The
token is terminated by the first appearance of tokchar,
or by the end of the source string.

The caller must supply sufficient space in token to
receive any token, Otherwise tokens will be truncated.

Returns: a pointer past the terminating tokchar.

This will happily return an infinity of empty tokens if
called with src pointing to the end of a string. Tokens
will never include a copy of tokchar.

A better name would be "strtkn", except that is reserved
for the system namespace. Change to that at your risk.

released to Public Domain, by C.B. Falconer.
Published 2006-02-20. Attribution appreciated.
*/

unsigned char *toksplit(unsigned char *src, /* Source of tokens */
char tokchar, /* token delimiting char */
unsigned char *token, /* receiver of parsed token */
int lgh) /* length token can receive */
/* not including final '\0' */
{
  if (src) {
    //while (' ' == *src) *src++;
    while (' ' == *src) *src=*src+1; // avoids a compiler warning

    while (*src && (tokchar != *src)) {
      if (lgh) {
        *token++ = *src;
        --lgh;
      }
      src++;
    }
    if (*src && (tokchar == *src)) src++;
  }
  *token = '\0';
  return src;
} /* toksplit */
