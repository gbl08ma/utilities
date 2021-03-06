#ifndef __FILEPROVIDER_H
#define __FILEPROVIDER_H
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

#include "menuGUI.hpp"

#define MAX_FILENAME_SIZE 270 //full path with //fls0/, extension and everything
#define MAX_NAME_SIZE 128 //friendly name (in "//fls0/folder/file.txt", this would be "file.txt")
#define MAX_ITEMS_IN_DIR 200
#define MAX_ITEMS_IN_CLIPBOARD 75
#define MAX_TEXTVIEWER_FILESIZE 64*1024
#define MAX_TEXTVIEWER_LINES 2000
#define MAX_ITEMS_PER_FOLDER_COPY 100 // also applies to folders in recursive search
typedef struct
{
  char filename[MAX_FILENAME_SIZE]; //filename, not proper for use with Bfile.
  size_t visname; // offset of the last part inside filename (the ABC.def in \\fls0\123\ABC.def).
  size_t size; // file size
  short action; // mostly for clipboard, can be used to tag something to do with the file
  short isfolder; // because menuitem shouldn't be the only struct holding this info
} File;

typedef struct
{
  unsigned short id, type;
  unsigned long fsize, dsize;
  unsigned int property;
  unsigned long address;
} file_type_t;

#define GETFILES_SUCCESS 0
#define GETFILES_MAX_FILES_REACHED 1
#define GETFILES_USER_ABORTED 2

void sortFiles(File* data, int size);
int getFiles(File* files, const char* basepath, int* count);
int searchForFiles(File* files, const char* basepath, const char* needle, int searchOnFilename, int searchOnContents, int searchRecursively, int matchCase, int* count, int isRecursiveCall=0);
void deleteFiles(File* files, Menu* menu);
void renameFile(char* old, char* newf);
char* filenameToName(char* filename);
void copyFile(char* oldfilename, char* newfilename);
void pasteClipboard(File* clipboard, char* browserbasepath, int itemsInClipboard);

#define FILE_ICON_FOLDER 0
#define FILE_ICON_G3M 1
#define FILE_ICON_G3E 2
#define FILE_ICON_G3A 3
#define FILE_ICON_G3P 4
#define FILE_ICON_G3B 5
#define FILE_ICON_BMP 6
#define FILE_ICON_TXT 7
#define FILE_ICON_CSV 8
#define FILE_ICON_OTHER 9
int filenameToIcon(char* name);
int stringEndsInG3A(char* string);
int stringEndsInJPG(char* string);
void createFolderRecursive(const char* folder);

// compression settings:
#define COMPRESSED_FILE_HEADER "PHSCOMPR" //Prizm heatshrink compressed
#define COMPRESSED_FILE_EXTENSION ".phc"
void compressFile(char* oldfilename, char* newfilename, int action, int silent=0);
int isFileCompressed(char* filename, int* origfilesize);
int fileOpen(const char* filename);

#ifdef ENABLE_PICOC_SUPPORT
extern "C" {
int picoc(char* SourceFile);
}
#endif

#endif