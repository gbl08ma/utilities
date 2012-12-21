#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/misc.h>
#include <fxcg/keyboard.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define CREATEMODE_FILE 1
#define CREATEMODE_FOLDER 5
#define READ 0
#define READ_SHARE 1
#define WRITE 2
#define READWRITE 3
#define READWRITE_SHARE 4
#include "events.hpp"
#include "toksplit.h"

#define FILE_HEADER "PCALEVT" //Prizm CALendar EVenT / Portable CALendar EVenT
#define FIELD_SEPARATOR '\x1D'
#define EVENT_SEPARATOR '\x1E'

//calendar and calendar event management
void append(unsigned char* s, char c) //beacuse strcat seems to have problems with 1-byte non-null-terminated chars like the field and event separators
{
  int len = strlen((char*)s);
  s[len] = c;
  s[len + 1] = '\0';
}
void calEventToChar(unsigned char* buf, CalendarEvent calEvent) {
  /* Parses a CalendarEvent struct and turns it into a string which can be written to a MCS entry or archived in a file.
     Field separator: 0x1D
     Event separator: 0x1E
     The first field (category) begins with no separator.
     An event doesn't begin with any separators, and the last field ends with a field separator followed by an event separator.
     An event can have at least 4KB size. The lengthy fields are obviously the title, the location and mainly the description.*/
  unsigned char smallbuf[50] = ""; 

  itoa(calEvent.category, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.daterange, (unsigned char*)smallbuf);  strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.startdate.day, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.startdate.month, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.startdate.year, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.enddate.day, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.enddate.month, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.enddate.year, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.dayofweek, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.repeat, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.timed, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.starttime.hour, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.starttime.minute, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.starttime.second, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.endtime.hour, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.endtime.minute, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  itoa(calEvent.endtime.second, (unsigned char*)smallbuf); strcat((char*)buf, (char*)smallbuf); append(buf, FIELD_SEPARATOR);
  strcat((char*)buf, (char*)calEvent.title); append(buf, FIELD_SEPARATOR);
  strcat((char*)buf, (char*)calEvent.location); append(buf, FIELD_SEPARATOR);
  strcat((char*)buf, (char*)calEvent.description); append(buf, EVENT_SEPARATOR);
  //the last field ends with a field separator followed by an event separator. both are contained in EVENT_SEPARATOR
}
/* End of date add/subtract functions */
CalendarEvent charToCalEvent(unsigned char* src) {
  /* Parses a string containing a single event and turns it into a CalendarEvent which the program can work with.
  */
  CalendarEvent calEvent;
  int curfield = 0; //field we are parsing currently. starts at the category, which is 0.
  unsigned char token[1024];
  strcpy((char*)token, "");
  src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  int notfinished = 1;
  while (notfinished) {
    switch (curfield) {
      case 0: //category
        calEvent.category = atoi((const char*)token);
        break;
      case 1: //daterange
        calEvent.daterange = atoi((const char*)token);
        break;
      case 2: //startdate.day
        calEvent.startdate.day = atoi((const char*)token);
        break;
      case 3: //startdate.month
        calEvent.startdate.month = atoi((const char*)token);
        break;
      case 4: //startdate.year
        calEvent.startdate.year = atoi((const char*)token);
        break;
      case 5: //enddate.day
        calEvent.enddate.day = atoi((const char*)token);
        break;
      case 6: //enddate.month
        calEvent.enddate.month = atoi((const char*)token);
        break;
      case 7: //enddate.year
        calEvent.enddate.year = atoi((const char*)token);
        break;
      case 8: //dayofweek
        calEvent.dayofweek = atoi((const char*)token);
        break;
      case 9: //repeat
        calEvent.repeat = atoi((const char*)token);
        break;
      case 10: //timed
        calEvent.timed = atoi((const char*)token);
        break;
      case 11: //starttime.hour
        calEvent.starttime.hour = atoi((const char*)token);
        break;
      case 12: //starttime.minute
        calEvent.starttime.minute = atoi((const char*)token);
        break;
      case 13: //starttime.second
        calEvent.starttime.second = atoi((const char*)token);
        break;
      case 14: //endtime.hour
        calEvent.endtime.hour = atoi((const char*)token);
        break;
      case 15: //endtime.minute
        calEvent.endtime.minute = atoi((const char*)token);
        break;
      case 16: //endtime.second
        calEvent.endtime.second = atoi((const char*)token);
        break;
      case 17: //title
        //calEvent.title = (unsigned char*)token;
        strcpy((char*)calEvent.title, (char*)token);
        break;
      case 18: //location
        //calEvent.location = (unsigned char*)token;
        strcpy((char*)calEvent.location, (char*)token);
        break;
      case 19: //description
        strcpy((char*)calEvent.description, (char*)token);
        notfinished = 0;
        break;
      default: //unknown field. may add special handling later.
        break;
    }
    curfield++;
    src = toksplit(src, FIELD_SEPARATOR, token, 1024);
  }
  return calEvent;
}

int AddSMEMEvent(CalendarEvent calEvent, const char* folder) {
  //Saves a calendar event on an existing calendar with specified file name.
  //If the specified file doesn't exist, it is created and the event is added to it.
  //Returns 0 on success, other values on error.
  PrintXY(1,8,(char*)"  Please wait           ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  Bdisp_PutDisp_DD();
  char foldername[128] = "";
  unsigned short pFolder[256];
  strcpy(foldername, "\\\\fls0\\");
  strcat(foldername, folder);
  Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file
  char filename[128] = "";
  char buffer[8] = "";
  strcpy(filename, "\\\\fls0\\");
  strcat(filename, folder);
  strcat(filename, "\\");
  filenameFromDate(calEvent.startdate, buffer);
  strcat(filename, buffer);
  strcat(filename, ".pce"); //filenameFromDate does not include file extension, so add it
  char newevent[4098] = "";
  calEventToChar((unsigned char*)newevent, calEvent);
  int size = strlen(FILE_HEADER) + strlen(newevent);
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  PrintXY(1,8,(char*)"  Please wait.          ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  Bdisp_PutDisp_DD();
  if(hFile < 0) // Check if it opened
  {
    // Returned error, file might not exist, so create it
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres >= 0) // Did it create?
    {
      PrintXY(1,8,(char*)"  Please wait..         ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      //open in order to write header and new event
      hFile = Bfile_OpenFile_OS(pFile, READWRITE);
      if(hFile < 0) // Still failing?
      {
        return 1;
      }
      //Write header
      PrintXY(1,8,(char*)"  Please wait...        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      Bfile_WriteFile_OS(hFile, FILE_HEADER, strlen(FILE_HEADER));
      //Write event
      Bfile_WriteFile_OS(hFile, newevent, strlen(newevent));
      PrintXY(1,8,(char*)"  Please wait....       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      Bfile_CloseFile_OS(hFile);
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
    int oldsize = Bfile_GetFileSize_OS(hFile, Bfile_TellFile_OS( hFile ));
    int newsize = oldsize + strlen(newevent);
    if (oldsize > MAX_EVENT_FILESIZE || newsize > MAX_EVENT_FILESIZE) { Bfile_CloseFile_OS(hFile); return 4; } //file bigger than we can handle
    unsigned char oldcontents[MAX_EVENT_FILESIZE] = "";
    if(oldsize) {
  	  Bfile_ReadFile_OS(hFile, oldcontents, oldsize, 0);
    }
    PrintXY(1,8,(char*)"  Please wait..         ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    Bfile_CloseFile_OS(hFile);
    Bfile_DeleteEntry(pFile);
    PrintXY(1,8,(char*)"  Please wait...        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    // we already read the previous contents and size, closed the file and deleted it.
    // now recreate it with new size and write new contents to it.

    int nBCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &newsize);
    if(nBCEres >= 0) // Did it create?
    {
      PrintXY(1,8,(char*)"  Please wait....       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      int nFile = Bfile_OpenFile_OS(pFile, READWRITE);
      PrintXY(1,8,(char*)"  Please wait.....      ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      Bfile_WriteFile_OS(nFile, oldcontents, oldsize);
      Bfile_WriteFile_OS(nFile, newevent, strlen(newevent));
      PrintXY(1,8,(char*)"  Please wait......     ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      Bfile_CloseFile_OS(nFile);
    } else {
      return 3;
    }

  }
  return 0;
}
void deleteElement(CalendarEvent arr[], int& size, int position)
{
	int k;

	if (position >= size) {}
		
	else
	{
		for (k = position; k < size - 1; k++)
			arr[k] = arr[k+1];
		--size;
	}
}
int GetSMEMeventsForDate(EventDate startdate, const char* folder, CalendarEvent calEvents[]);
int RemoveSMEMDay(EventDate date, const char* folder);

int RemoveSMEMEvent(EventDate startdate, int calEventPos, const char* folder) {
PrintXY(1,8,(char*)"  Please wait           ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
Bdisp_PutDisp_DD();
  //Deletes a calendar event on an existing calendar with specified file name.
  //If the specified event doesn't exist, error is returned.
  //Returns 0 on success, other values on error.
  char foldername[128] = "";
  unsigned short pFolder[256];
  strcpy(foldername, "\\\\fls0\\");
  strcat(foldername, folder);
  Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file
  char filename[128] = "";
  char buffer[8] = "";
  strcpy(filename, "\\\\fls0\\");
  strcat(filename, folder);
  strcat(filename, "\\");
  filenameFromDate(startdate, buffer);
  strcat(filename, buffer);
  strcat(filename, ".pce"); //filenameFromDate does not include file extension, so add it
  
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  Bfile_CloseFile_OS(hFile); //close file as we don't care what it has, we just want to check if it exists
PrintXY(1,8,(char*)"  Please wait.          ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
Bdisp_PutDisp_DD();
  if(hFile < 0) // Check if it opened
  {
    //returned error, so there're no events on this day, so we return error too
    return 1;
  } else {
    /*File exists and is open.
      0. Hope there's enough heap to store everything throughout the process.
      1. Read and parse the file contents, putting their events in an array (all using GetSMEMeventsForDate)
      2. Close, delete the (old) file.
      3. Look at the array so we find the event we want to delete. Take it off the array.
      4. Put the new array (without the deleted event) into a char.
      5. Create the same file with the previous size minus the size of the event that was deleted.
      6. Open the new file.
      7. Write header and previous contents minus the deleted event, from the char we created previously.
      8. Close file.
      It must be done this way because once a file is created, its size cannot be changed...*/

    //parse the old contents before deleting.
    CalendarEvent oldcalevents[MAX_DAY_EVENTS];
    int numarrayitems = GetSMEMeventsForDate(startdate, folder, oldcalevents);
    // we already read the previous contents and size, and parsed the contents, then closed the file.
    // safety check: see if GetSMEMevents didn't return error/no events
    if (numarrayitems <= 0) {
      return 2;
    }
    if (numarrayitems == 1) {
      //if there's only one event on this day, RemoveSMEMDay works much better (it removes file header).
      PrintXY(1,8,(char*)"  Please wait..         ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
      RemoveSMEMDay(startdate, folder);
      return 0; //stop now
    }
    deleteElement(oldcalevents, numarrayitems, calEventPos);
    const int numevents = numarrayitems; //put this in a const int so it isn't polluted when there are lots of calendar events/very big events.
    // GetSMEMEvents closed the file, so we can now delete it
    PrintXY(1,8,(char*)"  Please wait..         ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    Bfile_DeleteEntry(pFile);
    PrintXY(1,8,(char*)"  Please wait...        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    //convert the calevents back to char
    unsigned char eventbuf[2048] = ""; 
    unsigned char newfilecontents[MAX_EVENT_FILESIZE];
conversionstart:
    strcpy((char*)newfilecontents, (char*)FILE_HEADER); //we need to initialize the char, take the opportunity to add the file header
    for(int j = 0; j < numevents; j++) {
      strcpy((char*)eventbuf, "");
      calEventToChar(eventbuf, oldcalevents[j]);
      strcat((char*)newfilecontents,(char*)eventbuf);
    }
    PrintXY(1,8,(char*)"  Please wait....       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    // now recreate file with new size and write contents to it.
    int newsize = strlen((char*)newfilecontents);
    
    if(newsize < 10) {
      //less than 10 bytes is too small for an event, and this means the infamous
      //bug that results in a file with just the header started to fly.
      //this is a very ugly approach, but best than no solution at all / data deletion without information
      //note: doing the conversion again doesn't seem to do anything, most of the time.
      MsgBoxPush(5);
      int textX=18*2, textY=18;
      PrintMini(&textX, &textY, (unsigned char*)"Potential data corruption", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textY = textY+17; textX=18*2;
      PrintMini(&textX, &textY, (unsigned char*)"detected. Do you want to try", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textY = textY+17; textX=18*2;
      PrintMini(&textX, &textY, (unsigned char*)"saving the events for this day", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textY = textY+17; textX=18*2;
      PrintMini(&textX, &textY, (unsigned char*)"again, or do you want to", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textY = textY+17; textX=18*2;
      PrintMini(&textX, &textY, (unsigned char*)"continue with potential data", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textY = textY+17; textX=18*2;
      PrintMini(&textX, &textY, (unsigned char*)"loss?", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textY = textY+24; textX=18*2;
      PrintMini(&textX, &textY, (unsigned char*)"[F1]:Try again   [F6]:Continue", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      int key,inscreen=1;
      while(inscreen) {
        GetKey(&key);
        switch(key)
        {
          case KEY_CTRL_F1:
            goto conversionstart;
            break;
          case KEY_CTRL_F6:
            inscreen=0;
            break;
        }
      }
      MsgBoxPop();
    }
    int nBCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &newsize);
    PrintXY(1,8,(char*)"  Please wait.....      ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    if(nBCEres >= 0) // Did it create?
    {
      int nFile = Bfile_OpenFile_OS(pFile, READWRITE);
      Bfile_WriteFile_OS(nFile, newfilecontents, newsize);
      Bfile_CloseFile_OS(nFile);
      PrintXY(1,8,(char*)"  Please wait......     ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
    } else {
      return 3;
    }

  }
  return 0;
}

int EditSMEMEvent(EventDate startdate, int calEventPos, const char* folder, CalendarEvent editedEvent) {
PrintXY(1,8,(char*)"  Please wait           ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
Bdisp_PutDisp_DD();
  //Edits a calendar event on an existing calendar with specified folder name.
  //If the specified event doesn't exist, error is returned.
  //If the startdate of the edited event is different from the startdate, the event is moved into another day.
  //Because of this, startdate should be the date where the event was located before editing. Of course, calEventPos is also the position in the old file.
  //If the event needs to be moved, it is first added to another day (with AddSMEMEvent, which will either create a new file or append to a existing one), then
  //removed from the startdate day (with RemoveSMEMEvent) (removing is done in the end to avoid any data loss - the most that can happen is duplicated events). 
  //Returns 0 on success, other values on error.
  
  //Check if event needs to be moved to another file or not, as the procedure is completely different when it needs:  
  if(startdate.day != editedEvent.startdate.day || startdate.month != editedEvent.startdate.month || startdate.year != editedEvent.startdate.year) {
    //Add the event to the new day file.
    int opresult = AddSMEMEvent(editedEvent, folder);
    if(opresult != 0) { return opresult+100; } //return error if adding returned error
    
    //Remove the event from the previous day file.
    opresult = RemoveSMEMEvent(startdate, calEventPos, folder);
    if(opresult != 0) { return opresult+100; } //return error if deleting returned error
    return 0;
  } else {
  
  
  
  char foldername[128] = "";
  unsigned short pFolder[256];
  strcpy(foldername, "\\\\fls0\\");
  strcat(foldername, folder);
  Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file
  char filename[128] = "";
  char buffer[8] = "";
  strcpy(filename, "\\\\fls0\\");
  strcat(filename, folder);
  strcat(filename, "\\");
  filenameFromDate(startdate, buffer);
  strcat(filename, buffer);
  strcat(filename, ".pce"); //filenameFromDate does not include file extension, so add it
  
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  Bfile_CloseFile_OS(hFile); //close file as we don't care what it has, we just want to check if it exists
PrintXY(1,8,(char*)"  Please wait.          ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
Bdisp_PutDisp_DD();
  if(hFile < 0) // Check if it opened
  {
    //returned error, so there're no events on this day, so we return error too
    return 1;
  } else {
    /*File exists and is open.
      0. Hope there's enough heap to store everything throughout the process.
      1. Read and parse the file contents, putting their events in an array (all using GetSMEMeventsForDate)
      2. Close, delete the (old) file.
      3. Look at the array so we find the event we want to edit. Change each of its fields to the fields of the new event.
      4. Put the new array (with the modified event) into a char array.
      5. Create the same file with the size of the char array with the modified event.
      6. Open the new file.
      7. Write header and edited contents, from the char we created previously.
      8. Close file.
      It must be done this way because once a file is created, its size cannot be changed dynamically...*/

    //parse the old contents.
    CalendarEvent calevents[MAX_DAY_EVENTS];
    int numarrayitems = GetSMEMeventsForDate(startdate, folder, calevents);
    // we already read the previous contents and size, and parsed the contents, then closed the file.
    // safety check: see if GetSMEMevents didn't return error/no events
    if (numarrayitems <= 0) {
      return 2;
    }
    
    //Edit the event at the specified position:
    calevents[calEventPos].category = editedEvent.category;
    calevents[calEventPos].daterange = editedEvent.daterange;
    calevents[calEventPos].startdate.day = editedEvent.startdate.day;
    calevents[calEventPos].startdate.month = editedEvent.startdate.month;
    calevents[calEventPos].startdate.year = editedEvent.startdate.year;
    calevents[calEventPos].enddate.day = editedEvent.enddate.day;
    calevents[calEventPos].enddate.month = editedEvent.enddate.month;
    calevents[calEventPos].enddate.year = editedEvent.enddate.year;
    calevents[calEventPos].dayofweek = editedEvent.dayofweek;
    calevents[calEventPos].repeat = editedEvent.repeat;
    calevents[calEventPos].timed = editedEvent.timed;
    calevents[calEventPos].starttime.hour = editedEvent.starttime.hour;
    calevents[calEventPos].starttime.minute = editedEvent.starttime.minute;
    calevents[calEventPos].starttime.second = editedEvent.starttime.second;
    calevents[calEventPos].endtime.hour = editedEvent.endtime.hour;
    calevents[calEventPos].endtime.minute = editedEvent.endtime.minute;
    calevents[calEventPos].endtime.second = editedEvent.endtime.second;
    strcpy((char*)calevents[calEventPos].title, (char*)editedEvent.title);
    strcpy((char*)calevents[calEventPos].location, (char*)editedEvent.location);
    strcpy((char*)calevents[calEventPos].description, (char*)editedEvent.description);    
    
    
    const int numevents = numarrayitems; //put this in a const int so it isn't polluted when there are lots of calendar events/very big events.
    // GetSMEMEvents closed the file, so we can now delete it
    PrintXY(1,8,(char*)"  Please wait..         ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    Bfile_DeleteEntry(pFile);
    PrintXY(1,8,(char*)"  Please wait...        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    //convert the calevents back to char
    unsigned char eventbuf[2048] = ""; 
    unsigned char newfilecontents[MAX_EVENT_FILESIZE];
    strcpy((char*)newfilecontents, (char*)FILE_HEADER); //we need to initialize the char, take the opportunity to add the file header
    for(int j = 0; j < numevents; j++) {
      strcpy((char*)eventbuf, "");
      calEventToChar(eventbuf, calevents[j]);
      strcat((char*)newfilecontents,(char*)eventbuf);
    }
    PrintXY(1,8,(char*)"  Please wait....       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    // now recreate file with new size and write contents to it.
    int newsize = strlen((char*)newfilecontents);
    
    if(newsize < 10) { //too small for a single event == data corruption
      return 50;
    }
    int nBCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &newsize);
    PrintXY(1,8,(char*)"  Please wait.....      ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    if(nBCEres >= 0) // Did it create?
    {
      int nFile = Bfile_OpenFile_OS(pFile, READWRITE);
      Bfile_WriteFile_OS(nFile, newfilecontents, newsize);
      Bfile_CloseFile_OS(nFile);
      PrintXY(1,8,(char*)"  Please wait......     ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      Bdisp_PutDisp_DD();
    } else {
      return 3;
    }

  }
  return 0;
  
  }
}

int RemoveSMEMDay(EventDate date, const char* folder) {
  //remove all SMEM events for the day
  char filename[128] = "";
  char buffer[8] = "";
  strcpy(filename, "\\\\fls0\\");
  strcat(filename, folder);
  strcat(filename, "\\");
  filenameFromDate(date, buffer);
  strcat(filename, buffer);
  strcat(filename, ".pce\0"); //filenameFromDate does not include file extension, so add it
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 

  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle to check if exists
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

int GetSMEMeventsForDate(EventDate startdate, const char* folder, CalendarEvent calEvents[]) {
/*reads the storage memory searching for events starting on specified date.
  folder is where events will be searched for (useful for multiple calendar support)
  if calEvents is not NULL:
  puts events in the array passed as calEvents, which will be empty on error (no events for the day).
  if calEvents is NULL:
  does not parse the inner of events, only parses to count them.
  returns number of events found for the specified day, 0 if error or no events. */
  // Generate filename from given date
  char filename[128] = "";
  char buffer[8] = "";
  strcpy(filename, "\\\\fls0\\");
  strcat(filename, folder);
  strcat(filename, "\\");
  filenameFromDate(startdate, buffer);
  strcat(filename, buffer);
  strcat(filename, ".pce"); //filenameFromDate does not include file extension, so add it
  
  unsigned short pFile[256];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle

  // Check for file existence
  if(hFile >= 0) // Check if it opened
  {
    // Returned no error, file exists, open it
    int size = Bfile_GetFileSize_OS(hFile, Bfile_TellFile_OS( hFile ));
    // File exists and has size 'size'
    // Read file into a buffer which is then parsed and broke in multiple event strings.
    // These event strings are then turned into a CalendarEvent.
    if ((unsigned int)size > MAX_EVENT_FILESIZE) { Bfile_CloseFile_OS(hFile); return -1; } //file too big, return error.
    unsigned char asrc[MAX_EVENT_FILESIZE] = "";
    unsigned char* src = asrc;
    Bfile_ReadFile_OS(hFile, src, size, 0);
    Bfile_CloseFile_OS(hFile); //we got file contents, close it
    // Parse for events
    int curevent = 0; //current event number/array index (zero based)
    unsigned char token[2048];
    src = toksplit(src, EVENT_SEPARATOR , token, 2048);

    int notfinished = 1;
    while (notfinished) {
      //pass event to the parser and store it in the string event array
      if(calEvents != NULL) calEvents[curevent] = charToCalEvent(curevent==0? token+strlen(FILE_HEADER) : token); //convert to a calendar event. if is first event on file, it comes with a header that needs to be skipped.
      curevent++;
      if (strlen((char*)src) < 5) { //5 bytes is not enough space to hold an event, so that means there are no more events to process... right?
        notfinished = 0;
        break; //force it to stop. strtok can't happen again, otherwise token will be null and a system error is approaching!
      }
      if (curevent > MAX_DAY_EVENTS) { //check if we aren't going over the limit
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

/* Some insights on storing events on the main memory: (NOTE: currently something not needed, probably never will be)
  I don't think each add-in should use more than one or two folders in the main memory.
  The calendar add-in could use one folder per each calendar (if we ever support multiple calendars)
  If subfolders were allowed in MCS, we could store events based on their startdate. There'd be one folder for 2012 events, another inside it for August events and yet another inside the August one for events started on the 16th. The file name would then contain the starttime. This would make accessing events very easy, but unfortunately, it's not possible.
  We still need to split events into different files, because with MCS you read and write files all at once (no progressive reading/writing). But, each file can only be 8 chars in length!
  The solution is to store multiple events on a single file, and store all the events starting in the same day to the same file.
  That file would have a name with the format: YYYYMMDD, where YYYY is the year, MM the month with two chars (August would be 08, not 8) and DD the day with two chars (02 instead of 2).
  Again, this only looks at the startdate of the file. This is great, because all events have a startdate no matter for how long they extend, they repeat, or if they are full-day. (Holidays may be a problem, develop a special calendar format for them, since they are read-only?)

*/
void filenameFromDate(EventDate date, char* filename) {
  char smallbuf[8] = "";
  itoa(date.year, (unsigned char*)smallbuf); strcat(filename, smallbuf);
  itoa(date.month, (unsigned char*)smallbuf);
  if (date.month < 10) strcat(filename, "0");  //if month below 10, add leading 0
  strcat(filename, smallbuf);
  itoa(date.day, (unsigned char*)smallbuf);
  if (date.day < 10) strcat(filename, "0"); //if day below 10, add leading 0
  strcat(filename, smallbuf);
}