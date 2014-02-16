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

void bubbleSortFileMenuArray(File* data, MenuItem* mdata, int size) {
  int sort = GetSetting(SETTING_FILE_MANAGER_SORT);
  if(!sort) return;
  int j;
  File temp;
  MenuItem mtemp;
  for(int i = 1; i < size; i++) {
    j = i - 1;
    while( j >= 0 &&
      (sort == 1 ? strcmp( data[j+1].filename, data[j].filename ) < 0 :
        (sort == 2 ? strcmp( data[j+1].filename, data[j].filename ) > 0 :
          (sort == 3 ? data[j+1].size < data[j].size :
            data[j+1].size > data[j].size)))
      ) {
      temp =  data[j + 1];
      data[j+1] = data[j];
      data[j] = temp;
      mtemp =  mdata[j + 1];
      mdata[j+1] = mdata[j];
      mdata[j] = mtemp;
      j--;
    }
  }
}

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
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0
      || strcmp((char*)buffer, SELFFILE) == 0
      || strcmp((char*)buffer, CALENDARFOLDER_NAME) == 0))
    {
      if(files != NULL) {
        strncpy(menuitems[*count].text, (char*)buffer, 40);
        strcpy(files[*count].filename, basepath); 
        strcat(files[*count].filename, (char*)buffer);
        files[*count].size = fileinfo.fsize;
        if(fileinfo.fsize == 0) {
          files[*count].isfolder = 1;
          menuitems[*count].isfolder = 1;
          menuitems[*count].icon = FILE_ICON_FOLDER; // it would be a folder icon anyway, because isfolder is true
        } else {
          files[*count].isfolder = 0;
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
      Bfile_FindClose(findhandle);
      if(files != NULL && menuitems != NULL) bubbleSortFileMenuArray(files, menuitems, *count);
      return GETFILES_MAX_FILES_REACHED; // Don't find more files, the array is full. 
    } else ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  if(*count > 1 && files != NULL && menuitems != NULL) bubbleSortFileMenuArray(files, menuitems, *count);
  return GETFILES_SUCCESS;
}

char* SearchStringMatch(char* s1, char* s2, int matchCase) {
  if(matchCase) return strstr(s1, s2);
  else return strcasestr(s1, s2);
}
int SearchForFiles(File* files, char* basepath, char* needle, int searchOnFilename, int searchOnContents, int searchRecursively, int matchCase, int* count) {
  // searches storage memory for folders and files containing needle in the filename or contents, puts their count in int* count
  // if File* files is NULL, function will only count search results.
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
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0
      || strcmp((char*)buffer, SELFFILE) == 0
      || strcmp((char*)buffer, CALENDARFOLDER_NAME) == 0))
    {
      int match = 0;
      if(searchOnFilename && NULL != SearchStringMatch((char*)buffer, needle, matchCase)) {
        match = 1;
      }
      if(searchOnContents && !match) {
        if(fileinfo.fsize == 0) {
          //it's a folder
        } else {
          char filename[MAX_FILENAME_SIZE];
          strcpy(filename, basepath);
          strcat(filename, (char*)buffer);
          unsigned short pFile[MAX_FILENAME_SIZE+1];
          Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
          int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
          if(hFile >= 0) // Check if it opened
          { //opened
            unsigned char buf[1030] = ""; // initialize to zeros, and make sure it is a bit bigger than the amount
            // of bytes we're going to read, so that the string is always null-terminated and can be safely
            // passed to the string compare function.
            int readsize = 0;
            while(1) {
              readsize = Bfile_ReadFile_OS(hFile, buf, 1024, -1);
              if(!readsize) break;
              if(NULL != SearchStringMatch((char*)buf, needle, matchCase)) {
                match = 1;
                break;
              }
            }
            Bfile_CloseFile_OS(hFile);
          }
        }
      }
      if(match) {
        if(files != NULL) {
          strcpy(files[*count].filename, basepath); 
          strcat(files[*count].filename, (char*)buffer);
          files[*count].size = fileinfo.fsize;
          if(fileinfo.fsize == 0) {
            files[*count].isfolder = 1;
            
          } else {
            files[*count].isfolder = 0;
            
          }
        }
        *count=*count+1;
      }
    }
    if (*count-1==MAX_ITEMS_IN_DIR) {
      Bfile_FindClose(findhandle);
      return GETFILES_MAX_FILES_REACHED; // Don't find more files, the array is full. 
    } else ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
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
    closeProgressMessage();
  }
}

void nameFromFilename(char* filename, char* name, int max) {
  //this function takes a full filename like \\fls0\Folder\file.123
  //and puts file.123 in name.
  int i=strlen(filename)-1;
  while (i>=0 && filename[i] != '\\') i--;
  strncpy(name, filename+i+1, max);
}

void copyFile(char* oldfilename, char* newfilename) {
  if(!strcmp(newfilename, oldfilename) || stringEndsInG3A(newfilename)) {
    //trying to copy over the original file, or this is a g3a file (which we can't "touch")
    return;
  }
  unsigned short newfilenameshort[0x10A];
  unsigned short oldfilenameshort[0x10A];
  unsigned short tempfilenameshort[0x10A];
  Bfile_StrToName_ncpy(oldfilenameshort, (unsigned char*)oldfilename, 0x10A);
  Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
  Bfile_StrToName_ncpy(tempfilenameshort, (unsigned char*)"\\\\fls0\\UTILSTMP.PCT", 0x10A);
  
  int copySize;
  int hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0); // Get handle for the old file
  if(hOldFile < 0) {
    //returned error: couldn't open file to copy.
    return; //skip this file
  } else {
    copySize = Bfile_GetFileSize_OS(hOldFile);
    Bfile_CloseFile_OS(hOldFile); // close for now
  }

  int hNewFile = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0); // Get handle for the destination file. This should fail because the file shouldn't exist.
  if(hNewFile < 0) {
    // Returned error, dest file does not exist (which is good)
  } else {
    // dest file exists (which is bad) and is open.
    Bfile_CloseFile_OS(hNewFile);
    return; //skip this file
  }
  
  // at this point we know that:
  // source file exists
  // destination file doesn't exist
  // source file is closed so CreateEntry can proceed.
  // create a temp file in root because copying into subdirectories directly fails with sys error
  // so we create a temp file in root, and rename in the end
  int BCEres;
  BCEres = Bfile_CreateEntry_OS(tempfilenameshort, CREATEMODE_FILE, &copySize);
  if(BCEres >= 0) // Did it create?
  {
    //created. open newly-created destination file
    hNewFile = Bfile_OpenFile_OS(tempfilenameshort, READWRITE, 0);
    if(hNewFile < 0) // Still failing after the file was created?
    {
      //skip this copy.
      return;
    }
    
    // open old file again
    hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0);
    if(hOldFile < 0) // Opening origin file didn't fail before, but is failing now?!
    {
      //skip this copy.
      Bfile_CloseFile_OS(hNewFile); // close the new file we had opened
      return;
    }
    
    //File to copy is open, destination file is created and open.
    while(1) {
#define FILE_COPYBUFFER 24576
      unsigned char copybuffer[FILE_COPYBUFFER+5] = "";
      int rwsize = Bfile_ReadFile_OS( hOldFile, copybuffer, FILE_COPYBUFFER, -1 );
      if(rwsize > 0) {
        Bfile_WriteFile_OS(hNewFile, copybuffer, rwsize);
      } else {
        break;
      }
    }
    //done copying, close files.
    Bfile_CloseFile_OS(hOldFile);
    Bfile_CloseFile_OS(hNewFile);
    // now rename the temp file to the correct file name
    Bfile_RenameEntry(tempfilenameshort , newfilenameshort);
  } //else: create failed, but we're going to skip anyway
}
#define MAX_ITEMS_PER_FOLDER_COPY 50
void copyFolder(char* oldfilename, char* newfilename) {
  // create destination folder:
  unsigned short newfilenameshort[0x10A];
  Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
  if(0 < Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FOLDER, 0)) return; //return if error

  // now that we created the new folder, copy each item in the old folder into the new one.
  int numberOfItemsToCopyInTheEnd = 0;
  char itemsToCopyInTheEnd[MAX_ITEMS_PER_FOLDER_COPY][MAX_NAME_SIZE];
  int itemsToCopyIsFolder[MAX_ITEMS_PER_FOLDER_COPY];

  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  unsigned char buffer[MAX_FILENAME_SIZE+1];

  // make the buffer
  strcpy((char*)buffer, oldfilename);
  strcat((char*)buffer, "\\*");
  
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 )) {
      char olditem[MAX_FILENAME_SIZE];
      strcpy(olditem, oldfilename);
      strcat(olditem, "\\");
      strcat(olditem, (char*)buffer);
      char newitem[MAX_FILENAME_SIZE];
      strcpy(newitem, newfilename);
      strcat(newitem, "\\");
      strcat(newitem, (char*)buffer);
      // this item is a folder. We could call copyFolder again or copyFile here, and our code would be absolutely correct
      // but the Bfile functions are buggy, and they freak out when there are many file handles/multiple file handles inside folders.
      // (same reason why we use a temporary file name while copying)
      // so we add this to a list of items to copy later...
      // only 20 folders can be copied per folder
      if(numberOfItemsToCopyInTheEnd<MAX_ITEMS_PER_FOLDER_COPY) {
        strncpy(itemsToCopyInTheEnd[numberOfItemsToCopyInTheEnd], (char*)buffer, MAX_NAME_SIZE);
        itemsToCopyIsFolder[numberOfItemsToCopyInTheEnd] = !fileinfo.fsize;
        numberOfItemsToCopyInTheEnd++;
      }
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  // now that all handles are be closed, copy the items we found
  for(int i=0; i<numberOfItemsToCopyInTheEnd; i++) {
    // copy only if there's enough free stack
    if((0x881E0000 - (int)GetStackPtr()) < 350000) { // if stack usage is below 350000 bytes...
      char olditem[MAX_FILENAME_SIZE];
      strcpy(olditem, oldfilename);
      strcat(olditem, "\\");
      strcat(olditem, itemsToCopyInTheEnd[i]);
      char newitem[MAX_FILENAME_SIZE];
      strcpy(newitem, newfilename);
      strcat(newitem, "\\");
      strcat(newitem, itemsToCopyInTheEnd[i]);

      if(itemsToCopyIsFolder[i]) copyFolder(olditem, newitem);
      else copyFile(olditem, newitem);
    }
  }
}

void filePasteClipboardItems(File* clipboard, char* browserbasepath, int itemsInClipboard) {
  //this copies or moves to browserbasepath the items in the clipboard.
  //when the action field of a clipboard item is 0, the item will be moved.
  //when the action field is 1, the item will be copied.
  //don't forget to reload the file list after using this
  if (itemsInClipboard>0) {
    int curfile = 0;
    progressMessage((char*)" Pasting...", curfile, itemsInClipboard);
    while(curfile < itemsInClipboard) {
      char name[MAX_NAME_SIZE];
      nameFromFilename(clipboard[curfile].filename, name);
      if(curfile > 0) progressMessage((char*)" Pasting...", curfile, itemsInClipboard);
      char newfilename[MAX_FILENAME_SIZE];
      strncpy(newfilename, browserbasepath, MAX_FILENAME_SIZE);
      unsigned int maxcatlen = MAX_FILENAME_SIZE-strlen(newfilename);
      strncat(newfilename, name, maxcatlen);
      if (clipboard[curfile].action) {
        //copy file
        if(clipboard[curfile].isfolder) copyFolder(clipboard[curfile].filename, newfilename);
        else copyFile(clipboard[curfile].filename, newfilename);
      } else {
        //move file
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

int stringEndsInG3A(char* string) {
  if(EndsIWith(string, (char*)".g3a")) return 1;
  else return 0;
}