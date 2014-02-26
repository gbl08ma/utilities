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
void fillMenuStatusWithClip(char* title, short itemsinclip, short ismanager);
int fileManagerSub(char* browserbasepath, short* itemsinclip, short* shownClipboardHelp, short* shownMainMemHelp, File* clipboard, char* filetoedit);
short deleteFilesGUI(File* files, Menu* menu);
short makeFolderGUI(char* browserbasepath);
short renameFileGUI(File* files, Menu* menu, char* browserbasepath);
short searchFilesGUI(char* browserbasepath, short itemsinclip);
short fileInformation(File* file, short allowEdit, short itemsinclip);
void fileViewAsText(char* filename);
void viewFilesInClipboard(File* clipboard, short *itemsinclip);
void showCopyFolderWarning();
void shortenDisplayPath(char* longpath, char* shortpath, short jump=1);
void buildIconTable(MenuItemIcon* icontable);

#endif