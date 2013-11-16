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
int fileManagerSub(char* browserbasepath, int* itemsinclip, int* shownClipboardHelp, int* shownMainMemHelp, File* clipboard);
int deleteFilesGUI(File* files, Menu* menu);
int makeFolderGUI(char* browserbasepath);
int renameFileGUI(File* files, Menu* menu, char* browserbasepath);
void fileInformation(File* files, Menu* menu);
void fileViewAsText(char* filename);
void viewFilesInClipboard(File* clipboard, int *itemsinclip);
void showCopyFolderWarning();
void shortenDisplayPath(char* longpath, char* shortpath, int jump=1);
void buildIconTable(MenuItemIcon* icontable);

#endif