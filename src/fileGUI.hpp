#ifndef __FILEGUI_H
#define __FILEGUI_H
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

#include "fileProvider.hpp"
#include "menuGUI.hpp"

void fileManager();
void getFileManagerStatus(char* title, int itemsinclip, int ismanager);
int fileManagerChild(char* browserbasepath, int* itemsinclip, int* shownClipboardHelp, int* shownMainMemHelp, File* clipboard, char* filetoedit);
int deleteFilesPrompt(File* files, Menu* menu);
int newFolderScreen(char* browserbasepath);
int newG3Pscreen(char* browserbasepath);
int renameFileScreen(File* files, Menu* menu, char* browserbasepath);
int searchFilesScreen(char* browserbasepath, int itemsinclip);

#define VIEWFILEINFO_RETURN_EXIT 0
#define VIEWFILEINFO_RETURN_EDIT 1
#define VIEWFILEINFO_RETURN_RELOAD 2
#define VIEWFILEINFO_RETURN_EXECUTE 3
#define VIEWFILEINFO_RETURN_UP 4
#define VIEWFILEINFO_RETURN_DOWN 5
int viewFileInfo(File* file, int allowEdit, int itemsinclip, int allowUpDown=0);
void viewFileAsText(char* filename);
void viewClipboard(File* clipboard, int *itemsinclip);
void folderStatsScreen(File* files, Menu* menu);
void compressSelectedFiles(File* files, Menu* menu);
void decompressSelectedFiles(File* files, Menu* menu);
void shortenDisplayPath(char* longpath, char* shortpath, int jump=1);
void buildIconTable(MenuItemIcon* icontable);
int overwriteFilePrompt(char* filename);

#endif