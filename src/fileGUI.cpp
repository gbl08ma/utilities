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
  
  char titleBuffer[23] = ""; 
  int smemfree = 0;
  unsigned short smemMedia[10]={'\\','\\','f','l','s','0',0};
  Bfile_GetMediaFree_OS( smemMedia, &smemfree );
  
  char friendlypath[MAX_FILENAME_SIZE] = "";
  strcpy(friendlypath, browserbasepath+6);
  friendlypath[strlen(friendlypath)-1] = '\0'; //remove ending slash like OS does
  // test to see if friendlypath is too big
  int jump4=0;
  while(1) {
    int temptextX=5*18+10; // px length of menu title + 10, like menuGUI goes.
    int temptextY=0;
    PrintMini(&temptextX, &temptextY, (unsigned char*)friendlypath, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); // fake draw
    if(temptextX>LCD_WIDTH_PX-6) {
      char newfriendlypath[MAX_FILENAME_SIZE] = "";
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
    } else {
      break;
    }
  }
  menu.subtitle = friendlypath;
  menu.showsubtitle = 1;
  menu.type = MENUTYPE_MULTISELECT;
  menu.height = 7;
  menu.scrollout=1;
  strcpy(menu.nodatamsg, "No Data");
  strcpy(menu.title, "Files");
  menu.showtitle=1;
  while(1) {
    Bdisp_AllClr_VRAM();
    if(*itemsinclip==0) {
      itoa(smemfree, (unsigned char*)menu.statusText);
      LocalizeMessage1( 340, titleBuffer ); //"bytes free"
      strcat((char*)menu.statusText, (char*)titleBuffer);
    } else {
      strcpy((char*)menu.statusText, "Clipboard: ");
      itoa(*itemsinclip, (unsigned char*)titleBuffer);
      strcat((char*)menu.statusText, titleBuffer);
      if(*itemsinclip == 1) { strcat((char*)menu.statusText, " item, Shift\xe6\x91"); strcat((char*)menu.statusText, "9=Paste"); }
      else { strcat((char*)menu.statusText, " items, Shift\xe6\x91"); strcat((char*)menu.statusText, "9=Paste"); }
    }
    int iresult;
    if(menu.fkeypage == 0) {
      if(menu.numitems>0) {
        GetFKeyPtr(0x0188, &iresult); // RENAME
        FKey_Display(4, (int*)iresult);
      }
      if(menu.numselitems>0) {
        GetFKeyPtr(0x0069, &iresult); // CUT (white)
        FKey_Display(1, (int*)iresult);
        GetFKeyPtr(0x0034, &iresult); // COPY (white)
        FKey_Display(2, (int*)iresult);
        GetFKeyPtr(0x0038, &iresult); // DELETE
        FKey_Display(5, (int*)iresult);
      }
      GetFKeyPtr(0x038E, &iresult); // MKFLDR
      FKey_Display(3, (int*)iresult);
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
          if (browserbasepath[i] == '\\') {
            char tmp[MAX_FILENAME_SIZE] = "";
            memcpy(tmp,browserbasepath,i+1);
            tmp[i+1] = '\0';
            strcpy(browserbasepath, tmp);
          }
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
          if(1 == fileInformation(files[menu.selection-1].filename)) {
            // user wants to edit the file
            strcpy(filetoedit, files[menu.selection-1].filename);
            return 1;
          }
        }
        break;
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
        if (menu.numselitems > 0 && menu.fkeypage==0) {
          if((*itemsinclip < MAX_ITEMS_IN_CLIPBOARD) && menu.numselitems <= MAX_ITEMS_IN_CLIPBOARD-*itemsinclip) {
            int ifile = 0; int hasShownFolderCopyWarning = 0;
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
                if(res == KEY_CTRL_F2) {
                  if(!inclip) {
                    strcpy(clipboard[*itemsinclip].filename, files[ifile].filename);
                    //0=cut file; 1=copy file:
                    clipboard[*itemsinclip].action = 0;
                    clipboard[*itemsinclip].isfolder = menu.items[ifile].isfolder;
                    *itemsinclip = *itemsinclip + 1;
                  } else {
                    // file is already in the clipboard
                    // if it is set for copy, set it for cut
                    if(clipboard[clippos].action == 1) {
                      clipboard[clippos].action = 0;
                    }
                  }
                } else {
                  if(!inclip) {
                    if (!menu.items[ifile].isfolder) {
                      strcpy(clipboard[*itemsinclip].filename, files[ifile].filename);
                      //0=cut file; 1=copy file:
                      clipboard[*itemsinclip].action = 1;
                      clipboard[*itemsinclip].isfolder = 0;
                      *itemsinclip = *itemsinclip + 1;
                    } else {
                      if (!hasShownFolderCopyWarning) {
                        showCopyFolderWarning();
                        hasShownFolderCopyWarning = 1;
                      }
                    }
                  } else {
                    // file is already in the clipboard
                    // if it is set for cut and *is not a folder*, set it for copy
                    if(clipboard[clippos].action == 0 && !clipboard[clippos].isfolder) {
                      clipboard[clippos].action = 1;
                    } else {
                      if(!hasShownFolderCopyWarning && clipboard[clippos].isfolder) {
                        showCopyFolderWarning();
                        hasShownFolderCopyWarning = 1;
                      }
                    }
                  }
                }
                menu.items[ifile].isselected = 0; // clear selection
                menu.numselitems--;
              }
              ifile++;
            }
            if(*shownClipboardHelp == 0) {
              mMsgBoxPush(5);
              mPrintXY(3, 2, (char*)"Hint: press OPTN", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              mPrintXY(3, 3, (char*)"to manage the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              mPrintXY(3, 4, (char*)"clipboard.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              PrintXY_2(TEXT_MODE_NORMAL, 1, 6, 2, TEXT_COLOR_BLACK); // press exit message
              closeMsgBox();
              *shownClipboardHelp=1;
            }
          } else {
            mMsgBoxPush(4);
            mPrintXY(3, 2, (char*)"The clipboard is", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 3, (char*)"full; can't add", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            mPrintXY(3, 4, (char*)"more items to it.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
            closeMsgBox();
          }
        }
        break;
      case KEY_CTRL_F4:
        if(makeFolderGUI(browserbasepath)) return 1; // if user said yes and a folder was created, reload file list
        break;
      case KEY_CTRL_F5:
        if(menu.numitems>0) { if(renameFileGUI(files, &menu, browserbasepath)) return 1; }
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
  int key;
  while (1) {
    mPrintXY(3, 2, (char*)"Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, (char*)"Selected Items?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY_2(TEXT_MODE_NORMAL, 1, 4, 3, TEXT_COLOR_BLACK); // yes, F1
    PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 4, TEXT_COLOR_BLACK); // no, F6
    mGetKey(&key);
    if (key==KEY_CTRL_F1) {
      mMsgBoxPop();
      deleteFiles(files, menu);
      return 1;
    } else if (key == KEY_CTRL_F6 || key == KEY_CTRL_EXIT ) {
      mMsgBoxPop();
      return 0;
    }
  } 
}

int makeFolderGUI(char* browserbasepath) {
  //browserbasepath: folder we're working at
  //reload the files array after using this function!
  //returns 1 if user aborts, 0 if makes folder.
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  mPrintXY(1, 1, (char*)"Create folder", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  mPrintXY(1, 2, (char*)"Name:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
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
      char newfilename[MAX_FILENAME_SIZE] = "";
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
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  mPrintXY(1, 1, (char*)"Rename item", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  char title[MAX_NAME_SIZE+6] = "";
  strcpy(title, (char*)menu->items[menu->selection-1].text);
  strcat(title, " to:");
  mPrintXY(1, 2, (char*)title, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  char newname[MAX_NAME_SIZE] = "";
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
      char newfilename[MAX_FILENAME_SIZE] = "";
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

int fileInformation(char* filename, int allowEdit) {
  // returns 0 if user exits.
  // returns 1 if user wants to edit the file  
  textArea text;
  text.type=TEXTAREATYPE_INSTANT_RETURN;
  text.scrollbar=0;
  strcpy(text.title, (char*)"File information");
  
  textElement elem[15];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor
  
  elem[text.numelements].text = (char*)"File name:";
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  text.numelements++;
  
  elem[text.numelements].newLine=1;
  char name[MAX_NAME_SIZE] = "";
  nameFromFilename(filename, name);
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
  elem[text.numelements].text = (char*)filename;
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
  
  //Get file size
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  unsigned char sizebuffer[50] = "";
  int fsnotzero = 0;
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hFile >= 0) // Check if it opened
  { //opened
    unsigned int filesize = Bfile_GetFileSize_OS(hFile);
    Bfile_CloseFile_OS(hFile);
    if(filesize) fsnotzero = 1;
    itoa(filesize, (unsigned char*)sizebuffer);
    
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
  }
  
  doTextArea(&text);
  
  /*mPrintXY(1, 1, (char*)"File information", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  int textX=0, textY=24;
  PrintMini(&textX, &textY, (unsigned char*)"File name:", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
  char name[MAX_NAME_SIZE] = "";
  nameFromFilename(filename, name);
  textX=0; textY=textY+17;
  PrintMini(&textX, &textY, (unsigned char*)name, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textX=0; textY=textY+20;
  PrintMini(&textX, &textY, (unsigned char*)"Full file path:", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
  textX=0; textY=textY+18;
  PrintMiniMini( &textX, &textY, (unsigned char*)filename, 0, TEXT_COLOR_BLACK, 0 );
  
  // get file type description from OS
  unsigned int msgno;
  unsigned short iconbuffer[0x12*0x18];
  unsigned short folder[7]={};
  SMEM_MapIconToExt( (unsigned char*)name, folder, &msgno, iconbuffer );
  char mresult[88] = "";
  LocalizeMessage1( msgno, mresult );
  textX=0; textY=textY+15;
  PrintMini(&textX, &textY, (unsigned char*)"File type: ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)mresult, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  
  //Get file size
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hFile >= 0) // Check if it opened
  { //opened
    unsigned int filesize = Bfile_GetFileSize_OS(hFile);  
    unsigned char buffer[50] = "";
    itoa(filesize, (unsigned char*)buffer);
    textX=0; textY=textY+20;
    PrintMini(&textX, &textY, (unsigned char*)"File size: ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    PrintMini(&textX, &textY, (unsigned char*)" bytes", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    
    // if the file is not zero-byte, calculate the SHA-256 checksum:
    if(filesize > 0) {
      unsigned char output[32] = "";
      sha2_context ctx;
      unsigned char buf[4096];
      int readsize = 1; // not zero so we have the chance to enter the while loop
      sha2_starts( &ctx, 0 );
      while( readsize > 0) {
        readsize = Bfile_ReadFile_OS(hFile, buf, sizeof( buf ), -1);
        sha2_update( &ctx, buf, readsize );
      }
      sha2_finish( &ctx, output );
      memset( &ctx, 0, sizeof( sha2_context ) );
      
      unsigned char niceout[32] = "";
      textX=0; textY=textY+20;
      PrintMini(&textX, &textY, (unsigned char*)"SHA-256 checksum: ", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
      textY=textY+5;
      for(int i=0; i<32;i++) {
        strcpy((char*)niceout, (char*)"");
        ByteToHex( output[i], niceout );
        if((LCD_WIDTH_PX-textX) < 15) { textX=0; textY=textY+12; }
        PrintMiniMini( &textX, &textY, (unsigned char*)niceout, 0, TEXT_COLOR_BLACK, 0 );
      }
    }
    Bfile_CloseFile_OS(hFile);
  }  
  */
  int iresult;
  GetFKeyPtr(0x03B1, &iresult); // OPEN
  FKey_Display(0, (int*)iresult);
  if(allowEdit) {
    GetFKeyPtr(0x0185, &iresult); // EDIT
    FKey_Display(1, (int*)iresult);
  }
  if(fsnotzero) {
    GetFKeyPtr(0x0371, &iresult); // CALC (white)
    FKey_Display(5, (int*)iresult);
  }
  while (1) {
    int key;
    mGetKey(&key);
    switch(key) {
      case KEY_CTRL_EXIT:
      case KEY_CTRL_LEFT:
        return 0;
        break;
      case KEY_CTRL_F1:
        fileViewAsText(filename);
        return 0;
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
      case KEY_CTRL_F6:
        if(fsnotzero) {
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
  char name[MAX_NAME_SIZE] = "";
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
    if(filesize && filesize < MAX_TEXTVIEWER_FILESIZE) {
      asrc = (unsigned char*)alloca(filesize*sizeof(unsigned char));
      Bfile_ReadFile_OS(hFile, asrc, filesize, 0);
    } else {
      asrc = (unsigned char*)alloca(MAX_TEXTVIEWER_FILESIZE*sizeof(unsigned char));
      Bfile_ReadFile_OS(hFile, asrc, MAX_TEXTVIEWER_FILESIZE, 0);
    }
    Bfile_CloseFile_OS(hFile);
  } else {
    //Error opening file, abort
    mMsgBoxPush(4);
    mPrintXY(3, 2, (char*)"Error opening", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, (char*)"file to read.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
    closeMsgBox();
    return;
  }
  //linebreak detection is done outside of any loops for better speed:
  int newlinemode = 0;
  if(strstr((char*)asrc, "\r\n")) {
    newlinemode = 1; //Windows
  } else if (strstr((char*)asrc, "\r")) {
    newlinemode = 2; //Mac
  } else if (strstr((char*)asrc, "\n")) {
    newlinemode = 3; //Unix
  }
  char titlebuf[MAX_NAME_SIZE+20] ="";
  strcpy((char*)titlebuf, "Viewing ");
  strcat((char*)titlebuf, (char*)name);
  strcat((char*)titlebuf, " as text");
  DefineStatusMessage((char*)titlebuf, 1, 0, 0);
  DisplayStatusArea();
  textArea text;
  text.showtitle = 0;
  
  // get number of lines so we know how much textElement to allocate
  unsigned int numelements = 1; // at least one line it will have
  unsigned int bcur = 0;
  while(bcur < strlen((char*)asrc)) {
    if(*(asrc+bcur) == '\r' || *(asrc+bcur) == '\n') {
      if(*(asrc+bcur+1) != '\n') numelements++;
    }
    bcur++;
  }
  
  textElement* elem = (textElement*)alloca(numelements*sizeof(textElement));
  text.elements = elem;
  text.numelements = numelements;
  elem[0].text = (char*)asrc;
  elem[0].newLine=0;
  elem[0].color=COLOR_BLACK;
  elem[0].lineSpacing = 0;
  elem[0].spaceAtEnd = 0;
  elem[0].minimini = 0;
  unsigned int ecur = 1;
  bcur = 0;
  while(bcur < filesize && ecur <= numelements) {
    switch(newlinemode) {
      case 1: // Windows, \r\n
      default:
        if(*(asrc+bcur) == '\r') {
          //char after the next one (\n) will be the start of a new line
          //mark this char as null (so previous element ends here) and point this element to the start of the new line
          *(asrc+bcur)='\0';
          elem[ecur].text = (char*)(asrc+bcur+2);
          elem[ecur].newLine=1;
          elem[ecur].color=COLOR_BLACK;
          elem[ecur].lineSpacing = 0;
          elem[ecur].spaceAtEnd = 0;
          elem[ecur].minimini = 0;
          ecur++;
        }
        break;
      case 2: // Mac, \r
        if(*(asrc+bcur) == '\r') {
          *(asrc+bcur)='\0';
          elem[ecur].text = (char*)(asrc+bcur+1);
          elem[ecur].newLine=1;
          elem[ecur].color=COLOR_BLACK;
          elem[ecur].lineSpacing = 0;
          elem[ecur].spaceAtEnd = 0;
          elem[ecur].minimini = 0;
          ecur++;
        }
        break;
      case 3: // Unix, \n
        if(*(asrc+bcur) == '\n') {
          //src=asrc+bcur;
          *(asrc+bcur)='\0';
          if(asrc+bcur+1 >= asrc+filesize) {
            elem[ecur].text = (char*)"";
          } else {
            elem[ecur].text = (char*)(asrc+bcur+1);
          }
          elem[ecur].newLine=1;
          elem[ecur].color=COLOR_BLACK;
          elem[ecur].lineSpacing = 0;
          elem[ecur].spaceAtEnd = 0;
          elem[ecur].minimini = 0;
          ecur++;
        }
        break;
    }
    bcur++;
  }
  asrc[filesize] = '\0';
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
    menu.items = menuitems;
    int curitem = 0;
    while(curitem < *itemsinclip) {
      char buffer[MAX_FILENAME_SIZE] = "";
      nameFromFilename(clipboard[curitem].filename, buffer);
      strncpy(menuitems[curitem].text, (char*)buffer, 40);
      if(clipboard[curitem].action == 1) menuitems[curitem].color = TEXT_COLOR_RED;
      curitem++;
    }
    menu.numitems = *itemsinclip;
    clearLine(1, 8);
    int iresult;
    GetFKeyPtr(0x0149, &iresult); // CLEAR [white]
    FKey_Display(0, (int*)iresult);
    GetFKeyPtr(0x04DB, &iresult); // X symbol [white]
    FKey_Display(1, (int*)iresult);
    GetFKeyPtr(0x049D, &iresult); // Switch [white]
    FKey_Display(2, (int*)iresult);
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        if(!clipboard[menu.selection-1].isfolder) fileInformation(clipboard[menu.selection-1].filename, 0);
        break;
      case MENU_RETURN_EXIT:
        return;
        break;
      case KEY_CTRL_F1:
        *itemsinclip = 0;
        return;
        break;
      case KEY_CTRL_F2:
        {
          if (menu.selection-1 >= *itemsinclip) {} // safety check
          else
          {
            int k;
            for (k = menu.selection-1; k < *itemsinclip - 1; k++)
            {
              menuitems[k] = menuitems[k+1];
              clipboard[k] = clipboard[k+1];
            }
            *itemsinclip = *itemsinclip - 1;
          }
        }
        break;
      case KEY_CTRL_F3:
        if(clipboard[menu.selection-1].action == 1) {
          clipboard[menu.selection-1].action = 0;
          menuitems[menu.selection-1].color = TEXT_COLOR_BLACK;
        } else {
          if(!clipboard[menu.selection-1].isfolder) {
            clipboard[menu.selection-1].action = 1;
            menuitems[menu.selection-1].color = TEXT_COLOR_RED;
          } else showCopyFolderWarning();
        }
        break;
    }
  }
}

void showCopyFolderWarning() {
  mMsgBoxPush(4);
  mPrintXY(3, 2, (char*)"Copying folders", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 3, (char*)"not yet supported", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
  closeMsgBox();
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
  unsigned short iconbuffer[0x12*0x18];
  unsigned short folder[7]={'\\','\\','f','l','s','0',0};
  // get folder icon
  SMEM_MapIconToExt( (unsigned char*)"t", folder, &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_FOLDER].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.g3m", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_G3M].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.g3e", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_G3E].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.g3a", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_G3A].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.g3p", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_G3P].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.g3b", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_G3B].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.bmp", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_BMP].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.txt", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_TXT].data, iconbuffer, sizeof(iconbuffer));
  
  SMEM_MapIconToExt( (unsigned char*)"t.csv", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_CSV].data, iconbuffer, sizeof(iconbuffer));
  
  // get icon for unsupported files
  SMEM_MapIconToExt( (unsigned char*)"t.abc", (unsigned short*)"\x000\x000", &msgno, iconbuffer );
  memcpy(icontable[FILE_ICON_OTHER].data, iconbuffer, sizeof(iconbuffer));
}