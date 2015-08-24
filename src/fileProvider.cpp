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
#include <alloca.h>

#include "constantsProvider.hpp"
#include "graphicsProvider.hpp"
#include "settingsProvider.hpp"
#include "stringsProvider.hpp"
#include "menuGUI.hpp"
#include "fileProvider.hpp"
#include "fileGUI.hpp"
extern "C" {
#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
}
int compareFileStructs(File* f1, File* f2, int type) {
  if(f1->isfolder < f2->isfolder) return 1;
  else if(f1->isfolder > f2->isfolder) return -1;
  int retval;
  char* f1name = f1->filename + f1->visname;
  char* f2name = f2->filename + f2->visname;
  switch(type) {
    case 1:
      retval = strcmp(f1name, f2name);
      break;
    case 2:
      retval = -strcmp(f1name, f2name);
      break;
    case 3:
      retval = f1->size-f2->size;
      break;
    case 4:
      retval = f2->size-f1->size;
      break;
    case 5:
    case 6:
    default:
    {
      char* ptr1 = f1name + strlen(f1name);
      char* ptr2 = f2name + strlen(f2name);
      for(; ptr1 > f1name && *ptr1 != '.'; ptr1--);
      for(; ptr2 > f2name && *ptr2 != '.'; ptr2--);
      retval = (type == 5 ? 1 : -1) * strcmp(ptr1, ptr2);
      break;
    }
  }
  // make A-Z the sub-sorting for items that are otherwise the same,
  // according to the chosen criteria
  if(!retval) return strcmp(f1name, f2name);
  return retval;
}

void sortFiles(File* data, int size) {
  int sort = getSetting(SETTING_FILE_MANAGER_SORT);
  if(!sort) return;
  int i, j;
  File temp;

  for(i = 1; i < size; i++) {
    temp = data[i];
    for (j = i - 1; j >= 0 && compareFileStructs(&data[j], &temp, sort) > 0; j--) {
      data[j + 1] = data[j];
    }
    data[j + 1] = temp;
  }
}

int getFiles(File* files, const char* basepath, int* count) {
  /* searches storage memory for folders and files, puts their count in int* count
   * If File* files is NULL, function will only count files.
   * If it is not null, MenuItem* menuitems will also be updated
   * this function always returns status codes defined on fileProvider.hpp
   * basepath should start with \\fls0\ and should always have a slash (\) at the end
   */
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  char buffer[MAX_FILENAME_SIZE+1];
  size_t baselen = strlen(basepath);

  // make the buffer
  sprintf(buffer, "%s*", basepath);
  
  *count = 0;
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0
      || strcmp((char*)buffer, (char*)SELFFILE) == 0
      || strcmp((char*)buffer, (char*)CALENDARFOLDER_NAME) == 0))
    {
      if(files != NULL) {
        strncpy(files[*count].filename, basepath, MAX_FILENAME_SIZE); 
        strcat(files[*count].filename, (char*)buffer);
        files[*count].visname = baselen;
        files[*count].size = fileinfo.fsize;
        files[*count].isfolder = !fileinfo.fsize;
      }
      *count=*count+1;
    }
    if (*count==MAX_ITEMS_IN_DIR) {
      Bfile_FindClose(findhandle);
      if(files != NULL) sortFiles(files, *count);
      return GETFILES_MAX_FILES_REACHED; // Don't find more files, the array is full. 
    } else ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  if(*count > 1 && files != NULL) sortFiles(files, *count);
  return GETFILES_SUCCESS;
}

char* SearchStringMatch(const char* s1, const char* s2, int matchCase) {
  if(matchCase) return strstr(s1, s2);
  else return strcasestr(s1, s2);
}
int searchForFiles(File* files, const char* basepath, const char* needle, int searchOnFilename,
  int searchOnContents, int searchRecursively, int matchCase, int* count, int isRecursiveCall) {
  /* searches storage memory for folders and files containing needle in the filename or contents,
   * puts their count in int* count
   * if File* files is NULL, function will only count search results.
   * this function always returns status codes defined on fileProvider.hpp
   * basepath should start with \\fls0\ and should always have a slash (\) at the end
   */
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  char buffer[MAX_FILENAME_SIZE+1];
  size_t baselen = strlen(basepath);
  int nlen = strlen(needle);

  int numberOfFoldersToSearchInTheEnd = 0;
  char foldersToSearchInTheEnd[MAX_ITEMS_PER_FOLDER_COPY][MAX_NAME_SIZE];

  // make the buffer
  sprintf(buffer, "%s*", basepath);
  
  if(!isRecursiveCall) *count = 0;
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  int abortkey = 0;
  while(!ret && abortkey != KEY_PRGM_ACON) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0
      || strcmp((char*)buffer, (char*)SELFFILE) == 0
      || strcmp((char*)buffer, (char*)CALENDARFOLDER_NAME) == 0))
    {
      if(fileinfo.fsize == 0) {
        //it's a folder. add it to the recursion list, if we are searching recursively.
        if(searchRecursively && numberOfFoldersToSearchInTheEnd<MAX_ITEMS_PER_FOLDER_COPY) {
          strncpy(foldersToSearchInTheEnd[numberOfFoldersToSearchInTheEnd], (char*)buffer,
                  MAX_NAME_SIZE);
          numberOfFoldersToSearchInTheEnd++;
        }
      }
      int match = 0;
      if(searchOnFilename && NULL != SearchStringMatch(buffer, needle, matchCase)) {
        match = 1;
      }
      if(searchOnContents && !match) {
        if(fileinfo.fsize != 0) {
          char filename[MAX_FILENAME_SIZE];
          strcpy(filename, basepath);
          strcat(filename, (char*)buffer);
          unsigned short pFile[MAX_FILENAME_SIZE+1];
          Bfile_StrToName_ncpy(pFile, filename, MAX_FILENAME_SIZE); 
          int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
          if(hFile >= 0) // Check if it opened
          { //opened
            unsigned char buf[1030] = ""; // initialize to zeros, and make sure it is a bit bigger
            // than the amount of bytes we're going to read, so that the string is always
            // null-terminated and can be safely passed to the string compare function.
            while(1) {
              int readsize = Bfile_ReadFile_OS(hFile, buf, 1024, -1);
              if(NULL != memmem((char*)buf, readsize, needle, nlen, matchCase)) {
                match = 1;
                break;
              }
              if(readsize < 100) break;
              Bfile_SeekFile_OS(hFile, Bfile_TellFile_OS(hFile)-50); // rewind 50 bytes, to make
              // sure we didn't miss the needle (max. 50 bytes) which may be located between the end
              // of a read section and the start of another.
            }
            Bfile_CloseFile_OS(hFile);
          }
        }
      }
      if(match) {
        if(files != NULL) {
          strncpy(files[*count].filename, basepath, MAX_FILENAME_SIZE); 
          strcat(files[*count].filename, (char*)buffer);
          files[*count].visname = baselen;
          files[*count].size = fileinfo.fsize;
          files[*count].isfolder = !fileinfo.fsize;
        }
        *count=*count+1;
      }
      abortkey = PRGM_GetKey();
    }
    if (*count==MAX_ITEMS_IN_DIR) {
      Bfile_FindClose(findhandle);
      return GETFILES_MAX_FILES_REACHED; // Don't find more files, the array is full. 
    } else ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }
  Bfile_FindClose(findhandle);
  if(abortkey == KEY_PRGM_ACON) return GETFILES_USER_ABORTED;
  // now that all handles are closed, take care of the recursive search, if it was requested
  if((0x881E0000 - (int)GetStackPtr()) < 350000) { // if stack usage is below 350000 bytes...
    if(searchRecursively) for(int i=0; i<numberOfFoldersToSearchInTheEnd; i++) {
      // search only if there's enough free stack
      char newfolder[MAX_FILENAME_SIZE];
      strcpy(newfolder, basepath);
      strcat(newfolder, foldersToSearchInTheEnd[i]);
      strcat(newfolder, "\\");
      int sres = searchForFiles(files, newfolder, needle, searchOnFilename, searchOnContents,
                                searchRecursively, matchCase, count, 1);
      if(GETFILES_MAX_FILES_REACHED == sres)
        // if the files array is full, there's no point in searching again
        return GETFILES_MAX_FILES_REACHED;
      else if(GETFILES_USER_ABORTED == sres)
        return GETFILES_USER_ABORTED;
    }
  }
  return GETFILES_SUCCESS;
}

void deleteFiles(File* files, Menu* menu) {
  /* files: the array (list) of files to perform operations in.
   * NOT the files to delete (this will only delete selected files)
   * menu: the menu of the current file manager window. used to check which files are selected,
   * total number of files, etc.
   * REFRESH the files array after calling this!
   */
  if (menu->numitems > 0) {
    int curfile = 0; //current processing file (not number of deleted files!)
    int delfiles = 0; // number of deleted files
    unsigned short path[MAX_FILENAME_SIZE+1];
    progressMessage((char*)" Deleting...", 0, menu->numselitems);
    while(curfile < menu->numitems  && delfiles < menu->numselitems) {  
      if (menu->items[curfile].isselected) {
        Bfile_StrToName_ncpy(path, files[curfile].filename, MAX_FILENAME_SIZE+1);
        Bfile_DeleteEntry( path );
        delfiles++;
      }
      if(delfiles>0) // do not call progressMessage with 0 as the current value twice,
                     // otherwise MsgBox might be pushed twice!
        progressMessage((char*)" Deleting...", delfiles, menu->numselitems);
      curfile++;
    }
    closeProgressMessage();
  }
}

void renameFile(char* old, char* newf) {
  unsigned short orig[MAX_FILENAME_SIZE+1];
  unsigned short dest[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(orig, old, MAX_FILENAME_SIZE+1);
  Bfile_StrToName_ncpy(dest, newf, MAX_FILENAME_SIZE+1);
  if(Bfile_RenameEntry(orig, dest) < 0 && overwriteFilePrompt(newf)) {
    Bfile_DeleteEntry(dest);
    Bfile_RenameEntry(orig, dest);
  }
}

char* filenameToName(char* filename) {
  //this function takes a full filename like \\fls0\Folder\file.123
  //and returns a pointer to file.123.
  char* name = filename;
  while(*filename++)
    if(*filename == '\\') name = filename+1;
  return name;
}

void copyFile(char* oldfilename, char* newfilename) {
  if(!strcmp(newfilename, oldfilename) || stringEndsInG3A(newfilename)) {
    //trying to copy over the original file, or this is a g3a file (which we can't "touch")
    return;
  }
  unsigned short newfilenameshort[0x10A];
  unsigned short oldfilenameshort[0x10A];
  unsigned short tempfilenameshort[0x10A];
  Bfile_StrToName_ncpy(oldfilenameshort, oldfilename, 0x10A);
  Bfile_StrToName_ncpy(newfilenameshort, newfilename, 0x10A);
  Bfile_StrToName_ncpy(tempfilenameshort, TEMPFILE, 0x10A);
  
  int hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0); // Get handle for the old file
  if(hOldFile < 0) {
    //returned error: couldn't open file to copy.
    return; //skip this file
  }
  size_t copySize = Bfile_GetFileSize_OS(hOldFile);
  Bfile_CloseFile_OS(hOldFile); // close for now

  // Get handle for the destination file, just to check for its presence
  int hNewFile = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0);
  if(hNewFile >= 0) {
    Bfile_CloseFile_OS(hNewFile);
    // destination exists, show overwrite confirmation
    if(overwriteFilePrompt(newfilename))
      Bfile_DeleteEntry(newfilenameshort);
    else return; // skip this file
  }
  
  // at this point we know that:
  // destination file doesn't exist
  // source file is closed.
  // create a temp file in root because copying into subdirectories directly fails with sys error
  // so we create a temp file in root, and rename in the end
  if(Bfile_CreateEntry_OS(tempfilenameshort, CREATEMODE_FILE, &copySize) >= 0) // Did it create?
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
    if(hOldFile < 0) { // Opening origin file didn't fail before, but is failing now?!
      //skip this copy, after closing and deleting the temp file
      Bfile_CloseFile_OS(hNewFile);
      Bfile_DeleteEntry(tempfilenameshort);
      return;
    }
    
    //File to copy is open, destination file is created and open.
#define FILE_COPYBUFFER 24576
    int rwsize;
    do {
      unsigned char copybuffer[FILE_COPYBUFFER+5] = "";
      rwsize = Bfile_ReadFile_OS(hOldFile, copybuffer, FILE_COPYBUFFER, -1);
      if(rwsize > 0) Bfile_WriteFile_OS(hNewFile, copybuffer, rwsize);
    } while(rwsize > 0);
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
  Bfile_StrToName_ncpy(newfilenameshort, newfilename, 0x10A);
  if(0 < Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FOLDER, 0)) return; //return if error

  // now that we created the new folder, copy each item in the old folder into the new one.
  int numberOfItemsToCopyInTheEnd = 0;
  char itemsToCopyInTheEnd[MAX_ITEMS_PER_FOLDER_COPY][MAX_NAME_SIZE];
  int itemsToCopyIsFolder[MAX_ITEMS_PER_FOLDER_COPY];

  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  char buffer[MAX_FILENAME_SIZE+1];

  // make the buffer
  strcpy(buffer, oldfilename);
  strcat(buffer, "\\*");
  
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
      /* We could call copyFolder or copyFile here, and the logic would be absolutely correct.
       * However, the Bfile functions are buggy, and they freak out when there are multiple file
       * handles inside folders.
       * (same reason why we use a temporary file name while copying)
       * See http://prizm.cemetech.net/index.php/Category:Syscalls:Bfile
       * So we add this item to a list of items to copy later...
       * only 100 items can be copied per folder:
       */
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

void pasteClipboard(File* clipboard, char* browserbasepath, int itemsInClipboard) {
  //this copies or moves to browserbasepath the items in the clipboard.
  //when the action field of a clipboard item is 0, the item will be moved.
  //when the action field is 1, the item will be copied.
  //don't forget to reload the file list after using this
  unsigned int maxcatlen = MAX_FILENAME_SIZE-strlen(browserbasepath);
  if (itemsInClipboard>0) {
    int curfile = 0;
    progressMessage((char*)" Pasting...", curfile, itemsInClipboard);
    while(curfile < itemsInClipboard) {
      if(curfile > 0) progressMessage((char*)" Pasting...", curfile, itemsInClipboard);
      char newfilename[MAX_FILENAME_SIZE];
      strncpy(newfilename, browserbasepath, MAX_FILENAME_SIZE);
      strncat(newfilename, clipboard[curfile].filename + clipboard[curfile].visname, maxcatlen);
      if (clipboard[curfile].action) {
        //copy file
        if(clipboard[curfile].isfolder) copyFolder(clipboard[curfile].filename, newfilename);
        else copyFile(clipboard[curfile].filename, newfilename);
      } else {
        //move file
        renameFile(clipboard[curfile].filename, newfilename);
      }
      curfile++;
    }
    closeProgressMessage(); //we opened it, no matter if copying or moving.
  }
}

int filenameToIcon(char* name) {
  if(strEndsWith(name, (char*)".g1m") ||
     strEndsWith(name, (char*)".g2m") ||
     strEndsWith(name, (char*)".g3m"))
    return FILE_ICON_G3M;
  else if (strEndsWith(name, (char*)".g1e") ||
           strEndsWith(name, (char*)".g2e") ||
           strEndsWith(name, (char*)".g3e"))
    return FILE_ICON_G3E;
  else if (strEndsWith(name, (char*)".g3a") ||
           strEndsWith(name, (char*)".g3l"))
    return FILE_ICON_G3A;
  else if (strEndsWith(name, (char*)".g3p"))
    return FILE_ICON_G3P;
  else if (strEndsWith(name, (char*)".g3b"))
    return FILE_ICON_G3B;
  else if (strEndsWith(name, (char*)".bmp") ||
           strEndsWith(name, (char*)".jpg"))
    return FILE_ICON_BMP;
  else if (strEndsWith(name, (char*)".txt"))
    return FILE_ICON_TXT;
  else if (strEndsWith(name, (char*)".csv"))
    return FILE_ICON_CSV;
  else return FILE_ICON_OTHER;
}

int stringEndsInG3A(char* string) {
  return strEndsWith(string, (char*)".g3a");
}

int stringEndsInJPG(char* string) {
  return strEndsWith(string, (char*)".jpg") || strEndsWith(string, (char*)".jpeg");
}

void createFolderRecursive(const char* folder) {
  // creates folder \\fls0\Fol1\Abc even if \\fls0\Fol1 doesn't exist yet
  // despite the name, this is not a recursive function.
  int l = strlen(folder);
  int s = sizeof(SMEM_PREFIX)-1;
  while(1) {
    char nFolder[MAX_FILENAME_SIZE];
    unsigned short pFolder[MAX_FILENAME_SIZE];
    int end = l;
    for(int i = s; i < l; i++) {
      if(folder[i] == '\\') {
        end = i;
        break;
      }
    }
    s = end + 1;
    strncpy(nFolder, folder, end);
    nFolder[end] = 0;
    Bfile_StrToName_ncpy(pFolder, nFolder, MAX_FILENAME_SIZE);
    Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0);
    if(end == l) break;
  }
}

// (DE)COMPRESSION CODE - START

static int encoder_sink_read(int writehandle, heatshrink_encoder *hse, uint8_t *data,
                             size_t data_sz, size_t* final_sz) {
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

static int decoder_sink_read(int writehandle, heatshrink_decoder *hsd, uint8_t *data,
                             size_t data_sz) {
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

void compressFile(char* oldfilename, char* newfilename, int action, int silent) {
  // with action == 0, compresses. with action == 1, decompresses.
  // if silent is true, GUI is not touched.
  if(!strcmp(newfilename, oldfilename) ||
      stringEndsInG3A(newfilename) ||
      stringEndsInG3A(oldfilename)) {
    //trying to overwrite the original file, or this is a g3a file (which we can't "touch")
    return;
  }
  unsigned short newfilenameshort[0x10A];
  unsigned short oldfilenameshort[0x10A];
  unsigned short tempfilenameshort[0x10A];
  Bfile_StrToName_ncpy(oldfilenameshort, oldfilename, 0x10A);
  Bfile_StrToName_ncpy(newfilenameshort, newfilename, 0x10A);
  Bfile_StrToName_ncpy(tempfilenameshort, TEMPFILE, 0x10A);
  
  int hOldFile = Bfile_OpenFile_OS(oldfilenameshort, READWRITE, 0); // Get handle for the old file
  if(hOldFile < 0) return;
  unsigned int origfilesize = 0, lookahead = 0, windowsize = 0;
#define FILE_COMPORIGBUF 24576
  if(!action) {
    origfilesize = Bfile_GetFileSize_OS(hOldFile);
    // so that we can write the original size to the compressed file header

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
      if(!lookahead || newsize < smallestsize) {
        smallestsize = newsize;
        windowsize = wsArray[i];
        lookahead = laArray[i];
      }
      heatshrink_encoder_free(hse);
    }
    if(smallestsize >= origfilesize) {
      Bfile_CloseFile_OS(hOldFile);
      if(!silent) {
        mMsgBoxPush(5);
        multiPrintXY(3, 2, "Compressing this\nfile doesn't\nyield a smaller\nsize; aborted.",
                           TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        closeMsgBox(0, 6);
      }
      return;
    }
  }
  Bfile_CloseFile_OS(hOldFile); // close for now

  // Get handle for the destination file. This should fail because the file shouldn't exist.
  int hNewFile = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0);
  if(hNewFile >= 0) {
    // dest file exists (which is bad) and is open.
    Bfile_CloseFile_OS(hNewFile);
    if(overwriteFilePrompt(newfilename)) {
      Bfile_DeleteEntry(newfilenameshort);
    } else return; //cancel
  }
  
  // at this point we know that:
  // source file exists
  // destination file doesn't exist
  // source file is closed so CreateEntry can proceed.
  // create a temp file in root because copying into subdirectories directly fails with sys error
  // so we create a temp file in root, and rename in the end
  int BCEres;
  size_t filesize = 1;
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
      // 6: 4 bytes for original filesize and 2 for window size and lookahead
      int chl = sizeof(COMPRESSED_FILE_HEADER)-1+6;
      Bfile_ReadFile_OS( hOldFile, header, chl, -1 );
      if(strncmp((char*)header, (char*)COMPRESSED_FILE_HEADER, chl))
        goto cleanexit; // not a compressed file

      windowsize = header[chl-2];
      lookahead = header[chl-1];
      heatshrink_decoder *hsd = heatshrink_decoder_alloc(256, windowsize, lookahead);
      if (hsd == NULL) goto cleanexit;
      HSD_finish_res fres;
      while(1) {
        unsigned char in_buf[FILE_COMPORIGBUF+5];
        unsigned int read_sz = Bfile_ReadFile_OS( hOldFile, in_buf, FILE_COMPORIGBUF, -1 );
        if (!read_sz) {
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
      unsigned char header[16];
      strcpy((char*)header, (char*)COMPRESSED_FILE_HEADER);
      // write uncompressed filesize to header
      int len=sizeof(COMPRESSED_FILE_HEADER)-1;
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
        unsigned char in_buf[FILE_COMPORIGBUF+5];
        unsigned int read_sz = Bfile_ReadFile_OS( hOldFile, in_buf, FILE_COMPORIGBUF, -1 );
        if(encoder_sink_read(hNewFile, hse, in_buf, read_sz, NULL)) break;
      }
      heatshrink_encoder_free(hse);
    }
    //done compressing, close files.
cleanexit:    
    Bfile_CloseFile_OS(hOldFile);
    Bfile_CloseFile_OS(hNewFile);
    // now rename the temp file to the correct file name
    Bfile_RenameEntry(tempfilenameshort , newfilenameshort);

    if(!silent) {
      mMsgBoxPush(5);
      if(action) {
        multiPrintXY(3, 2, "Decompression\nsuccessful.Delete\ncompressed file?",
                     TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      } else {
        multiPrintXY(3, 2, "Compression\nsuccessful.Delete\noriginal file?",
                     TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      }
    }
    if(silent || closeMsgBox(1)) {
      Bfile_DeleteEntry(oldfilenameshort);
      return;
    }
  } //else: create failed, return
}

int isFileCompressed(char* filename, int* origfilesize) {
  if(!strEndsWith(filename, (char*)COMPRESSED_FILE_EXTENSION)) return 0;
  int hFile = fileOpen(filename); // Get handle for the old file
  if(hFile < 0) return 0;
  unsigned char header[14] = "";
  int chl = sizeof(COMPRESSED_FILE_HEADER)-1;
  Bfile_ReadFile_OS(hFile, header, chl+4, -1 ); // +4 bytes for the original filesize info
  Bfile_CloseFile_OS(hFile);
  if(strncmp((char*)header, (char*)COMPRESSED_FILE_HEADER, chl)) return 0;
  *origfilesize = (header[chl] << 24) | (header[chl+1] << 16) |
                  (header[chl+2] << 8) | header[chl+3];
  return 1;
}

// wrapper around Bfile_OpenFile_OS that takes a "normal" string directly
// returns a file handle or the same error codes as Bfile_OpenFile_OS
int fileOpen(const char* filename) {
  unsigned short pFile[MAX_FILENAME_SIZE];
  Bfile_StrToName_ncpy(pFile, filename, MAX_FILENAME_SIZE); 
  return Bfile_OpenFile_OS(pFile, READWRITE, 0);
}
#ifdef ENABLE_PICOC_SUPPORT
extern "C" {
#include "picoc/picoc.h"
#include "picoc/interpreter.h"

/* read and scan a file for definitions */
void PicocPlatformScanFile(const char *filename)
{
    char* asrc = NULL;
    //Get file contents
    int hFile = fileOpen(filename); // Get handle
    if(hFile >= 0) // Check if it opened
    { //opened
        unsigned int filesize = Bfile_GetFileSize_OS(hFile);
        if(!filesize) {
            Bfile_CloseFile_OS(hFile);
            return;
        }
        if(filesize > MAX_TEXTVIEWER_FILESIZE) filesize = MAX_TEXTVIEWER_FILESIZE;
        // check if there's enough stack to proceed:
        if(0x881E0000 - (int)GetStackPtr() < 500000 - filesize*sizeof(unsigned char) - 30000) {
            asrc = (char*)alloca(filesize*sizeof(unsigned char)+5);
            Bfile_ReadFile_OS(hFile, asrc, filesize, 0);
            Bfile_CloseFile_OS(hFile);
            asrc[filesize] = '\0';
        } else {
            // there's not enough stack to put the file in RAM, so just return.
            // this can happen when opening a file from the search results
            Bfile_CloseFile_OS(hFile);
            return;
        }
    } else {
        //Error opening file, abort
        return;
    }
    PicocParse(filename, asrc, strlen(asrc), 1, 0, 0);
}

extern unsigned char* HeapMemory;         /* all memory - stack and heap */
extern void *HeapBottom;   /* the bottom of the (downward-growing) heap */
extern void *StackFrame;           /* the current stack frame */
extern void *HeapStackTop;                /* the top of the stack */
int picoc(char* SourceFile)
{
    int StackSize = HEAP_SIZE;
    HeapMemory = (unsigned char*)alloca(StackSize);
    HeapBottom = (void *)(HeapMemory + HEAP_SIZE);
    StackFrame = (void *)HeapMemory;
    HeapStackTop = (void *)HeapMemory;
    
    PicocInitialise(StackSize);

    if (PicocPlatformSetExitPoint())
    {
        PicocCleanup();
        return PicocExitValue;
    }
    
    PicocPlatformScanFile(SourceFile);
    
    //PicocCallMain(argc - ParamCount, &argv[ParamCount]);
    
    PicocCleanup();
    return PicocExitValue;
}

}
#endif