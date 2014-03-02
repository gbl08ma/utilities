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
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "menuGUI.hpp"
#include "inputGUI.hpp"
#include "fileProvider.hpp" 
#include "fileGUI.hpp"
#include "textGUI.hpp"
#include "sha2.h"
#include "editorGUI.hpp"
#include "debugGUI.hpp"

void fileManager() {
  int res = 1;
  int itemsinclip = 0;
  int shownClipboardHelp = 0;
  int shownMainMemHelp = 0;
  char browserbasepath[MAX_FILENAME_SIZE+1] = "\\\\fls0\\";
  char filetoedit[MAX_FILENAME_SIZE+1] = "";
  File* clipboard = (File*)alloca((MAX_ITEMS_IN_CLIPBOARD+1)*sizeof(File));
  while(res) {
    strcpy(filetoedit, (char*)"");
    res = fileManagerSub(browserbasepath, &itemsinclip, &shownClipboardHelp, &shownMainMemHelp, clipboard, filetoedit);
    if(strlen(filetoedit) > 0) {
      fileTextEditor(filetoedit);
    }
  }
}
int smemfree = 0;
void fillMenuStatusWithClip(char* title, int itemsinclip, int ismanager) {
  char titleBuffer[88] = "";
  if(!itemsinclip) {
    itoa(smemfree, (unsigned char*)title);
    LocalizeMessage1( 340, titleBuffer ); //"bytes free"
    strncat((char*)title, (char*)titleBuffer, 65);
  } else {
    strcpy((char*)title, "Clipboard: ");
    itoa(itemsinclip, (unsigned char*)titleBuffer);
    strcat((char*)title, titleBuffer);
    strcat((char*)title, (itemsinclip == 1 ? " item" : " items"));
    if(!ismanager) {
      strcat((char*)title, ", Shift\xe6\x91""9=Paste");
    }
  }
}

int fileManagerSub(char* browserbasepath, int* itemsinclip, int* shownClipboardHelp, int* shownMainMemHelp, File* clipboard, char* filetoedit) {
  Menu menu;
  MenuItemIcon icontable[12];
  buildIconTable(icontable);
  
  // first get file count so we know how much to alloc
  int res = GetAnyFiles(NULL, NULL, browserbasepath, &menu.numitems);
  if(res == GETFILES_MAX_FILES_REACHED) {
    // show "folder has over 200 items, some will be skipped" message
    mMsgBoxPush(5);
    PrintXY_2(TEXT_MODE_NORMAL, 1, 2, 1833, TEXT_COLOR_BLACK); // folder has over
    PrintXY_2(TEXT_MODE_NORMAL, 1, 3, 1834, TEXT_COLOR_BLACK); // 200 files
    PrintXY_2(TEXT_MODE_NORMAL, 1, 4, 1835, TEXT_COLOR_BLACK); // some will
    PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 1836, TEXT_COLOR_BLACK); // be skipped
    PrintXY_2(TEXT_MODE_NORMAL, 1, 6, 2, TEXT_COLOR_BLACK); // press exit message
    closeMsgBox();
  }
  MenuItem* menuitems = NULL;
  File* files = NULL;
  if(menu.numitems > 0) {
    menuitems = (MenuItem*)alloca(menu.numitems*sizeof(MenuItem));
    files = (File*)alloca(menu.numitems*sizeof(File));
    // populate arrays
    GetAnyFiles(files, menuitems, browserbasepath, &menu.numitems);
    menu.items = menuitems;
  }
  
  unsigned short smemMedia[10]={'\\','\\','f','l','s','0',0};
  Bfile_GetMediaFree_OS( smemMedia, &smemfree );
  
  char friendlypath[MAX_FILENAME_SIZE];
  strcpy(friendlypath, browserbasepath+6);
  friendlypath[strlen(friendlypath)-1] = '\0'; //remove ending slash like OS does
  // test to see if friendlypath is too big
  int jump4=0;
  while(1) {
    int temptextX=5*18+10; // px length of menu title + 10, like menuGUI goes.
    int temptextY=0;
    PrintMini(&temptextX, &temptextY, (unsigned char*)friendlypath, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); // fake draw
    if(temptextX>LCD_WIDTH_PX-6) {
      char newfriendlypath[MAX_FILENAME_SIZE];
      shortenDisplayPath(friendlypath, newfriendlypath, (jump4 ? 4 : 1));
      if(strlen(friendlypath) > strlen(newfriendlypath) && strlen(newfriendlypath) > 3) { // check if len > 3 because shortenDisplayPath may return just "..." when the folder name is too big
        // shortenDisplayPath still managed to shorten, copy and continue
        jump4 = 1; //it has been shortened already, so next time jump the first four characters
        strcpy(friendlypath, newfriendlypath);
      } else {
        // shortenDisplayPath can't shorten any more even if it still
        // doesn't fit in the screen, so give up.
        break;
      }
    } else break;
  }
  menu.subtitle = friendlypath;
  menu.showsubtitle = 1;
  menu.type = MENUTYPE_MULTISELECT;
  menu.height = 7;
  menu.scrollout=1;
  strcpy(menu.nodatamsg, "No Data");
  strcpy(menu.title, "Files");
  menu.showtitle=1;
  menu.useStatusText=1;
  while(1) {
    Bdisp_AllClr_VRAM();
    fillMenuStatusWithClip((char*)menu.statusText, *itemsinclip, 0);
    if(menu.fkeypage == 0) {
      drawFkeyLabels(-1, // set by menu as SELECT [empty]
        (menu.numselitems>0 ? 0x0069 : 0x03B6), // CUT (white) or SEQ
        (menu.numselitems>0 ? 0x0034 : (menu.numitems>0 ? 0x0187 : -1)), // COPY (white) or SEARCH (only if there are items)
        0x0186, // NEW
        (menu.numitems>0 ? 0x0188 : -1), // RENAME or nothing
        (menu.numselitems>0 ? 0x0038 : -1)); // DELETE or nothing
    }
    res = doMenu(&menu, icontable);
    switch(res) {
      case MENU_RETURN_EXIT:
        if(!strcmp(browserbasepath,"\\\\fls0\\")) { //check that we aren't already in the root folder
          //we are, return 0 so we exit
          return 0;
        } else {
          int i=strlen(browserbasepath)-2;
          while (i>=0 && browserbasepath[i] != '\\')
                  i--;
          if (browserbasepath[i] == '\\') browserbasepath[i+1] = '\0';
          return 1; //reload at new folder
        }
        break;
      case MENU_RETURN_SELECTION:
        if(menuitems[menu.selection-1].isfolder) {
          strcpy(browserbasepath, files[menu.selection-1].filename); //switch to selected folder
          strcat(browserbasepath, "\\");
          if(!strcmp(browserbasepath, "\\\\fls0\\@MainMem\\") && !*shownMainMemHelp) {
            mMsgBoxPush(5);
            mPrintXY(3, 2, (char*)"Note: this is not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 3, (char*)"the Main Memory,", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 4, (char*)"just a special", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 5, (char*)"mirror of it.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            PrintXY_2(TEXT_MODE_NORMAL, 1, 6, 2, TEXT_COLOR_BLACK); // press exit message
            closeMsgBox();
            *shownMainMemHelp=1;
          }
          return 1; //reload at new folder
        } else {
          int res = fileInformation(&files[menu.selection-1], 1, *itemsinclip);
          if(1 == res) {
            // user wants to edit the file
            strcpy(filetoedit, files[menu.selection-1].filename);
            return 1;
          } else if(2 == res) return 1;
        }
        break;
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
        if (menu.numselitems > 0) {
          if((*itemsinclip < MAX_ITEMS_IN_CLIPBOARD) && menu.numselitems <= MAX_ITEMS_IN_CLIPBOARD-*itemsinclip) {
            int ifile = 0;
            while(ifile < menu.numitems) {  
              if (menu.items[ifile].isselected) {
                int inclip = 0;
                int clippos = 0;
                for(int i = 0; i<*itemsinclip; i++) {
                  if(!strcmp(clipboard[i].filename, files[ifile].filename)) {
                    inclip=1;
                    clippos = i;
                  }
                }
                if(!inclip) {
                  strcpy(clipboard[*itemsinclip].filename, files[ifile].filename);
                  //0=cut file; 1=copy file:
                  clipboard[*itemsinclip].action = (res == KEY_CTRL_F2 ? 0 : 1);
                  clipboard[*itemsinclip].isfolder = files[ifile].isfolder;
                  clipboard[*itemsinclip].size = files[ifile].size;
                  *itemsinclip = *itemsinclip + 1;
                } else {
                  // file is already in the clipboard
                  // set it to the opposite action
                  clipboard[clippos].action = !clipboard[clippos].action;
                }
                menu.items[ifile].isselected = 0; // clear selection
                menu.numselitems--;
              }
              ifile++;
            }
          } else {
            mMsgBoxPush(4);
            mPrintXY(3, 2, (char*)"The clipboard is", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 3, (char*)"full; can't add", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 4, (char*)"more items to it.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
            closeMsgBox();
          }
        } else if (menu.numselitems == 0) {
          if(res==KEY_CTRL_F2) {
            mMsgBoxPush(6);
            MenuItem smallmenuitems[5];
            strcpy(smallmenuitems[0].text, "Do not sort");
            strcpy(smallmenuitems[1].text, "Name (A to Z)");
            strcpy(smallmenuitems[2].text, "Name (Z to A)");
            strcpy(smallmenuitems[3].text, "Size (small 1st)");
            strcpy(smallmenuitems[4].text, "Size (big 1st)");
            
            Menu smallmenu;
            smallmenu.items=smallmenuitems;
            smallmenu.numitems=5;
            smallmenu.width=17;
            smallmenu.height=6;
            smallmenu.startX=3;
            smallmenu.startY=2;
            smallmenu.scrollbar=0;
            smallmenu.showtitle=1;
            smallmenu.selection = GetSetting(SETTING_FILE_MANAGER_SORT)+1;
            strcpy(smallmenu.title, "Sort items by:");
            int sres = doMenu(&smallmenu);
            mMsgBoxPop();
            
            if(sres == MENU_RETURN_SELECTION) {
              SetSetting(SETTING_FILE_MANAGER_SORT, smallmenu.selection-1, 1);
              if(menu.numitems > 1) {
                HourGlass(); //sorting can introduce a noticeable delay when there are many items
                insertSortFileMenuArray(files, menuitems, menu.numitems);
              }
            }
          } else if(menu.numitems && searchFilesGUI(browserbasepath, *itemsinclip)) return 1;
        }
        break;
      case KEY_CTRL_F4: {
        mMsgBoxPush(4);
        MenuItem smallmenuitems[5];
        strcpy(smallmenuitems[0].text, "Folder");
        strcpy(smallmenuitems[1].text, "Text File");
        
        Menu smallmenu;
        smallmenu.items=smallmenuitems;
        smallmenu.numitems=2;
        smallmenu.width=17;
        smallmenu.height=4;
        smallmenu.startX=3;
        smallmenu.startY=2;
        smallmenu.scrollbar=0;
        smallmenu.showtitle=1;
        strcpy(smallmenu.title, "Create new:");
        int sres = doMenu(&smallmenu);
        mMsgBoxPop();
        
        if(sres == MENU_RETURN_SELECTION) {
          if(smallmenu.selection == 1) {
            if(makeFolderGUI(browserbasepath)) return 1; // if user said yes and a folder was created, reload file list
          } else if(smallmenu.selection == 2) {
            fileTextEditor(NULL, browserbasepath); return 1;
          }
        }
        break;
      }
      case KEY_CTRL_F5:
        if(menu.numitems>0) if(renameFileGUI(files, &menu, browserbasepath)) return 1;
        break;
      case KEY_CTRL_F6:
        if(menu.numselitems>0) if(deleteFilesGUI(files, &menu)) return 1; // if user said yes and files were deleted, reload file list
        break;
      case KEY_CTRL_PASTE:
        filePasteClipboardItems(clipboard, browserbasepath, *itemsinclip);
        *itemsinclip = 0;
        return 1;
        break;
      case KEY_CTRL_OPTN:
        viewFilesInClipboard(clipboard, itemsinclip);
        break;
    }
  }
  return 1;
}

int deleteFilesGUI(File* files, Menu* menu) {
  mMsgBoxPush(4);
  mPrintXY(3, 2, (char*)"Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 3, (char*)"Selected Items?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY_2(TEXT_MODE_NORMAL, 1, 4, 3, TEXT_COLOR_BLACK); // yes, F1
  PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 4, TEXT_COLOR_BLACK); // no, F6
  int key;
  while (1) {
    mGetKey(&key);
    switch(key) {
      case KEY_CTRL_F1:
        mMsgBoxPop();
        deleteFiles(files, menu);
        return 1;
      case KEY_CTRL_F6:
      case KEY_CTRL_EXIT:
        mMsgBoxPop();
        return 0;
    }
  } 
}

int makeFolderGUI(char* browserbasepath) {
  //browserbasepath: folder we're working at
  //reload the files array after using this function!
  //returns 1 if user aborts, 0 if makes folder.
  SetBackGround(10);
  clearLine(1,8);
  clearLine(1,3); // clear background at end of input
  drawScreenTitle((char*)"Create folder", (char*)"Name:");
  char newname[MAX_NAME_SIZE] = "";
  textInput input;
  input.forcetext=1;
  input.charlimit=MAX_NAME_SIZE;
  input.buffer = (char*)newname;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return 0; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      // create folder
      char newfilename[MAX_FILENAME_SIZE];
      strcpy(newfilename, browserbasepath);
      strcat(newfilename, newname);
      unsigned short newfilenameshort[0x10A];
      Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
      Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FOLDER, 0); //create a folder
      return 1;
    }
  }
  return 0;
}

int renameFileGUI(File* files, Menu* menu, char* browserbasepath) {
  //reload the files array after using this function!
  //returns 0 if user aborts, 1 if renames.
  SetBackGround(6);
  clearLine(1,8);
  clearLine(1,3); // clear background at end of input
  char title[MAX_NAME_SIZE+6];
  strcpy(title, (char*)menu->items[menu->selection-1].text);
  strcat(title, " to:");
  drawScreenTitle((char*)"Rename item", title);
  char newname[MAX_NAME_SIZE];
  strcpy(newname, (char*)menu->items[menu->selection-1].text);
  textInput input;
  input.forcetext=1;
  input.charlimit=MAX_NAME_SIZE;
  input.buffer = (char*)newname;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return 0; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      char newfilename[MAX_FILENAME_SIZE];
      strcpy(newfilename, browserbasepath);
      strcat(newfilename, newname);
      unsigned short newfilenameshort[0x10A];
      unsigned short oldfilenameshort[0x10A];
      Bfile_StrToName_ncpy(oldfilenameshort, (unsigned char*)files[menu->selection-1].filename, 0x10A);
      Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
      Bfile_RenameEntry(oldfilenameshort , newfilenameshort);
      return 1;
    }
  }
  return 0;
}

int searchFilesGUI(char* browserbasepath, int itemsinclip) {
  // returns 1 when it wants the caller to jump to browserbasepath
  // returns 0 otherwise.
  char statusText[100];
  fillMenuStatusWithClip((char*)statusText, itemsinclip, 1);
  DefineStatusMessage((char*)statusText, 1, 0, 0);
  
  int searchOnFilename = !!(GetSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 0));
  int searchOnContents = !!(GetSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 1));
  int searchRecursively = !!(GetSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 2));
  int matchCase = !!(GetSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 3));
  
  char needle[55] = "";
  textInput input;
  input.forcetext=1; //force text so needle must be at least one char.
  input.charlimit=50;
  input.acceptF6=1;
  input.buffer = needle;
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "On filename");
  menuitems[0].type = MENUITEM_CHECKBOX;
  strcpy(menuitems[1].text, "On contents");
  menuitems[1].type = MENUITEM_CHECKBOX;
  strcpy(menuitems[2].text, "Recursively");
  menuitems[2].type = MENUITEM_CHECKBOX;
  strcpy(menuitems[3].text, "Match case");
  menuitems[3].type = MENUITEM_CHECKBOX;

  Menu menu;
  menu.items=menuitems;
  menu.type=MENUTYPE_FKEYS;
  menu.numitems=4;
  menu.height=4;
  menu.startY=4;
  menu.scrollbar=0;
  menu.scrollout=1;
  menu.pBaRtR=1;
  int curstep = 0;
  while(1) {
    if(curstep == 0) {
      SetBackGround(9);
      drawScreenTitle((char*)"File Search", (char*)"Search for:");
      clearLine(1, 3); // remove aestethically unpleasing bit of background at the end of the field
      drawFkeyLabels(0, 0, 0, 0, 0, 0x04A3); // Next
      while(1) {
        input.key=0;
        int res = doTextInput(&input);
        if (res==INPUT_RETURN_EXIT) return 0; // user aborted
        else if (res==INPUT_RETURN_CONFIRM) break; // continue to next step
      }
      curstep++;
    } else {
      int inloop=1;
      while(inloop) {
        // this must be here, inside this loop:
        SetBackGround(9);
        drawScreenTitle((char*)"File Search", (char*)"Search for:");
        mPrintXY(1, 3, (char*)needle, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        drawFkeyLabels(0x036F, 0, 0, 0, 0, 0x00A5); // <, SEARCH (white)
        menuitems[0].value = searchOnFilename;
        menuitems[1].value = searchOnContents;
        menuitems[2].value = searchRecursively;
        menuitems[3].value = matchCase;
        int res = doMenu(&menu);
        if(res == MENU_RETURN_EXIT) return 0;
        else if(res == KEY_CTRL_F1) {
          curstep--;
          break;
        } else if(res == KEY_CTRL_F6) inloop=0;
        else if(res == MENU_RETURN_SELECTION) {
          switch(menu.selection) {
            case 1:
              searchOnFilename = !menuitems[0].value;
              if(!searchOnFilename && !searchOnContents) searchOnContents = 1;
              break;
            case 2:
              searchOnContents = !menuitems[1].value;
              if(!searchOnFilename && !searchOnContents) searchOnFilename = 1;
              break;
            case 3:
              searchRecursively = !menuitems[2].value;
              break;
            case 4:
              matchCase = !menuitems[3].value;
              break;
          }
        }
      }
      if(!inloop) break;
    }
  }
  unsigned int newsetting = 0;
  if(searchOnFilename) newsetting |= (1U << 0);
  if(searchOnContents) newsetting |= (1U << 1);
  if(searchRecursively) newsetting |= (1U << 2);
  if(matchCase) newsetting |= (1U << 3);
  SetSetting(SETTING_FILE_MANAGER_SEARCH, newsetting, 1); // remember search preferences
  SearchForFiles(NULL, browserbasepath, needle, searchOnFilename, searchOnContents, searchRecursively, matchCase, &menu.numitems);
  MenuItem* resitems = (MenuItem*)alloca(menu.numitems*sizeof(MenuItem));
  File* files = (File*)alloca(menu.numitems*sizeof(File));
  if(menu.numitems) SearchForFiles(files, browserbasepath, needle, searchOnFilename, searchOnContents, searchRecursively, matchCase, &menu.numitems);
  int curitem = 0;
  while(curitem < menu.numitems) {
    nameFromFilename(files[curitem].filename, resitems[curitem].text, 42);
    resitems[curitem].type = MENUITEM_NORMAL;
    resitems[curitem].color = COLOR_BLACK;
    curitem++;
  }
  
  menu.height=7;
  menu.selection = 1;
  menu.startY=1;
  menu.pBaRtR=0;
  strcpy(menu.nodatamsg, "No files found");
  strcpy(menu.title, "Search results");
  menu.showtitle = 1;
  menu.scrollbar=1;
  menu.items = resitems;
  
  while(1) {
    Bdisp_AllClr_VRAM();
    if(menu.numitems>0) drawFkeyLabels(0x049F, 0x01FC); // VIEW, JUMP
    switch(doMenu(&menu)) {
      case MENU_RETURN_EXIT:
        return 0;
        break;
      case KEY_CTRL_F1:
      case MENU_RETURN_SELECTION:
        if(menu.numitems>0) {
          if(files[menu.selection-1].isfolder) {
            strcpy(browserbasepath, files[menu.selection-1].filename);
            strcat(browserbasepath, "\\");
            return 1;
          } else fileInformation(&files[menu.selection-1], 0, itemsinclip);
        }
        break;
      case KEY_CTRL_F2:
        if(menu.numitems>0) {
          if(!files[menu.selection-1].isfolder) {
            // get folder path from full filename
            // we can work on the string in the files array, as it is going to be discarded right after:
            int i=strlen(files[menu.selection-1].filename)-1;
            while (i>=0 && files[menu.selection-1].filename[i] != '\\')
                    i--;
            if (files[menu.selection-1].filename[i] == '\\') files[menu.selection-1].filename[i] = '\0';
          }
          strcpy(browserbasepath, files[menu.selection-1].filename);
          strcat(browserbasepath, "\\");
          return 1;
        }
        break;
    }
  }
}

int fileInformation(File* file, int allowEdit, int itemsinclip) {
  // returns 0 if user exits.
  // returns 1 if user wants to edit the file
  // returns 2 if user compressed or decompressed the file
  textArea text;
  text.type=TEXTAREATYPE_INSTANT_RETURN;
  text.scrollbar=0;
  strcpy(text.title, (char*)"File information");
  
  textElement elem[10];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor
  
  elem[text.numelements].text = (char*)"File name:";
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  text.numelements++;
  
  elem[text.numelements].newLine=1;
  char name[MAX_NAME_SIZE];
  nameFromFilename(file->filename, name);
  elem[text.numelements].text = name;
  text.numelements++;
  
  elem[text.numelements].newLine=1;
  elem[text.numelements].lineSpacing=3;
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].text = (char*)"Full file path:";
  text.numelements++;
  
  elem[text.numelements].newLine=1;
  elem[text.numelements].minimini=1;
  elem[text.numelements].color=TEXT_COLOR_BLACK;
  elem[text.numelements].text = (char*)file->filename;
  text.numelements++;
  
  elem[text.numelements].newLine=1;
  elem[text.numelements].lineSpacing=3;
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].text = (char*)"File type:";
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  // get file type description from OS
  unsigned int msgno;
  unsigned short iconbuffer[0x12*0x18];
  unsigned short folder[7]={};
  SMEM_MapIconToExt( (unsigned char*)name, folder, &msgno, iconbuffer );
  char mresult[88] = "";
  LocalizeMessage1( msgno, mresult );
  
  elem[text.numelements].text = (char*)mresult;
  text.numelements++;
  char sizebuffer[50];
  itoa(file->size, (unsigned char*)sizebuffer);
  
  elem[text.numelements].newLine=1;
  elem[text.numelements].lineSpacing=3;
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].text = (char*)"File size:";
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  elem[text.numelements].text = (char*)sizebuffer;
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  elem[text.numelements].text = (char*)"bytes";
  text.numelements++;

  int compressed = 0;
  if(isFileCompressed(file->filename)) compressed = 1;
  
  while (1) {
    char statusText[100];
    fillMenuStatusWithClip((char*)statusText, itemsinclip, 1);
    DefineStatusMessage((char*)statusText, 1, 0, 0);
    doTextArea(&text);
    drawFkeyLabels(0x03B1, (allowEdit? 0x0185: -1), (allowEdit? (compressed ? 0x161 : 0x160) : -1), -1, -1, (file->size>0 ? 0x0371 : -1)); //OPEN, EDIT, Comp (white) or Dec (white), CALC (white)
    int key;
    mGetKey(&key);
    switch(key) {
      case KEY_CTRL_EXIT:
      case KEY_CTRL_LEFT:
        return 0;
        break;
      case KEY_CTRL_F1:
        fileViewAsText(file->filename);
        break;
      case KEY_CTRL_F2:
        if(allowEdit) {
          if(stringEndsInG3A(name)) {
            mMsgBoxPush(4);
            mPrintXY(3, 2, (char*)"g3a files can't", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 3, (char*)"be edited by", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 4, (char*)"an add-in.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
            closeMsgBox();
          } else {
            return 1;
          }
        }
        break;
      case KEY_CTRL_F3:
        if(allowEdit) {
          if(compressed) {
            char newfilename[MAX_FILENAME_SIZE] = "";
            int len=strlen(file->filename);
            strncpy(newfilename, file->filename, len-4); //strip file extension
            newfilename[len-4] = '\0'; // strncpy does not zero-terminate when limit is reached
            compressFile(file->filename, newfilename, 1);
          } else {
            char newfilename[MAX_FILENAME_SIZE] = "";
            strcpy(newfilename, file->filename);
            strcat(newfilename, (char*)COMPRESSED_FILE_EXTENSION);
            compressFile(file->filename, newfilename, 0);
          }
          return 2;
        }
        break;
      case KEY_CTRL_F6:
        if(file->size > 0) {
          unsigned short pFile[MAX_FILENAME_SIZE+1];
          Bfile_StrToName_ncpy(pFile, (unsigned char*)file->filename, strlen(file->filename)+1); 
          int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
          if(hFile >= 0) // Check if it opened
          { //opened
            unsigned char output[32] = "";
            sha2_context ctx;
            unsigned char buf[4096];
            int readsize = 1; // not zero so we have the chance to enter the while loop
            sha2_starts( &ctx, 0 );
            while( readsize > 0) {
              readsize = Bfile_ReadFile_OS(hFile, buf, sizeof( buf ), -1);
              sha2_update( &ctx, buf, readsize );
            }
            Bfile_CloseFile_OS(hFile);
            sha2_finish( &ctx, output );
            memset( &ctx, 0, sizeof( sha2_context ) );
            
            mMsgBoxPush(4);
            unsigned char niceout[32] = "";
            int textX=2*18, textY=24;
            PrintMini(&textX, &textY, (unsigned char*)"SHA-256 checksum:", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
            textY=textY+20;
            textX=2*18;
            for(int i=0; i<32;i++) {
              strcpy((char*)niceout, (char*)"");
              ByteToHex( output[i], niceout );
              if((LCD_WIDTH_PX-2*18-textX) < 15) { textX=2*18; textY=textY+12; }
              PrintMiniMini( &textX, &textY, (unsigned char*)niceout, 0, TEXT_COLOR_BLACK, 0 );
            }
            PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
            closeMsgBox();
          }
        }
        break;
    }
  }
}

void fileViewAsText(char* filename) { //name is the "nice" name of the file, i.e. not full path
  char name[MAX_NAME_SIZE];
  nameFromFilename(filename, name);
  
  unsigned char* asrc = NULL;
  //Get file contents
  unsigned short pFile[MAX_FILENAME_SIZE];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  unsigned int filesize = 0;
  if(hFile >= 0) // Check if it opened
  { //opened
    filesize = Bfile_GetFileSize_OS(hFile);
    if(!(filesize && filesize < MAX_TEXTVIEWER_FILESIZE)) filesize = MAX_TEXTVIEWER_FILESIZE;
    // check if there's enough stack to proceed:
    if(0x881E0000 - (int)GetStackPtr() < 500000 - filesize*sizeof(unsigned char) - 30000) {
      asrc = (unsigned char*)alloca(filesize*sizeof(unsigned char));
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
    mMsgBoxPush(4);
    mPrintXY(3, 2, (char*)"Error opening", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, (char*)"file to read.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
    closeMsgBox();
    return;
  }
  char titlebuf[MAX_NAME_SIZE+20];
  strcpy((char*)titlebuf, "Viewing ");
  strcat((char*)titlebuf, (char*)name);
  strcat((char*)titlebuf, " as text");
  DefineStatusMessage((char*)titlebuf, 1, 0, 0);
  textArea text;
  text.showtitle = 0;
  
  // get number of lines so we know how much textElement to allocate
  text.numelements = 1; // at least one line it will have
  unsigned int bcur = 0;
  while(bcur < strlen((char*)asrc)) {
    int jump = 1;
    if(*(asrc+bcur) == '\r' && *(asrc+bcur+1) == '\n') {
      jump=2;
      text.numelements++;
    } else if(*(asrc+bcur) == '\r') {
      text.numelements++;
    } else if(*(asrc+bcur) == '\n') {
      text.numelements++;
    }
    bcur += jump;
  }
  
  textElement* elem = (textElement*)alloca(text.numelements*sizeof(textElement));
  text.elements = elem;
  elem[0].text = (char*)asrc;
  elem[0].newLine=0;
  elem[0].color=COLOR_BLACK;
  elem[0].lineSpacing = 0;
  elem[0].spaceAtEnd = 0;
  elem[0].minimini = 0;
  int ecur = 1;
  bcur = 0;
  while(bcur < filesize && ecur <= text.numelements) {
    int jump = 1;
    if(*(asrc+bcur) == '\r' || *(asrc+bcur) == '\n') {
      //char after the next one (\n) will be the start of a new line
      //mark this char as null (so previous element ends here) and point this element to the start of the new line
      jump = ((*(asrc+bcur) == '\r' && *(asrc+bcur+1) == '\n') ? 2 : 1);
      *(asrc+bcur)='\0';
      elem[ecur].text = (char*)(asrc+bcur+jump);
      elem[ecur].newLine=1;
      elem[ecur].color=COLOR_BLACK;
      elem[ecur].lineSpacing = 0;
      elem[ecur].spaceAtEnd = 0;
      elem[ecur].minimini = 0;
      ecur++;
    }
    bcur += jump;
  }
  doTextArea(&text);
}

void viewFilesInClipboard(File* clipboard, int* itemsinclip) {
  Menu menu;
  menu.height = 7;
  menu.scrollout = 1;
  menu.type=MENUTYPE_FKEYS;
  while(1) {
    if(*itemsinclip<=0) return;
    if(menu.selection > *itemsinclip) menu.selection = 1;
    MenuItem menuitems[MAX_ITEMS_IN_CLIPBOARD];
    strcpy(menu.title, "Clipboard");
    menu.showtitle=1;
    menu.subtitle = (char*)"Black=cut, Red=copy";
    menu.showsubtitle=1;
    fillMenuStatusWithClip((char*)menu.statusText, *itemsinclip, 1);
    menu.useStatusText = 1;
    menu.items = menuitems;
    int curitem = 0;
    while(curitem < *itemsinclip) {
      char buffer[MAX_FILENAME_SIZE];
      nameFromFilename(clipboard[curitem].filename, buffer);
      strncpy(menuitems[curitem].text, (char*)buffer, 40);
      if(clipboard[curitem].action == 1) menuitems[curitem].color = TEXT_COLOR_RED;
      curitem++;
    }
    menu.numitems = *itemsinclip;
    drawFkeyLabels(0x0149, 0x04DB, 0x049D, 0, 0, 0); // CLEAR [white], X symbol [white], Switch [white], clear rest of line
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        if(!clipboard[menu.selection-1].isfolder) fileInformation(&clipboard[menu.selection-1], 0, *itemsinclip);
        break;
      case KEY_CTRL_F1:
        *itemsinclip = 0;
      case MENU_RETURN_EXIT:
        return;
        break;
      case KEY_CTRL_F2:
        if (menu.selection-1 >= *itemsinclip) {} // safety check
        else {
          for (int k = menu.selection-1; k < *itemsinclip - 1; k++) {
            menuitems[k] = menuitems[k+1];
            clipboard[k] = clipboard[k+1];
          }
          *itemsinclip = *itemsinclip - 1;
        }
        break;
      case KEY_CTRL_F3:
        clipboard[menu.selection-1].action = !clipboard[menu.selection-1].action;
        if(clipboard[menu.selection-1].action) menuitems[menu.selection-1].color = TEXT_COLOR_BLACK;
        else menuitems[menu.selection-1].color = TEXT_COLOR_RED;
        break;
    }
  }
}

void shortenDisplayPath(char* longpath, char* shortpath, int jump) {
  //this function takes a long path for display, like \myfolder\long\display\path
  //and shortens it one level, like this: ...\long\display\path
  //putting the result in shortpath
  strcpy(shortpath, (char*)"...");
  int i = jump; // jump the specified amount of characters... by default it jumps the first /
  // but it can also be made to jump e.g. 4 characters, which would jump ".../" (useful for when the text has been through this function already)
  int max = strlen(longpath);
  while (i<max && longpath[i] != '\\')
          i++;
  if (longpath[i] == '\\') {
    strcat(shortpath, longpath+i);
  }
}

void buildIconTable(MenuItemIcon* icontable) {
  unsigned int msgno;
  unsigned short folder[7]={'\\','\\','f','l','s','0',0};

  const char *bogusFiles[] = {"t", // for folder
                              "t.g3m",
                              "t.g3e",
                              "t.g3a",
                              "t.g3p",
                              "t.g3b",
                              "t.bmp",
                              "t.txt",
                              "t.csv",
                              "t.abc" //for "unsupported" files
                             };

  for(int i = 0; i < 10; i++)
    SMEM_MapIconToExt( (unsigned char*)bogusFiles[i], (i==0 ? folder : (unsigned short*)"\x000\x000"), &msgno, icontable[i].data );
}