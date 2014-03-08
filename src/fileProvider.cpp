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
#include "keyboardProvider.hpp"
#include "debugGUI.hpp"
extern "C" {
#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
}
int compareFileStructs(File* f1, File* f2, int type) {
  if(f1->isfolder < f2->isfolder) return 1;
  else if(f1->isfolder > f2->isfolder) return -1;
  switch(type) {
    case 1:
      return strcmp( f1->filename, f2->filename );
    case 2:
      return -strcmp( f1->filename, f2->filename );
    case 3:
      return f1->size-f2->size;
    case 4:
    default:
      return f2->size-f1->size;
  }
}

void insertSortFileMenuArray(File* data, MenuItem* mdata, int size) {
  int sort = GetSetting(SETTING_FILE_MANAGER_SORT);
  if(!sort) return;
  int i, j;
  File temp;
  MenuItem mtemp;

  for(i = 1; i < size; i++) {
    temp = data[i];
    mtemp = mdata[i];
    for (j = i - 1; j >= 0 && compareFileStructs(&data[j], &temp, sort) > 0; j--) {
      data[j + 1] = data[j];
      mdata[j + 1] = mdata[j];
    }
    data[j + 1] = temp;
    mdata[j + 1] = mtemp;
  }
  // update menu text pointers (these are still pointing to the old text locations):
  for(i = 0; i < size; i++) {
    mdata[i].text = data[i].visname;
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
        strncpy(files[*count].visname, (char*)buffer, 40);
        strcpy(files[*count].filename, basepath); 
        strcat(files[*count].filename, (char*)buffer);
        files[*count].size = fileinfo.fsize;
        files[*count].isfolder = menuitems[*count].isfolder = !fileinfo.fsize;
        if(fileinfo.fsize == 0) menuitems[*count].icon = FILE_ICON_FOLDER; // it would be a folder icon anyway, because isfolder is true
        else menuitems[*count].icon = fileIconFromName((char*)buffer);
        menuitems[*count].isselected = 0; //clear selection. this means selection is cleared when changing directory (doesn't happen with native file manager)
        // because usually alloca is used to declare space for MenuItem*, the space is not cleared. which means we need to explicitly set each field:
        menuitems[*count].text = files[*count].visname;
        menuitems[*count].color=TEXT_COLOR_BLACK;
        menuitems[*count].type=MENUITEM_NORMAL;
        menuitems[*count].value=MENUITEM_VALUE_NONE;
      }
      *count=*count+1;
    }
    if (*count-1==MAX_ITEMS_IN_DIR) {
      Bfile_FindClose(findhandle);
      if(files != NULL && menuitems != NULL) insertSortFileMenuArray(files, menuitems, *count);
      return GETFILES_MAX_FILES_REACHED; // Don't find more files, the array is full. 
    } else ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  if(*count > 1 && files != NULL && menuitems != NULL) insertSortFileMenuArray(files, menuitems, *count);
  return GETFILES_SUCCESS;
}

char* SearchStringMatch(char* s1, char* s2, int matchCase) {
  if(matchCase) return strstr(s1, s2);
  else return strcasestr(s1, s2);
}
int SearchForFiles(File* files, char* basepath, char* needle, int searchOnFilename, int searchOnContents, int searchRecursively, int matchCase, int* count, int isRecursiveCall) {
  // searches storage memory for folders and files containing needle in the filename or contents, puts their count in int* count
  // if File* files is NULL, function will only count search results.
  // this function always returns status codes defined on fileProvider.hpp
  // basepath should start with \\fls0\ and should always have a slash (\) at the end
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  unsigned char buffer[MAX_FILENAME_SIZE+1];

  int numberOfFoldersToSearchInTheEnd = 0;
  char foldersToSearchInTheEnd[MAX_ITEMS_PER_FOLDER_COPY][MAX_NAME_SIZE];

  // make the buffer
  strcpy((char*)buffer, basepath);
  strcat((char*)buffer, "*");
  
  if(!isRecursiveCall) *count = 0;
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
      if(fileinfo.fsize == 0) {
        //it's a folder. add it to the recursion list, if we are searching recursively.
        if(searchRecursively && numberOfFoldersToSearchInTheEnd<MAX_ITEMS_PER_FOLDER_COPY) {
          strncpy(foldersToSearchInTheEnd[numberOfFoldersToSearchInTheEnd], (char*)buffer, MAX_NAME_SIZE);
          numberOfFoldersToSearchInTheEnd++;
        }
      }
      int match = 0;
      if(searchOnFilename && NULL != SearchStringMatch((char*)buffer, needle, matchCase)) {
        match = 1;
      }
      if(searchOnContents && !match) {
        if(fileinfo.fsize != 0) {
          char filename[MAX_FILENAME_SIZE];
          strcpy(filename, basepath);
          strcat(filename, (char*)buffer);
          unsigned short pFile[MAX_FILENAME_SIZE+1];
          Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, MAX_FILENAME_SIZE); 
          int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
          if(hFile >= 0) // Check if it opened
          { //opened
            unsigned char buf[1030] = ""; // initialize to zeros, and make sure it is a bit bigger than the amount
            // of bytes we're going to read, so that the string is always null-terminated and can be safely
            // passed to the string compare function.
            int readsize = 0;
            int nlen = strlen(needle);
            while(1) {
              readsize = Bfile_ReadFile_OS(hFile, buf, 1024, -1);
              if(NULL != memmem((char*)buf, 1024, needle, nlen, matchCase)) {
                match = 1;
                break;
              }
              if(readsize < 100) break;
              Bfile_SeekFile_OS(hFile, Bfile_TellFile_OS(hFile)-90); // rewind 90 bytes, to make sure we didn't miss the needle
              // which may be separated between the end of a read and the start of another. Since the needle has a maximum of
              // 50 bytes, rewinding 90 ensures we don't miss it between reads.
            }
            Bfile_CloseFile_OS(hFile);
          }
        }
      }
      if(match) {
        if(files != NULL) {
          strcpy(files[*count].filename, basepath); 
          strcat(files[*count].filename, (char*)buffer);
          strncpy(files[*count].visname, (char*)buffer, 40);
          files[*count].size = fileinfo.fsize;
          files[*count].isfolder = !fileinfo.fsize;
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
  // now that all handles are closed, look inside the folders we found, if recursive search is enabled
  if((0x881E0000 - (int)GetStackPtr()) < 350000) { // if stack usage is below 350000 bytes...
    if(searchRecursively) for(int i=0; i<numberOfFoldersToSearchInTheEnd; i++) {
      // search only if there's enough free stack
      char newfolder[MAX_FILENAME_SIZE];
      strcpy(newfolder, basepath);
      strcat(newfolder, foldersToSearchInTheEnd[i]);
      strcat(newfolder, "\\");
      if(GETFILES_MAX_FILES_REACHED == SearchForFiles(files, newfolder, needle, searchOnFilename, searchOnContents, searchRecursively, matchCase, count, 1))
        return GETFILES_MAX_FILES_REACHED; // if the files array is full, there's no point in searching again
    }
  }
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
  
  int hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0); // Get handle for the old file
  if(hOldFile < 0) {
    //returned error: couldn't open file to copy.
    return; //skip this file
  }
  int copySize = Bfile_GetFileSize_OS(hOldFile);
  Bfile_CloseFile_OS(hOldFile); // close for now

  int hNewFile = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0); // Get handle for the destination file. This should fail because the file shouldn't exist.
  if(hNewFile >= 0) {
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
      // We could call copyFolder again or copyFile here, and our code would be absolutely correct
      // but the Bfile functions are buggy, and they freak out when there are many file handles/multiple file handles inside folders.
      // (same reason why we use a temporary file name while copying)
      // so we add this to a list of items to copy later...
      // only 100 items can be copied per folder
      if(numberOfItemsToCopyInTheEnd<MAX_ITEMS_PER_FOLDER_COPY) {
        strncpy(itemsToCopyInTheEnd[numberOfItemsToCopyInTheEnd], (char*)buffer, MAX_NAME_SIZE);
        itemsToCopyIsFolder[numberOfItemsToCopyInTheEnd] = !fileinfo.fsize;
        numberOfItemsToCopyInTheEnd++;
      }
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  // now that all handles are closed, copy the items we found
  // copy only if there's enough free stack
  if((0x881E0000 - (int)GetStackPtr()) < 350000) { // if stack usage is below 350000 bytes...
    for(int i=0; i<numberOfItemsToCopyInTheEnd; i++) {
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

// (DE)COMPRESSION CODE - START

static int encoder_sink_read(int writehandle, heatshrink_encoder *hse, uint8_t *data, size_t data_sz, size_t* final_sz) {
  size_t out_sz = 4096;
  uint8_t out_buf[out_sz];
  memset(out_buf, 0, out_sz);
  size_t sink_sz = 0;
  size_t poll_sz = 0;
  HSE_sink_res sres;
  HSE_poll_res pres;
  HSE_finish_res fres;

  size_t sunk = 0;
  do {
    if (data_sz > 0) {
      sres = heatshrink_encoder_sink(hse, &data[sunk], data_sz - sunk, &sink_sz);
      if (sres < 0) { return 1; }
      sunk += sink_sz;
    }
    
    do {
      pres = heatshrink_encoder_poll(hse, out_buf, out_sz, &poll_sz);
      if (pres < 0) { return 1; }
      if(writehandle >= 0) Bfile_WriteFile_OS(writehandle, out_buf, poll_sz);
      else if (final_sz!=NULL) *final_sz = *final_sz+poll_sz;
    } while (pres == HSER_POLL_MORE);
    
    if (poll_sz == 0 && data_sz == 0) {
      fres = heatshrink_encoder_finish(hse);
      if (fres < 0) { return 1; }
      if (fres == HSER_FINISH_DONE) { return 1; }
    }
  } while (sunk < data_sz);
  return 0;
}

static int decoder_sink_read(int writehandle, heatshrink_decoder *hsd, uint8_t *data, size_t data_sz) {
  size_t sink_sz = 0;
  size_t poll_sz = 0;
  size_t out_sz = 4096;
  uint8_t out_buf[out_sz];
  memset(out_buf, 0, out_sz);

  HSD_sink_res sres;
  HSD_poll_res pres;
  HSD_finish_res fres;

  size_t sunk = 0;
  do {
    if (data_sz > 0) {
      sres = heatshrink_decoder_sink(hsd, &data[sunk], data_sz - sunk, &sink_sz);
      if (sres < 0) { return 1; }
      sunk += sink_sz;
    }

    do {
      pres = heatshrink_decoder_poll(hsd, out_buf, out_sz, &poll_sz);
      if (pres < 0) { return 1; }
      Bfile_WriteFile_OS(writehandle, out_buf, poll_sz);
    } while (pres == HSDR_POLL_MORE);
    
    if (data_sz == 0 && poll_sz == 0) {
      fres = heatshrink_decoder_finish(hsd);
      if (fres < 0) { return 1; }
      if (fres == HSDR_FINISH_DONE) { return 1; }
    }
  } while (sunk < data_sz);

  return 0;
}

void compressFile(char* oldfilename, char* newfilename, int action) {
  // with action == 0, compresses. with action == 1, decompresses.
  if(!strcmp(newfilename, oldfilename) || stringEndsInG3A(newfilename)) {
    //trying to overwrite the original file, or this is a g3a file (which we can't "touch")
    return;
  }
  unsigned short newfilenameshort[0x10A];
  unsigned short oldfilenameshort[0x10A];
  unsigned short tempfilenameshort[0x10A];
  Bfile_StrToName_ncpy(oldfilenameshort, (unsigned char*)oldfilename, 0x10A);
  Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
  Bfile_StrToName_ncpy(tempfilenameshort, (unsigned char*)"\\\\fls0\\UTILSTMP.PCT", 0x10A);
  
  int hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0); // Get handle for the old file
  if(hOldFile < 0) return;
  unsigned int origfilesize = 0, lookahead = 0, windowsize = 0;
#define FILE_COMPORIGBUF 24576
  if(!action) {
    origfilesize = Bfile_GetFileSize_OS(hOldFile); // so that we can write to the compressed file header the original size
    // test compression with different settings to find out which is best for this case
    static const unsigned char wsArray[]={14, 14, 12, 14, 8 };
    static const unsigned char laArray[]={8,  3,  3,  5, 4 };
    unsigned int smallestsize = 0;
    for(int i = 0; i < 5; i++) {
      Bfile_SeekFile_OS(hOldFile, 0); //move cursor to beginning
      heatshrink_encoder *hse = heatshrink_encoder_alloc(wsArray[i], laArray[i]);
      if (hse == NULL) { Bfile_CloseFile_OS(hOldFile); return; }
      unsigned int newsize = 0;
      while(1) {
        unsigned char in_buf[FILE_COMPORIGBUF+5] = "";
        unsigned int read_sz = Bfile_ReadFile_OS( hOldFile, in_buf, FILE_COMPORIGBUF, -1 );
        if(encoder_sink_read(-1, hse, in_buf, read_sz, &newsize)) break;
      }
      if(newsize < smallestsize || !lookahead) {
        smallestsize = newsize;
        windowsize = wsArray[i];
        lookahead = laArray[i];
      }
      heatshrink_encoder_free(hse);
    }
    if(smallestsize >= origfilesize) {
      Bfile_CloseFile_OS(hOldFile);
      mMsgBoxPush(5);
      mPrintXY(3, 2, (char*)"Compressing this", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 3, (char*)"file doesn't", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 4, (char*)"yield a smaller", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 5, (char*)"size; aborted.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      PrintXY_2(TEXT_MODE_NORMAL, 1, 6, 2, TEXT_COLOR_BLACK); // press exit message
      closeMsgBox();
      return;
    }
  }
  Bfile_CloseFile_OS(hOldFile); // close for now

  int hNewFile = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0); // Get handle for the destination file. This should fail because the file shouldn't exist.
  if(hNewFile >= 0) {
    // dest file exists (which is bad) and is open.
    Bfile_CloseFile_OS(hNewFile);
    return; //cancel
  }
  
  // at this point we know that:
  // source file exists
  // destination file doesn't exist
  // source file is closed so CreateEntry can proceed.
  // create a temp file in root because copying into subdirectories directly fails with sys error
  // so we create a temp file in root, and rename in the end
  int BCEres, filesize = 1;
  BCEres = Bfile_CreateEntry_OS(tempfilenameshort, CREATEMODE_FILE, &filesize);
  if(BCEres >= 0) // Did it create?
  {
    //created. open newly-created destination file
    hNewFile = Bfile_OpenFile_OS(tempfilenameshort, READWRITE, 0);
    if(hNewFile < 0) return;
    
    // open old file again
    hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0);
    if(hOldFile < 0) // Opening origin file didn't fail before, but is failing now?!
    {
      //cancel
      Bfile_CloseFile_OS(hNewFile); // close the new file we had opened
      return;
    }
    
    //File to copy is open, destination file is created and open.
    if(action) {
      // DECOMPRESS
      // check file header, jumping it at the same time
      unsigned char header[16] = "";
      int chl = strlen((char*)COMPRESSED_FILE_HEADER)+6; // 4 bytes for original filesize and 2 for window size and lookahead
      Bfile_ReadFile_OS( hOldFile, header, chl, -1 );
      if(strncmp((char*)header, (char*)COMPRESSED_FILE_HEADER, chl)) goto cleanexit; // not a compressed file
      windowsize = header[chl-2];
      lookahead = header[chl-1];
      heatshrink_decoder *hsd = heatshrink_decoder_alloc(256, windowsize, lookahead);
      if (hsd == NULL) goto cleanexit;
      HSD_finish_res fres;
      while(1) {
        unsigned char in_buf[FILE_COMPORIGBUF+5] = "";
        unsigned int read_sz = Bfile_ReadFile_OS( hOldFile, in_buf, FILE_COMPORIGBUF, -1 );
        if (read_sz <= 0) {
          fres = heatshrink_decoder_finish(hsd);
          if (fres < 0) { goto cleanexit; }
          if (fres == HSDR_FINISH_DONE) break;
        } else {
            if (decoder_sink_read(hNewFile, hsd, in_buf, read_sz)) break;
        }
      }
      heatshrink_decoder_free(hsd);
    } else {
      // COMPRESS
      // we can't write header directly because source buffers for WriteFile can't be in ROM
      unsigned char header[16] = "";
      strcpy((char*)header, (char*)COMPRESSED_FILE_HEADER);
      // write uncompressed filesize to header
      int len=strlen((char*)COMPRESSED_FILE_HEADER);
      header[len] = (origfilesize >> 24) & 0xFF;
      header[len+1] = (origfilesize >> 16) & 0xFF;
      header[len+2] = (origfilesize >> 8) & 0xFF;
      header[len+3] = origfilesize & 0xFF;
      header[len+4] = windowsize;
      header[len+5] = lookahead;
      Bfile_WriteFile_OS(hNewFile, header, len+6);
      heatshrink_encoder *hse = heatshrink_encoder_alloc(windowsize, lookahead);
      if (hse == NULL) goto cleanexit;
      while(1) {
        unsigned char in_buf[FILE_COMPORIGBUF+5] = "";
        unsigned int read_sz = Bfile_ReadFile_OS( hOldFile, in_buf, FILE_COMPORIGBUF, -1 );
        if(encoder_sink_read(hNewFile, hse, in_buf, read_sz, NULL)) break;
      }
      heatshrink_encoder_free(hse);
    }
    //done copying, close files.
cleanexit:    
    Bfile_CloseFile_OS(hOldFile);
    Bfile_CloseFile_OS(hNewFile);
    // now rename the temp file to the correct file name
    Bfile_RenameEntry(tempfilenameshort , newfilenameshort);

    mMsgBoxPush(5);
    mPrintXY(3, 2, (action? (char*)"Decompression" : (char*)"Compression"), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, (char*)"successful.Delete", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 4, (action? (char*)"compressed file?" : (char*)"original file?"), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 3, TEXT_COLOR_BLACK); // yes, F1
    PrintXY_2(TEXT_MODE_NORMAL, 1, 6, 4, TEXT_COLOR_BLACK); // no, F6
    if(closeMsgBox(1)) {
      Bfile_DeleteEntry(oldfilenameshort);
      return;
    }
  } //else: create failed, return
}

int isFileCompressed(char* filename, int* origfilesize) {
  if(!EndsIWith(filename, (char*)COMPRESSED_FILE_EXTENSION)) return 0;
  unsigned short filenameshort[MAX_FILENAME_SIZE];
  Bfile_StrToName_ncpy(filenameshort, (unsigned char*)filename, MAX_FILENAME_SIZE);
  int hFile = Bfile_OpenFile_OS(filenameshort, READWRITE, 0); // Get handle for the old file
  if(hFile < 0) return 0;
  unsigned char header[14] = "";
  int chl = strlen((char*)COMPRESSED_FILE_HEADER);
  Bfile_ReadFile_OS(hFile, header, chl+4, -1 ); // +4 bytes for the original filesize info
  if(strncmp((char*)header, (char*)COMPRESSED_FILE_HEADER, chl)) {
    Bfile_CloseFile_OS(hFile);
    return 0;
  }
  *origfilesize = (header[chl] << 24) | (header[chl+1] << 16) | (header[chl+2] << 8) | header[chl+3];
  Bfile_CloseFile_OS(hFile);
  return 1;
}