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
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "stringsProvider.hpp"
#include "menuGUI.hpp"
#include "inputGUI.hpp"
#include "fileProvider.hpp" 
#include "fileGUI.hpp"
#include "textGUI.hpp"
#include "sha1.h"
#include "sha2.h"
#include "editorGUI.hpp"
#include "imageGUI.hpp"
#include "linkProvider.hpp"
#include "sprites.h"

void fileManager() {
  int res = 1;
  int itemsinclip = 0;
  int shownMainMemHelp = 0;
  char browserbasepath[MAX_FILENAME_SIZE+1] = SMEM_PREFIX;
  File clipboard[MAX_ITEMS_IN_CLIPBOARD+1];
  char filetoedit[MAX_FILENAME_SIZE+1];
  strcpy(filetoedit, "");
  while(res) {
    int fileAction = 0;
    res = fileManagerChild(browserbasepath, &itemsinclip, &fileAction, &shownMainMemHelp, clipboard, filetoedit);
    switch(fileAction) {
      case 1:
        textfileEditor(filetoedit);
      break;
      #ifdef ENABLE_PICOC_SUPPORT
      case 2:
        int res = picoc(filetoedit);
        if(res) {
          char resStr[10];
          itoa(res, (unsigned char*)resStr);
          mMsgBoxPush(4);
          multiPrintXY(3, 2, "PicoC execution\nfinished with\ncode:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(9, 4, resStr, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          closeMsgBox();
        }
        break;
      #endif
    }
  }
}
int smemfree = 0;
void getFileManagerStatus(char* title, int itemsinclip, int ismanager) {
  char titleBuffer[120];
  if(!itemsinclip) {
    itoa(smemfree, (unsigned char*)title);
    LocalizeMessage1( 340, titleBuffer ); //"bytes free"
    strncat((char*)title, (char*)titleBuffer, 65);
  } else {
    strcpy(title, "Clipboard: ");
    itoa(itemsinclip, (unsigned char*)titleBuffer);
    strcat(title, titleBuffer);
    strcat(title, " item");
    if(itemsinclip != 1) strcat((char*)title, "s");
    if(!ismanager) {
      char message[50];
      stringToMini(message, ", SHIFT\xe6\x91""9=Paste");
      strcat((char*)title, message);
    }
  }
}

int fileManagerChild(char* browserbasepath, int* itemsinclip, int* fileAction, int* shownMainMemHelp, File* clipboard, char* filetoedit) {
  Menu menu;
  MenuItemIcon icontable[12];
  buildIconTable(icontable);
  
  // first get file count so we know how much to alloc
  int res = getFiles(NULL, browserbasepath, &menu.numitems);
  if(res == GETFILES_MAX_FILES_REACHED) {
    // show "folder has over 200 items, some will be skipped" message
    mMsgBoxPush(5);
    for(int i = 0; i < 4; i++) // folder has over / 200 files / some will / be skipped
      PrintXY_2(TEXT_MODE_NORMAL, 1, 2+i, 1833+i, TEXT_COLOR_BLACK);
    closeMsgBox(0, 6);
  }
  File* files = NULL;
  MenuItem* menuitems = NULL;
  if(menu.numitems > 0) {
    files = (File*)alloca(menu.numitems*sizeof(File));
    menuitems = (MenuItem*)alloca(menu.numitems*sizeof(MenuItem));
    // populate files array
    getFiles(files, browserbasepath, &menu.numitems);
  }
  int has_preset_selection = 0;
  for(int i = 0; i < menu.numitems; i++) {
      menuitems[i].isfolder = files[i].isfolder;
      if(menuitems[i].isfolder) menuitems[i].icon = FILE_ICON_FOLDER; // it would be a folder icon anyway, because isfolder is true
      else menuitems[i].icon = filenameToIcon(files[i].filename + files[i].visname);
      menuitems[i].isselected = 0; //clear selection. this means selection is cleared when changing directory (doesn't happen with native file manager)
      // because usually alloca is used to declare space for MenuItem*, the space is not cleared. which means we need to explicitly set each field:
      menuitems[i].text = files[i].filename + files[i].visname;
      menuitems[i].color=TEXT_COLOR_BLACK;
      menuitems[i].type=MENUITEM_NORMAL;
      menuitems[i].value=MENUITEM_VALUE_NONE;
      if(!has_preset_selection && filetoedit[0] && !strcmp(files[i].filename, filetoedit)) {
        menu.selection = i+1;
        has_preset_selection = 1;
      }
  }
  menu.items = menuitems;
  strcpy(filetoedit, "");
  
  unsigned short smemMedia[10]={'\\','\\','f','l','s','0',0};
  Bfile_GetMediaFree_OS( smemMedia, &smemfree );
  
  char friendlypath[MAX_FILENAME_SIZE];
  strcpy(friendlypath, browserbasepath+6);
  friendlypath[strlen(friendlypath)-1] = '\0'; //remove ending slash like OS does
  // test to see if friendlypath is too big
  int jump=1;
  while(1) {
    int temptextX=5*18+10; // px length of menu title + 10, like menuGUI goes.
    int temptextY=0;
    PrintMini(&temptextX, &temptextY, friendlypath, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); // fake draw
    if(temptextX>LCD_WIDTH_PX-6) {
      char newfriendlypath[MAX_FILENAME_SIZE];
      shortenDisplayPath(friendlypath, newfriendlypath, jump);
      if(strlen(friendlypath) > strlen(newfriendlypath) && strlen(newfriendlypath) > 2) { // check if len > 2 because shortenDisplayPath may return just the MB char with "..." when the folder name is too big
        // shortenDisplayPath still managed to shorten, copy and continue
        jump = 3; //it has been shortened already, so next time jump the first three characters
        strcpy(friendlypath, newfriendlypath);
      } else {
        // shortenDisplayPath can't shorten any more even if it still
        // doesn't fit in the screen, so give up.
        break;
      }
    } else break;
  }
  menu.subtitle = friendlypath;
  menu.type = MENUTYPE_MULTISELECT;
  menu.height = 7;
  menu.scrollout = 1;
  menu.returnOnLeft = 1;
  menu.returnOnRight = 1;
  menu.nodatamsg = (char*)"No Data";
  menu.title = (char*)"Files";
  char statusbuffer[120];
  while(1) {
    getFileManagerStatus((char*)statusbuffer, *itemsinclip, 0);
    menu.statusText = statusbuffer;
    int allcompressed = 1;
    if(menu.numselitems == 0) {
      drawFkeyLabels(0, // set by menu as SELECT [empty], otherwise make it white (we're not using Bdisp_AllClr_VRAM)
        0x03B6, // SEQ
        (menu.numitems>0 ? 0x0187 : 0), // SEARCH (only if there are items)
        0x0186, // NEW
        (menu.numitems>0 ? 0x0188 : 0), // RENAME or white
        0); // white
    } else {
      if(menu.numselitems) {
        for(int i = 0; i < menu.numitems; i++) {
          if(menu.items[i].isselected) {
            if(menu.items[i].isfolder) {
              allcompressed = 0;
              break; // no need to check more, if one of the items is a folder, then not all items are compressed.
            }
            int origfilesize;
            if(!isFileCompressed(files[i].filename, &origfilesize)) {
              allcompressed = 0;
              break; // no need to check more, at least one of the selected files is not compressed.
            }
          }
        }
      }
      drawFkeyLabels(0, // set by menu as SELECT [empty], otherwise make it white (we're not using Bdisp_AllClr_VRAM)
        0x0069, // CUT (white)
        0x0034, // COPY (white)
        (allcompressed ? 0x161 : 0x160), // Comp (white) / Dec (white)
        0x045B, // REVERSE (white)
        0x0038); // DELETE
    }
    if(has_preset_selection) {
      has_preset_selection = 0;
      res = MENU_RETURN_SELECTION;
    } else
      res = doMenu(&menu, icontable);
    switch(res) {
      case MENU_RETURN_EXIT:
        if(!strcmp(browserbasepath, SMEM_PREFIX)) { //check that we aren't already in the root folder
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
          if(!strcmp(browserbasepath, SMEM_PREFIX "@MainMem\\") && !*shownMainMemHelp) {
            mMsgBoxPush(5);
            multiPrintXY(3, 2, "Note: this is not\nthe Main Memory,\njust a special\nmirror of it.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            closeMsgBox(0, 6);
            *shownMainMemHelp=1;
          }
          return 1; //reload at new folder
        } else {
          int ininfo = 1;
          while(ininfo) {
            int res = viewFileInfo(&files[menu.selection-1], 1, *itemsinclip, 1);
            switch(res) {
              case VIEWFILEINFO_RETURN_EXIT:
                ininfo = 0;
                break;
              case VIEWFILEINFO_RETURN_EDIT:
                strcpy(filetoedit, files[menu.selection-1].filename);
                *fileAction = 1;
                // deliberate fallthrough
              case VIEWFILEINFO_RETURN_RELOAD:
                return 1;
              #ifdef ENABLE_PICOC_SUPPORT
              case VIEWFILEINFO_RETURN_EXECUTE:
                // user wants to run the file in picoc
                strcpy(filetoedit, files[menu.selection-1].filename);
                *fileAction = 2;
                return 1;
              #endif
              case VIEWFILEINFO_RETURN_UP:
                do {
                  menu.selection--;
                  if(menu.selection < 1) menu.selection = menu.numitems;
                } while(files[menu.selection-1].isfolder);
                break;
              case VIEWFILEINFO_RETURN_DOWN:
                do {
                  menu.selection++;
                  if(menu.selection > menu.numitems) menu.selection = 1;
                } while(files[menu.selection-1].isfolder);
            }
          }
        }
        break;
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
        if (menu.numselitems > 0) {
          int ifile = 0, hasErrored = 0;
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
                if(!hasErrored) { // if it's not yet full / we're yet to show the message
                  if(*itemsinclip < MAX_ITEMS_IN_CLIPBOARD) {
                    clipboard[*itemsinclip] = files[ifile];
                    //0=cut file; 1=copy file:
                    clipboard[*itemsinclip].action = (res == KEY_CTRL_F2 ? 0 : 1);
                    *itemsinclip = *itemsinclip + 1;
                  } else {
                    mMsgBoxPush(4);
                    multiPrintXY(3, 2, (char*)"The clipboard is\nfull; can't add\nmore items to it.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
                    closeMsgBox();
                    hasErrored = 1; // do not keep looping with the same message
                  }
                }
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
          if(res==KEY_CTRL_F2) {
            mMsgBoxPush(6);
            MenuItem smallmenuitems[7];
            smallmenuitems[0].text = (char*)"Do not sort";
            smallmenuitems[1].text = (char*)"Name (A to Z)";
            smallmenuitems[2].text = (char*)"Name (Z to A)";
            smallmenuitems[3].text = (char*)"Size (small 1st)";
            smallmenuitems[4].text = (char*)"Size (big 1st)";
            smallmenuitems[5].text = (char*)"Type (A to Z)";
            smallmenuitems[6].text = (char*)"Type (Z to A)";
            
            Menu smallmenu;
            smallmenu.items=smallmenuitems;
            smallmenu.numitems=7;
            smallmenu.width=17;
            smallmenu.height=6;
            smallmenu.startX=3;
            smallmenu.startY=2;
            smallmenu.scrollout=1;
            smallmenu.selection = getSetting(SETTING_FILE_MANAGER_SORT)+1;
            smallmenu.title = (char*)"Sort items by:";
            int sres = doMenu(&smallmenu);
            mMsgBoxPop();
            
            if(sres == MENU_RETURN_SELECTION) {
              setSetting(SETTING_FILE_MANAGER_SORT, smallmenu.selection-1, 1);
              if(menu.numitems > 1) {
                HourGlass(); //sorting can introduce a noticeable delay when there are many items
                sortFiles(files, menu.numitems);
                for(int i = 0; i < menu.numitems; i++) {
                    menuitems[i].isfolder = files[i].isfolder;
                    if(menuitems[i].isfolder) menuitems[i].icon = FILE_ICON_FOLDER;
                    else menuitems[i].icon = filenameToIcon(files[i].filename + files[i].visname);
                    menuitems[i].isselected = 0;
                    menuitems[i].text = files[i].filename + files[i].visname;
                }
              }
            }
          } else if(menu.numitems && searchFilesScreen(browserbasepath, *itemsinclip)) return 1;
        }
        break;
      case KEY_CTRL_F4: {
        if(menu.numselitems == 0) {
          mMsgBoxPush(4);
          MenuItem smallmenuitems[3];
          smallmenuitems[0].text = (char*)"Folder";
          smallmenuitems[1].text = (char*)"Text File";
          smallmenuitems[2].text = (char*)"Blank g3p file";
          
          Menu smallmenu;
          smallmenu.items=smallmenuitems;
          smallmenu.numitems=3;
          smallmenu.width=17;
          smallmenu.height=4;
          smallmenu.startX=3;
          smallmenu.startY=2;
          smallmenu.title = (char*)"Create new:";
          int sres = doMenu(&smallmenu);
          mMsgBoxPop();
          
          if(sres == MENU_RETURN_SELECTION) {
            if(smallmenu.selection == 1) {
              if(newFolderScreen(browserbasepath)) return 1; // if user said yes and a folder was created, reload file list
            } else if(smallmenu.selection == 2) {
              textfileEditor(NULL, browserbasepath); return 1;
            } else if(smallmenu.selection == 3) {
              if(newG3Pscreen(browserbasepath)) return 1;
            }
          }
        } else {
          if(allcompressed) decompressSelectedFiles(files, &menu);
          else compressSelectedFiles(files, &menu);
          return 1;
        }
        break;
      }
      case KEY_CTRL_F5:
        if(menu.numselitems == 0) {
          if(menu.numitems>0 && renameFileScreen(files, &menu, browserbasepath)) return 1;
        } else {
          for(int i = 0; i < menu.numitems; i++) {
            menu.items[i].isselected = !menu.items[i].isselected;
          }
          menu.numselitems = menu.numitems - menu.numselitems;
        }
        break;
      case KEY_CTRL_F6:
      case KEY_CTRL_DEL:
        if(menu.numselitems>0 && deleteFilesPrompt(files, &menu)) return 1; // if user said yes and files were deleted, reload file list
        break;
      case KEY_CTRL_PASTE:
        // clear shift icon and "Shift->9=Paste" part from status bar before pasting
        getFileManagerStatus((char*)statusbuffer, *itemsinclip, 1);
        DefineStatusMessage(statusbuffer, 1, 0, 0);
        DisplayStatusArea();
        pasteClipboard(clipboard, browserbasepath, *itemsinclip);
        *itemsinclip = 0;
        return 1;
        break;
      case KEY_CTRL_OPTN:
      {
        mMsgBoxPush(4);
        MenuItem smallmenuitems[2];
        smallmenuitems[0].text = (char*)"Folder statistics";
        smallmenuitems[1].text = (char*)"View clipboard";
        
        Menu smallmenu;
        smallmenu.items=smallmenuitems;
        smallmenu.numitems=2;
        smallmenu.width=17;
        smallmenu.height=4;
        smallmenu.startX=3;
        smallmenu.startY=2;
        int sres = doMenu(&smallmenu);
        mMsgBoxPop();
        
        if(sres == MENU_RETURN_SELECTION) {
          if(smallmenu.selection == 1) {
            folderStatsScreen(files, &menu);
          } else if(smallmenu.selection == 2) {
            viewClipboard(clipboard, itemsinclip);
          }
        }
        break;
      }
      case KEY_CTRL_CLIP:
        viewClipboard(clipboard, itemsinclip);
        break;
    }
  }
  return 1;
}

int deleteFilesPrompt(File* files, Menu* menu) {
  mMsgBoxPush(4);
  multiPrintXY(3, 2, "Delete the\nSelected Items?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  if(closeMsgBox(1, 4)) {
    deleteFiles(files, menu);
    return 1;
  }
  return 0;
}

int newFolderScreen(char* browserbasepath) {
  //browserbasepath: folder we're working at
  //reload the files array after using this function!
  //returns 1 if user creates folder, 0 if aborts.
  SetBackGround(10);
  clearLine(1,8);
  drawScreenTitle("Create folder", "Name:");
  char newname[MAX_NAME_SIZE];
  newname[0] = 0;
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
      Bfile_StrToName_ncpy(newfilenameshort, newfilename, 0x10A);
      Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FOLDER, 0); //create a folder
      return 1;
    }
  }
  return 0;
}

int newG3Pscreen(char* browserbasepath) {
  //browserbasepath: folder we're working at
  //reload the files array after using this function!
  //returns 1 if user creates file, 0 if aborts.
  SetBackGround(10);
  clearLine(1,8);
  drawScreenTitle("Create g3p file", "Name:");
  char newname[MAX_NAME_SIZE];
  newname[0] = 0;
  textInput input;
  input.forcetext=1;
  input.charlimit=MAX_NAME_SIZE;
  input.buffer = (char*)newname;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return 0; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      unsigned short tempfilenameshort[0x10A];
      Bfile_StrToName_ncpy(tempfilenameshort, TEMPFILE2, 0x10A);
      size_t filesize = Blank_g3p_phc_len;
      int h = Bfile_CreateEntry_OS(tempfilenameshort, CREATEMODE_FILE, &filesize);
      if(h < 0) return 0;
      h = Bfile_OpenFile_OS(tempfilenameshort, READWRITE, 0);
      if(h < 0) return 0;
      // file write buffer must be in RAM:
      char filebuf[Blank_g3p_phc_len+5] = "";
      memcpy(filebuf, Blank_g3p_phc, Blank_g3p_phc_len);

      Bfile_WriteFile_OS(h, filebuf, Blank_g3p_phc_len);
      Bfile_CloseFile_OS(h);

      char newfilename[MAX_FILENAME_SIZE];
      sprintf(newfilename, "%s%s.g3p", browserbasepath, newname);

      compressFile((char*)TEMPFILE2, (char*)newfilename, 1, 1); // will also delete temp file
      return 1;
    }
  }
  return 0;
}

int renameFileScreen(File* files, Menu* menu, char* browserbasepath) {
  //reload the files array after using this function!
  //returns 0 if user aborts, 1 if renames.
  SetBackGround(6);
  clearLine(1,8);
  char title[MAX_NAME_SIZE+6];
  strcpy(title, menu->items[menu->selection-1].text);
  strcat(title, " to:");
  drawScreenTitle("Rename item", title);
  char newname[MAX_NAME_SIZE];
  strcpy(newname, menu->items[menu->selection-1].text);
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
      renameFile(files[menu->selection-1].filename, newfilename);
      return 1;
    }
  }
  return 0;
}

int searchFilesScreen(char* browserbasepath, int itemsinclip) {
  // returns 1 when it wants the caller to jump to browserbasepath
  // returns 0 otherwise.
  char statusText[120];
  getFileManagerStatus((char*)statusText, itemsinclip, 1);
  DefineStatusMessage((char*)statusText, 1, 0, 0);
  
  int searchOnFilename = !!(getSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 0));
  int searchOnContents = !!(getSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 1));
  int searchRecursively = !!(getSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 2));
  int matchCase = !!(getSetting(SETTING_FILE_MANAGER_SEARCH) & (1U << 3));
  
  char needle[55] = "";
  textInput input;
  input.forcetext=1; //force text so needle must be at least one char.
  input.charlimit=50;
  input.acceptF6=1;
  input.buffer = needle;
  MenuItem menuitems[4];
  menuitems[0].text = (char*)"On filename";
  menuitems[0].type = MENUITEM_CHECKBOX;
  menuitems[1].text = (char*)"On contents";
  menuitems[1].type = MENUITEM_CHECKBOX;
  menuitems[2].text = (char*)"Recursively";
  menuitems[2].type = MENUITEM_CHECKBOX;
  menuitems[3].text = (char*)"Match case";
  menuitems[3].type = MENUITEM_CHECKBOX;

  Menu menu;
  menu.items=menuitems;
  menu.type=MENUTYPE_FKEYS;
  menu.numitems=4;
  menu.height=4;
  menu.startY=4;
  menu.scrollout=1;
  menu.pBaRtR=1;
  int curstep = 0;
  while(1) {
    if(curstep == 0) {
      SetBackGround(9);
      drawScreenTitle("File Search", "Search for:");
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
        drawScreenTitle("File Search", "Search for:");
        mPrintXY(1, 3, needle, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
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
  newsetting |= ((unsigned)searchOnFilename << 0);
  newsetting |= ((unsigned)searchOnContents << 1);
  newsetting |= ((unsigned)searchRecursively << 2);
  newsetting |= ((unsigned)matchCase << 3);
  setSetting(SETTING_FILE_MANAGER_SEARCH, newsetting, 1); // remember search preferences

  SetBackGround(9);
  drawScreenTitle("File Search", "Searching...");
  multiPrintXY(1, 3, "Please be patient.\n\nYou can press AC/ON\nto abort.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  clearLine(1, 8);
  Bdisp_PutDisp_DD();

  int sres = searchForFiles(NULL, browserbasepath, needle, searchOnFilename, searchOnContents, searchRecursively, matchCase, &menu.numitems);
  if(sres == GETFILES_USER_ABORTED) {
    int bkey; GetKey(&bkey); // key debouncing to avoid getting "Break" message closed because AC is still pressed
    AUX_DisplayErrorMessage( 0x01 );
    return 0;
  }
  MenuItem* resitems = (MenuItem*)alloca(menu.numitems*sizeof(MenuItem));
  File* files = (File*)alloca(menu.numitems*sizeof(File));
  if(menu.numitems) {
    sres = searchForFiles(files, browserbasepath, needle, searchOnFilename, searchOnContents, searchRecursively, matchCase, &menu.numitems);
    if(sres == GETFILES_USER_ABORTED) {
      int bkey; GetKey(&bkey);
      AUX_DisplayErrorMessage( 0x01 );
      return 0;
    }
  }
  int curitem = 0;
  while(curitem < menu.numitems) {
    resitems[curitem].text = files[curitem].filename + files[curitem].visname;
    resitems[curitem].type = MENUITEM_NORMAL;
    resitems[curitem].color = COLOR_BLACK;
    curitem++;
  }
  
  menu.height=7;
  menu.selection = 1;
  menu.startY=1;
  menu.pBaRtR=0;
  menu.returnOnRight = 1;
  menu.nodatamsg = (char*)"No files found";
  menu.title = (char*)"Search results";
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
          } else viewFileInfo(&files[menu.selection-1], 0, itemsinclip);
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

int viewFileInfo(File* file, int allowEdit, int itemsinclip, int allowUpDown) {
  // returns 0 if user exits.
  // returns 1 if user wants to edit the file
  // returns 2 if user compressed or decompressed the file

  int compressed = 0, origfilesize = 0;
  if(isFileCompressed(file->filename, &origfilesize)) compressed = 1;

  textArea text;
  text.type=TEXTAREATYPE_INSTANT_RETURN;
  text.scrollbar=0;
  text.title = (char*)"File information";
  
  textElement elem[15];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor
  
  elem[text.numelements].text = (char*)"File name:";
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  text.numelements++;
  
  elem[text.numelements].newLine=1;
  elem[text.numelements].text = file->filename + file->visname;
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
  SMEM_MapIconToExt((unsigned char*)(file->filename + file->visname), folder, &msgno, iconbuffer );
  char mresult[88];
  LocalizeMessage1( msgno, mresult );
  
  if(compressed) elem[text.numelements].text = (char*)ADDIN_FRIENDLYNAME " Compressed File";
  else elem[text.numelements].text = (char*)mresult;
  text.numelements++;
  char sizebuffer[50];
  
  elem[text.numelements].newLine=1;
  elem[text.numelements].lineSpacing=3;
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].text = (char*)"File size:";
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  sprintf(sizebuffer, "%d bytes", file->size);
  elem[text.numelements].text = (char*)sizebuffer;
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  char osizebuffer[50];
  if(compressed) {
    sprintf(osizebuffer, "(%d decompressed)", origfilesize);
    elem[text.numelements].text = osizebuffer;
    elem[text.numelements].minimini=1;
    text.numelements++;
  }
  
  while (1) {
    char statusText[120];
    getFileManagerStatus((char*)statusText, itemsinclip, 1);
    DefineStatusMessage((char*)statusText, 1, 0, 0);
    doTextArea(&text);
    drawFkeyLabels((compressed? -1 : 0x03B1), (allowEdit && !compressed? 0x0185: -1), (allowEdit? (compressed ? 0x161 : 0x160) : -1), -1, -1, (file->size>0 ? 0x0371 : -1)); //OPEN, EDIT, Comp (white) or Dec (white), CALC (white)
    #ifdef ENABLE_PICOC_SUPPORT
    if(allowEdit && strEndsWith(file->filename, (char*)".c")) drawFkeyLabels(-1, -1, -1, 0x0184); // EXE (white)
    #endif
    int key;
    mGetKey(&key);
    switch(key) {
      case KEY_CTRL_EXIT:
      case KEY_CTRL_LEFT:
        return 0;
      case KEY_CTRL_UP:
        if(allowUpDown) return VIEWFILEINFO_RETURN_UP;
        break;
      case KEY_CTRL_DOWN:
        if(allowUpDown) return VIEWFILEINFO_RETURN_DOWN;
        break;
      case KEY_CTRL_EXE:
      case KEY_CTRL_RIGHT:
        #ifdef ENABLE_PICOC_SUPPORT
        if(allowEdit && strEndsWith(file->filename, (char*)".c")) return 3;
        #endif
        // else deliberate fallthrough
      case KEY_CTRL_F1:
        if(!compressed) {
          if(stringEndsInJPG(file->filename)) viewImage(file->filename);
          else viewFileAsText(file->filename);
        }
        break;
      case KEY_CTRL_F2:
        if(allowEdit && !compressed) {
          if(stringEndsInG3A(file->filename + file->visname)) {
            mMsgBoxPush(4);
            multiPrintXY(3, 2, "g3a files can't\nbe edited by\nan add-in.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
            closeMsgBox();
          } else {
            return 1;
          }
        }
        break;
      case KEY_CTRL_F3:
        if(allowEdit) {
          if(compressed) {
            char newfilename[MAX_FILENAME_SIZE];
            int len=strlen(file->filename);
            strncpy(newfilename, file->filename, len-4); //strip file extension
            newfilename[len-4] = '\0'; // strncpy does not zero-terminate when limit is reached
            compressFile(file->filename, newfilename, 1);
          } else {
            char newfilename[MAX_FILENAME_SIZE];
            strcpy(newfilename, file->filename);
            strcat(newfilename, (char*)COMPRESSED_FILE_EXTENSION);
            compressFile(file->filename, newfilename, 0);
          }
          return 2;
        }
        break;
      #ifdef DISABLED_EXPERIMENTAL_CODE
      case KEY_CTRL_F4:
        serialTransferSingleFile(file->filename);
        break;
      #else
      # ifdef ENABLE_PICOC_SUPPORT
      case KEY_CTRL_F4:
        if(allowEdit && strEndsWith(file->filename, (char*)".c"))
          return 3;
        break;
      # endif
      #endif
      case KEY_CTRL_F6:
        if(file->size > 0) {
          int hFile = fileOpen(file->filename); // Get handle
          if(hFile >= 0) // Check if it opened
          { //opened
            unsigned char output1[20] = "";
            unsigned char output2[32] = "";
            sha1_context ctx1;
            sha2_context ctx2;
            unsigned char buf[4096];
            sha1_starts(&ctx1);
            sha2_starts(&ctx2, 0);
            int readsize;
            do {
              readsize = Bfile_ReadFile_OS(hFile, buf, sizeof( buf ), -1);
              sha1_update(&ctx1, buf, readsize);
              sha2_update(&ctx2, buf, readsize);
            } while(readsize > 0);
            Bfile_CloseFile_OS(hFile);
            sha1_finish(&ctx1, output1);
            sha2_finish(&ctx2, output2);
            memset(&ctx1, 0, sizeof(sha1_context));
            memset(&ctx2, 0, sizeof(sha2_context));
            
            mMsgBoxPush(5);
            int textX=2*18, textY=24;
            PrintMini(&textX, &textY, (char*)"SHA-256 checksum:", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
            textY += 20;
            textX=2*18;
            for(int i=0; i<32;i++) {
              unsigned char niceout[32] = "";
              ByteToHex( output2[i], niceout );
              if((LCD_WIDTH_PX-2*18-textX) < 15) { textX=2*18; textY=textY+12; }
              PrintMiniMini( &textX, &textY, (char*)niceout, 0, TEXT_COLOR_BLACK, 0 );
            }

            textX=2*18; textY=24*3;
            PrintMini(&textX, &textY, (char*)"SHA-1 checksum:", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
            textY += 20;
            textX = 2*18;
            for(int i=0; i<20;i++) {
              unsigned char niceout[32] = "";
              ByteToHex( output1[i], niceout );
              if((LCD_WIDTH_PX-2*18-textX) < 15) { textX=2*18; textY=textY+12; }
              PrintMiniMini( &textX, &textY, (char*)niceout, 0, TEXT_COLOR_BLACK, 0 );
            }
            closeMsgBox(0, 6);
          } else AUX_DisplayErrorMessage(0x2B); // Data ERROR
        }
        break;
    }
  }
}

void viewFileAsText(char* filename) {
  unsigned char* asrc = NULL;
  //Get file contents
  int hFile = fileOpen(filename); // Get handle
  unsigned int filesize = 0;
  if(hFile >= 0) // Check if it opened
  { //opened
    filesize = Bfile_GetFileSize_OS(hFile);
    if(!filesize) {
      Bfile_CloseFile_OS(hFile);
      return;
    }
    if(filesize > MAX_TEXTVIEWER_FILESIZE) filesize = MAX_TEXTVIEWER_FILESIZE;
    // check if there's enough stack to proceed:
    if(0x881E0000 - (int)GetStackPtr() < 500000 - filesize*sizeof(unsigned char) - 30000) {
      asrc = (unsigned char*)alloca(filesize*sizeof(unsigned char)+5);
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
    AUX_DisplayErrorMessage(0x2B); // Data ERROR
    return;
  }
  char titlebuf[MAX_NAME_SIZE+20];
  sprintf(titlebuf, "Viewing %s as text", filenameToName(filename));
  DefineStatusMessage((char*)titlebuf, 1, 0, 0);
  textArea text;
  text.allowLeft = 1;
  
  // get number of lines so we know how much textElement to allocate
  text.numelements = 1; // at least one line it will have
  unsigned int bcur = 0;
  unsigned int len = strlen((char*)asrc);
  while(bcur < len) {
    int jump = 1;
    if(*(asrc+bcur) == '\r' && *(asrc+bcur+1) == '\n') {
      jump=2;
      text.numelements++;
    } else if(*(asrc+bcur) == '\r' || *(asrc+bcur) == '\n') text.numelements++;
    bcur += jump;
  }
  if(text.numelements > MAX_TEXTVIEWER_LINES) text.numelements = MAX_TEXTVIEWER_LINES;

  textElement elem[text.numelements];
  text.elements = elem;
  elem[0].text = (char*)asrc;
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
      ecur++;
    }
    bcur += jump;
  }
  doTextArea(&text);
}

void viewClipboard(File* clipboard, int* itemsinclip) {
  Menu menu;
  menu.height = 7;
  menu.scrollout = 1;
  menu.type=MENUTYPE_FKEYS;
  menu.returnOnRight = 1;
  while(1) {
    MenuItem menuitems[MAX_ITEMS_IN_CLIPBOARD];
    menu.title = (char*)"Clipboard";
    menu.subtitle = (char*)"Black=cut, Red=copy";
    menu.nodatamsg = (char*)"Clipboard empty";
    char statusbuffer[120];
    getFileManagerStatus(statusbuffer, *itemsinclip, 1);
    menu.statusText = statusbuffer;
    menu.items = menuitems;
    int curitem = 0;
    while(curitem < *itemsinclip) {
      menuitems[curitem].text = clipboard[curitem].filename + clipboard[curitem].visname;
      if(clipboard[curitem].action == 1) menuitems[curitem].color = TEXT_COLOR_RED;
      curitem++;
    }
    menu.numitems = *itemsinclip;
    if(*itemsinclip > 0) drawFkeyLabels(0x0149, 0x04DB, 0x049D, 0, 0, 0); // CLEAR [white], X symbol [white], Switch [white], clear rest of line
    else clearLine(1, 8);
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_SELECTION:
        if(!clipboard[menu.selection-1].isfolder) viewFileInfo(&clipboard[menu.selection-1], 0, *itemsinclip);
        break;
      case MENU_RETURN_EXIT:
        return;
      case KEY_CTRL_F1:
        if(*itemsinclip > 0) *itemsinclip = 0;
        break;
      case KEY_CTRL_F2:
        if (menu.selection-1 >= *itemsinclip || *itemsinclip==0) {} // safety check
        else {
          for (int k = menu.selection-1; k < *itemsinclip - 1; k++) {
            menuitems[k] = menuitems[k+1];
            clipboard[k] = clipboard[k+1];
          }
          *itemsinclip = *itemsinclip - 1;
        }
        break;
      case KEY_CTRL_F3:
        if(*itemsinclip > 0) {
          clipboard[menu.selection-1].action = !clipboard[menu.selection-1].action;
          if(clipboard[menu.selection-1].action) menuitems[menu.selection-1].color = TEXT_COLOR_BLACK;
          else menuitems[menu.selection-1].color = TEXT_COLOR_RED;
        }
        break;
    }
  }
}

void folderStatsScreen(File* files, Menu* menu) {
  textArea text;
  text.type = TEXTAREATYPE_NORMAL;
  text.scrollbar=0;
  text.title = (char*)"Folder statistics";
  
  textElement elem[7];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor

  
  int folders_count = 0, sel_folders_count = 0,
      sel_size = 0, total_size = 0;
  for(int i = 0; i < menu->numitems; i++) {
    if(files[i].isfolder) {
      folders_count++;
      if(menu->items[i].isselected) sel_folders_count++;
    }
    else if(menu->items[i].isselected) {
      sel_size += files[i].size;
    }
    total_size += files[i].size;
  }

  char ficbuffer[40];
  sprintf(ficbuffer, "%d files\n%d folders", menu->numitems - folders_count, folders_count);
  elem[text.numelements].text = (char*)ficbuffer;
  text.numelements++;

  char tsibuffer[20];
  elem[text.numelements].newLine=1;
  elem[text.numelements].lineSpacing=3;
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].text = (char*)"Size of files:";
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;

  sprintf(tsibuffer, "%d bytes", total_size);
  elem[text.numelements].text = (char*)tsibuffer;
  text.numelements++;

  char tssbuffer[20];
  char sficbuffer[40];
  if(menu->numselitems) {
    elem[text.numelements].newLine=1;
    elem[text.numelements].lineSpacing=7;
    elem[text.numelements].color=COLOR_BLUE;
    elem[text.numelements].text = (char*)"Selected items";
    text.numelements++;

    sprintf(sficbuffer, "%d files\n%d folders", menu->numselitems - sel_folders_count, sel_folders_count);
    elem[text.numelements].newLine=1;
    elem[text.numelements].text = (char*)sficbuffer;
    text.numelements++;

    elem[text.numelements].newLine=1;
    elem[text.numelements].lineSpacing=3;
    elem[text.numelements].color=COLOR_LIGHTGRAY;
    elem[text.numelements].text = (char*)"Size of files:";
    elem[text.numelements].spaceAtEnd=1;
    text.numelements++;

    sprintf(tssbuffer, "%d bytes", sel_size);
    elem[text.numelements].text = (char*)tssbuffer;
    text.numelements++;
  }
  
  doTextArea(&text);
}

void compressSelectedFiles(File* files, Menu* menu) {
  int tf = 0, cf = 0, origfilesize = 0;
  for(int i = 0; i < menu->numitems; i++)
    if(!files[i].isfolder && menu->items[i].isselected && !isFileCompressed(files[i].filename, &origfilesize))
      tf++;

  progressMessage((char*)" Compressing...", 0, tf);
  for(int i = 0; i < menu->numitems; i++) {
    if(!files[i].isfolder && menu->items[i].isselected) {
      char newfilename[MAX_FILENAME_SIZE];
      strcpy(newfilename, files[i].filename);
      strcat(newfilename, (char*)COMPRESSED_FILE_EXTENSION);
      if(!isFileCompressed(files[i].filename, &origfilesize)) {
        compressFile((char*)files[i].filename, (char*)newfilename, 0, 1);
        cf++;
        progressMessage((char*)" Compressing...", cf, tf);
      }
    }
  }
  closeProgressMessage();
}

void decompressSelectedFiles(File* files, Menu* menu) {
  // assumes all selected files in the menu are compressed!
  int tf = 0, cf = 0;
  for(int i = 0; i < menu->numitems; i++) {
    if(!files[i].isfolder && menu->items[i].isselected) {
      tf++;
    }
  }
  progressMessage((char*)"Decompressing...", 0, tf);
  for(int i = 0; i < menu->numitems; i++) {
    if(!files[i].isfolder && menu->items[i].isselected) {
      char newfilename[MAX_FILENAME_SIZE];
      int len=strlen(files[i].filename);
      strncpy(newfilename, files[i].filename, len-4); //strip file extension
      newfilename[len-4] = '\0'; // strncpy does not zero-terminate when limit is reached
      compressFile((char*)files[i].filename, (char*)newfilename, 1, 1);
      cf++;
      progressMessage((char*)"Decompressing...", cf, tf);
    }
  }
  closeProgressMessage();
}

void shortenDisplayPath(char* longpath, char* shortpath, int jump) {
  //this function takes a long path for display, like \myfolder\long\display\path
  //and shortens it one level, like this: ...\long\display\path
  //putting the result in shortpath
  // jump: amount of characters to jump when parsing the string. useful for jumping the first / or .../
  strcpy(shortpath, (char*)"\xe5\x94");
  int max = strlen(longpath);
  while (jump < max && longpath[jump] != '\\')
          jump++;
  if (longpath[jump] == '\\') {
    strcat(shortpath, longpath+jump);
  }
}

void buildIconTable(MenuItemIcon* icontable) {
  unsigned int msgno;
  unsigned short folder[7]={'\\','\\','f','l','s','0',0};

  static const char *bogusFiles[] = {"t", // for folder
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

int overwriteFilePrompt(char* filename) {
  char cut[MAX_NAME_SIZE];
  char* visname = filenameToName(filename);
  strncpy(cut, visname, 17);
  cut[17] = 0;
  return (0x7539 == OverwriteConfirmation(cut, 0));
}