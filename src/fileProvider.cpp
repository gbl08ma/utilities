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
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "stringsProvider.hpp"
#include "menuGUI.hpp"
#include "fileProvider.hpp"
#include "debugGUI.hpp"

int GetAnyFiles(File* files, MenuItem* menuitems, char* basepath, int* count) {
  // searches storage memory for folders and files, puts their count in int* count
  // if File* files is NULL, function will only count files. If it is not null, MenuItem* menuitems will also be updated
  // this function always returns status codes defined on fileProvider.hpp
  // basepath should start with \\fls0\ and should always have a slash (\) at the end
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  unsigned char buffer[MAX_FILENAME_SIZE+1];

  // make the buffer
  strcpy((char*)buffer, basepath);
  strcat((char*)buffer, "*");
  
  *count = 0;
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 || \
      /*strcmp((char*)buffer, "@MainMem") == 0 ||*/ strcmp((char*)buffer, SELFFILE) == 0
      || strcmp((char*)buffer, CALENDARFOLDER) == 0))
    {
      if(files != NULL) {
        strncpy(menuitems[*count].text, (char*)buffer, 40);
        strcpy(files[*count].filename, basepath); 
        strcat(files[*count].filename, (char*)buffer);
        if(fileinfo.fsize == 0) {
          files[*count].isfolder = 1;
          menuitems[*count].isfolder = 1;
          menuitems[*count].icon = FILE_ICON_FOLDER; // it would be a folder icon anyway, because isfolder is true
        } else {
          menuitems[*count].isfolder = 0;
          menuitems[*count].isfolder = 0;
          menuitems[*count].icon = fileIconFromName((char*)buffer);
        }
        menuitems[*count].isselected = 0; //clear selection. this means selection is cleared when changing directory (doesn't happen with native file manager)
        // because usually alloca is used to declare space for MenuItem*, the space is not cleared. which means we need to explicitly set each field:
        menuitems[*count].color=TEXT_COLOR_BLACK;
        menuitems[*count].type=MENUITEM_NORMAL;
        menuitems[*count].value=MENUITEM_VALUE_NONE;
      }
      *count=*count+1;
    }
    if (*count-1==MAX_ITEMS_IN_DIR) { 
      return GETFILES_MAX_FILES_REACHED; // Don't find more files, the array is full. 
    } 
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  return GETFILES_SUCCESS;
}

void deleteFiles(File* files, Menu* menu) {
  //files: the array (list) of files to perform operations in. NOT files to delete (this will only delete selected files)
  //menu: the menu of the current file manager window. used to check which files are selected, total number of files, etc.
  //REFRESH the files array after calling this!
  if (menu->numitems > 0) {
    int curfile = 0; //current processing file (not number of deleted files!)
    int delfiles = 0; // number of deleted files
    unsigned short path[MAX_FILENAME_SIZE+1];
    progressMessage((char*)" Deleting...", 0, menu->numselitems);
    while(curfile < menu->numitems  && delfiles < menu->numselitems) {  
      if (menu->items[curfile].isselected) {
        Bfile_StrToName_ncpy(path, (unsigned char*)files[curfile].filename, MAX_FILENAME_SIZE+1);
        Bfile_DeleteEntry( path );
        delfiles++;
      }
      if(delfiles>0) // do not call progressMessage with 0 as the current value twice, otherwise MsgBox might be pushed twice!
        progressMessage((char*)" Deleting...", delfiles, menu->numselitems);
      curfile++;
    }
  }
  closeProgressMessage();
}

void nameFromFilename(char* filename, char* name) {
  //this function takes a full filename like \\fls0\Folder\file.123
  //and puts file.123 in name.
  strcpy(name, (char*)"");
  int i=strlen(filename)-1;
  while (i>=0 && filename[i] != '\\')
          i--;
  if (filename[i] == '\\') {
    strcpy(name, filename+i+1);
  }
}

void filePasteClipboardItems(File* clipboard, char* browserbasepath, int itemsInClipboard) {
  //this copies or moves to browserbasepath the files in the clipboard.
  //when the isselected field of a clipboard item is 0, the item will be moved.
  //when the isselected field is 1, the item will be copied.
  //don't forget to reload the file list after using this
  if (itemsInClipboard>0) {
    int curfile = 0;
    progressMessage((char*)" Pasting...", curfile, itemsInClipboard);
    while(curfile < itemsInClipboard) {
      char name[MAX_NAME_SIZE] = "";
      nameFromFilename(clipboard[curfile].filename, name);
      if(curfile > 0) progressMessage((char*)" Pasting...", curfile, itemsInClipboard);
      if (clipboard[curfile].action) {
        //copy file
        char newfilename[MAX_FILENAME_SIZE] = "";
        strncpy(newfilename, browserbasepath, MAX_FILENAME_SIZE);
        unsigned int maxcatlen = MAX_FILENAME_SIZE-strlen(newfilename);
        strncat(newfilename, name, maxcatlen);
        if(!strcmp(newfilename, clipboard[curfile].filename)) {
          //a file with that name already exists on the new location.
          curfile++; continue; //skip
        }
        unsigned short newfilenameshort[0x10A];
        unsigned short oldfilenameshort[0x10A];
        Bfile_StrToName_ncpy(oldfilenameshort, (unsigned char*)clipboard[curfile].filename, 0x10A);
        Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
        
        int hOldFile;
        int copySize;
        hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READ, 0); // Get handle for the old file
        if(hOldFile < 0) {
          //returned error: couldn't open file to copy.
          curfile++; continue; //skip this file
        } else {
          copySize = Bfile_GetFileSize_OS(hOldFile);
          Bfile_CloseFile_OS(hOldFile); // close for now
        }
          
        int hNewFile;
        hNewFile = Bfile_OpenFile_OS(newfilenameshort, WRITE, 0); // Get handle for the destination file. This should fail because the file shouldn't exist.
        if(hNewFile < 0) {
          // Returned error, dest file does not exist (which is good)
        } else {
          // dest file exists (which is bad) and is open.
          Bfile_CloseFile_OS(hNewFile);
          curfile++; continue; //skip this file
        }
        
        // at this point we know that:
        // source file exists
        // destination file doesn't exist
        // source file is closed so CreateEntry can proceed.
        int BCEres;
        BCEres = Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FILE, &copySize);
        if(BCEres >= 0) // Did it create?
        {
          //created. open newly-created destination file
          hNewFile = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0);
          if(hNewFile < 0) // Still failing after the file was created?
          {
            //skip this copy.
            curfile++; continue;
          }
          
          // open old file again
          hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0);
          if(hOldFile < 0) // Opening origin file didn't fail before, but is failing now?!
          {
            //skip this copy.
            Bfile_CloseFile_OS(hNewFile); // close the new file we had opened
            curfile++; continue;
          }
          
          //File to copy is open, destination file is created and open.
          long int curpos = 0; // NOTE: for debug
          while(1) {
#define FILE_COPYBUFFER 4096
            unsigned char copybuffer[FILE_COPYBUFFER+5] = "";
            debugMessage((char*)"  in loop", (char*)"  curpos: ", curpos);
            int rwsize = Bfile_ReadFile_OS( hOldFile, copybuffer, FILE_COPYBUFFER, -1 );
            if(rwsize > 0) {
              Bfile_WriteFile_OS(hNewFile, copybuffer, rwsize);
            } else {
              break;
            }
            curpos = curpos + rwsize; // NOTE: now this is for debug only
          }
          //done copying, close files.
          Bfile_CloseFile_OS(hOldFile);
          Bfile_CloseFile_OS(hNewFile);
        } //else: create failed, but we're going to skip anyway
      } else {
        //move file
        char newfilename[MAX_FILENAME_SIZE] = "";
        strncpy(newfilename, browserbasepath, MAX_FILENAME_SIZE);
        unsigned int maxcatlen = MAX_FILENAME_SIZE-strlen(newfilename);
        strncat(newfilename, name, maxcatlen);
        unsigned short newfilenameshort[0x10A];
        unsigned short oldfilenameshort[0x10A];
        Bfile_StrToName_ncpy(oldfilenameshort, (unsigned char*)clipboard[curfile].filename, 0x10A);
        Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
        Bfile_RenameEntry(oldfilenameshort , newfilenameshort);
      }
      curfile++;
    }
    closeProgressMessage(); //we opened it, no matter if copying or moving.
  }
}

int fileIconFromName(char* name) {
  if(EndsIWith(name, (char*)".g1m") || EndsIWith(name, (char*)".g2m") || EndsIWith(name, (char*)".g3m"))
    return FILE_ICON_G3M;
  else if (EndsIWith(name, (char*)".g1e") || EndsIWith(name, (char*)".g2e") || EndsIWith(name, (char*)".g3e"))
    return FILE_ICON_G3E;
  else if (EndsIWith(name, (char*)".g3a") || EndsIWith(name, (char*)".g3l"))
    return FILE_ICON_G3A;
  else if (EndsIWith(name, (char*)".g3p"))
    return FILE_ICON_G3P;
  else if (EndsIWith(name, (char*)".g3b"))
    return FILE_ICON_G3B;
  else if (EndsIWith(name, (char*)".bmp"))
    return FILE_ICON_BMP;
  else if (EndsIWith(name, (char*)".txt"))
    return FILE_ICON_TXT;
  else if (EndsIWith(name, (char*)".csv"))
    return FILE_ICON_CSV;
  else return FILE_ICON_OTHER;
}