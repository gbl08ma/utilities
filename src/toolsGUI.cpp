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

#include "toolsGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "selectorGUI.hpp" 
#include "fileProvider.hpp"

void memoryCapacityViewer() {
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Memory usage", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  int smemfree;
  unsigned char buffer[50] ="";
  unsigned short smemMedia[7]={'\\','\\','f','l','s','0',0};
  Bfile_GetMediaFree_OS( smemMedia, &smemfree );
  itoa(smemfree, buffer);
  unsigned char smemtext[70] = "";
  strcpy((char*)smemtext, "Storage: ");
  strcat((char*)smemtext, (char*)buffer);
  strcat((char*)smemtext, " bytes free");
  int textY = 24+6; int textX = 0;
  PrintMini(&textX, &textY, smemtext, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  strcpy((char*)smemtext, "out of ");
  itoa(TOTAL_SMEM, buffer);
  strcat((char*)smemtext, (char*)buffer);
  strcat((char*)smemtext, " bytes (");
  itoa(TOTAL_SMEM-smemfree, buffer);
  strcat((char*)smemtext, (char*)buffer);
  strcat((char*)smemtext, " bytes used)");
  textY = textY + 17; textX = 60;
  PrintMiniMini( &textX, &textY, smemtext, 0, TEXT_COLOR_BLACK, 0 );  
  
  
#ifdef DRAW_MEMUSAGE_GRAPHS
  textY = textY + 12;
  //what could be done in one line, has to be done in 3+another var, because of integer overflows -.-
  long long int tmpvar = TOTAL_SMEM-smemfree;
  tmpvar = LCD_WIDTH_PX*tmpvar;
  long long int barwidthcpl = tmpvar/TOTAL_SMEM;
  drawRectangle(0, textY+24, LCD_WIDTH_PX, 20, COLOR_GRAY);
  drawRectangle(0, textY+24, barwidthcpl, 20, COLOR_BLUE);
  
  int newTextX = 0;
  int newTextY = textY+5;
  itoa(100*(TOTAL_SMEM-smemfree)/TOTAL_SMEM, buffer);
  strcat((char*)buffer, "% used");
  PrintMiniMini( &newTextX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 1 ); //fake draw
  textX = LCD_WIDTH_PX/2 - newTextX/2;
  PrintMiniMini( &textX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 0 ); //draw  
  
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_WHITE, COLOR_GRAY);  
  VRAMReplaceColorInRect(0, textY+24, barwidthcpl, 20, COLOR_GRAY, COLOR_BLUE);
  
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_CYAN, COLOR_WHITE);
  textX = 0; textY = textY + 5;
#endif
  
  int MCSmaxspace; int MCScurrentload; int MCSfreespace;  
  MCS_GetState( &MCSmaxspace, &MCScurrentload, &MCSfreespace );
  barwidthcpl = (LCD_WIDTH_PX*(MCSmaxspace-MCSfreespace))/MCSmaxspace;
  itoa(MCSfreespace, buffer);
  unsigned char mcstext[70] = "";
  strcpy((char*)mcstext, "Main: ");
  strcat((char*)mcstext, (char*)buffer);
  strcat((char*)mcstext, " bytes free");  
  textY = textY + 17; textX = 0;
  PrintMini(&textX, &textY, mcstext, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  strcpy((char*)mcstext, "out of ");
  itoa(MCSmaxspace, buffer);
  strcat((char*)mcstext, (char*)buffer);
  strcat((char*)mcstext, " bytes (");
  itoa(MCScurrentload, buffer);
  strcat((char*)mcstext, (char*)buffer);
  strcat((char*)mcstext, " bytes used)");
  textY = textY + 17; textX = 60;
  PrintMiniMini( &textX, &textY, mcstext, 0, TEXT_COLOR_BLACK, 0 );
#ifdef DRAW_MEMUSAGE_GRAPHS
  textY = textY + 12;
  drawRectangle(0, textY+24, LCD_WIDTH_PX, 20, COLOR_GRAY);
  drawRectangle(0, textY+24, barwidthcpl, 20, COLOR_BLUE);

  newTextX = 0;
  newTextY = textY+5;
  itoa(100*MCScurrentload/MCSmaxspace, buffer);
  strcat((char*)buffer, "% used");
  PrintMiniMini( &newTextX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 1 ); //fake draw
  textX = LCD_WIDTH_PX/2 - newTextX/2;
  PrintMiniMini( &textX, &newTextY, (unsigned char*)buffer, 0, TEXT_COLOR_CYAN, 0 ); //draw  
  
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_WHITE, COLOR_GRAY);  
  VRAMReplaceColorInRect(0, textY+24, barwidthcpl, 20, COLOR_GRAY, COLOR_BLUE);
  VRAMReplaceColorInRect(0, textY+24, LCD_WIDTH_PX, 20, COLOR_CYAN, COLOR_WHITE);
#endif
  int key;
  mGetKey(&key);
}

int GetAddins(AddIn addins[]) {
  /*searches storage memory for active and inactive add-ins, returns their count*/
  unsigned short path[0x10A], path2[0x10A], found[0x10A];
  unsigned char buffer[0x10A], buffer2[0x10A];

  // make the buffer
  strcpy((char*)buffer, "\\\\fls0\\*");
  strcpy((char*)buffer2, "\\\\fls0\\*");
  
  int curitem = 0;
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, 0x10A);
  Bfile_StrToName_ncpy(path2, buffer2, 0x10A);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  Bfile_StrToName_ncpy(path, (unsigned char*)"*.g3a", 0x10A);
  Bfile_StrToName_ncpy(path2, (unsigned char*)"*.h3a", 0x10A);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, 0x10A);
    if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0 || strcmp((char*)buffer, "utilities.g3a") == 0) &&
        ((Bfile_Name_MatchMask((const short int*)path, (const short int*)found)) || (Bfile_Name_MatchMask((const short int*)path2, (const short int*)found))))
    {
      strcpy(addins[curitem].filename, (char*)buffer);
      //TODO: get friendly add-in name from system add-in table
      strcpy(addins[curitem].name, (char*)buffer);
      addins[curitem].name[strlen((char*)buffer)-4] = '\0';
      addins[curitem].active= (Bfile_Name_MatchMask((const short int*)path, (const short int*)found) ? 1 : 0);
      curitem++;
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }

  Bfile_FindClose(findhandle);
  
  return curitem;
}

void addinManager() {
  int res=1;
  Menu menu;
  
  menu.scrollbar=1;
  menu.scrollout=1;
  menu.showtitle=1;
  menu.selection=1;
  menu.scroll=0;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  strcpy(menu.nodatamsg, "No Add-Ins");
  strcpy(menu.title, "Add-In Manager");
  strcpy(menu.statusText, "");
  while(res) {
    res = addinManagerSub(&menu);
  }
}

int addinManagerSub(Menu* menu) {
  //returns 1 when it wants to be restarted (refresh addins)
  //returns 0 if the idea really is to exit the screen
  AddIn addins[200]; 
  
  menu->numitems = GetAddins(addins);
  MenuItem* menuitems = (MenuItem*)alloca(menu->numitems*sizeof(MenuItem));
  menu->items = menuitems;
 
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  int curaddin = 0; //current processing addin
  if (menu->numitems>0) {
    while(curaddin < menu->numitems) {
      strcpy(menuitems[curaddin].text, (char*)addins[curaddin].name);
      menuitems[curaddin].color = (addins[curaddin].active ? TEXT_COLOR_BLACK : TEXT_COLOR_CYAN);
      curaddin++;
    }
  }
  
  int iresult;  
  if(menu->numitems>0) {
    GetFKeyPtr(0x0103, &iresult); // CHANGE (white)
    FKey_Display(0, (int*)iresult);
    GetFKeyPtr(0x0038, &iresult); // DELETE
    FKey_Display(1, (int*)iresult);
  }
  GetFKeyPtr(0x03FD, &iresult); // HELP (white)
  FKey_Display(5, (int*)iresult);
  int res = doMenu(menu);
  
  unsigned short newpath[0x10A];
  char buffer[0x10A] = "";
  unsigned short oldpath[0x10A];
  switch(res)
  {
    case KEY_CTRL_F1:
      if(addins[menu->selection-1].active) { //disable
        strcpy(buffer, "\\\\fls0\\");
        strcat(buffer, addins[menu->selection-1].filename);
        Bfile_StrToName_ncpy(oldpath, (unsigned char*)buffer, 0x10A);
        buffer[strlen((char*)buffer)-3] = 'h'; //so it goes from g3a to h3a
        Bfile_StrToName_ncpy(newpath, (unsigned char*)buffer, 0x10A);
        Bfile_RenameEntry( oldpath , newpath );
      } else { //enable
        strcpy(buffer, "\\\\fls0\\");
        strcat(buffer, addins[menu->selection-1].filename);
        Bfile_StrToName_ncpy(oldpath, (unsigned char*)buffer, 0x10A);
        buffer[strlen((char*)buffer)-3] = 'g'; //so it goes from h3a to g3a
        Bfile_StrToName_ncpy(newpath, (unsigned char*)buffer, 0x10A);
        Bfile_RenameEntry( oldpath , newpath );
      }
      return 1; //reload list
      break;
    case KEY_CTRL_F2:
      MsgBoxPush(4);
      while (1) {
        int key;
        PrintXY(3, 2, (char*)"  Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY(3, 3, (char*)"  Selected Add-In?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY_2(TEXT_MODE_NORMAL, 1, 4, 3, TEXT_COLOR_BLACK); // yes, F1
        PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 4, TEXT_COLOR_BLACK); // no, F6
        mGetKey(&key);
        if (key==KEY_CTRL_F1) {
          MsgBoxPop();
          strcpy(buffer, "\\\\fls0\\");
          strcat(buffer, addins[menu->selection-1].filename);
          Bfile_StrToName_ncpy(oldpath, (unsigned char*)buffer, 0x10A);
          Bfile_DeleteEntry( oldpath );
          return 1;
        } else if (key == KEY_CTRL_F6 || key == KEY_CTRL_EXIT ) {
          MsgBoxPop();
          break;
        }
      }
      break;
    case KEY_CTRL_F6:
      if(1) {
        textArea text;
        strcpy(text.title, (char*)"Add-In Manager");
        
        textElement elem[10];
        text.elements = elem;
        
        elem[0].text = (char*)"This tool lets you hide add-ins from the Main Menu without the need for deleting them from your calculator. To hide an add-in, press [F1] when it is selected.";
        elem[0].spaceAtEnd=1;
        
        elem[1].text = (char*)"Hidden add-ins are shown in light blue. To make an hidden add-in appear back in the Main Menu, press [F1] again. This tool also lets you delete (uninstall) add-ins.";
        
        elem[2].newLine = 1;
        elem[2].lineSpacing = 8;
        elem[2].text = (char*)"To hide add-ins, this tool simply changes their file extension between g3a (normal, shown) and h3a (hidden). This means the hiding effect is permanent even if the Utilities add-in is uninstalled.";
        elem[2].spaceAtEnd=1;
        
        elem[3].text = (char*)"This add-in can't perform operations on itself. Add-ins are shown in the same order as they are in memory - they aren't ordered from A to Z.";
        elem[3].spaceAtEnd=1;
        
        elem[4].newLine = 1;
        elem[4].lineSpacing = 8;
        elem[4].text = (char*)"Enabling and disabling add-ins may change their position in memory, and consequently in the Main Menu and in this list. Use this to your advantage, but note that the order of the add-ins can't be directly controlled, at least not without removing all add-ins and putting them back one by one, through PC-USB.";
        elem[4].spaceAtEnd=1;
        
        text.numelements = 5;
        doTextArea(&text);
        
      }
      break;
    case MENU_RETURN_EXIT: return 0; break;
  }
  return 1;
}

void changeFKeyColor() {
  unsigned char*keycolor = (unsigned char*) 0xFD8013E4;
  unsigned char selcolor = (unsigned char) 0xFF; //just so it isn't uninitialized
  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Function key color", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  int textX=0; int textY=132;
  PrintMiniMini( &textX, &textY, (unsigned char*)"Please note that only the Utilities add-in and an hidden", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"debug screen on your calculator are able to change this", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"setting, which survives reboots. To reset it back to the", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"black color you need to use this add-in, the hidden debug", 0, TEXT_COLOR_BLACK, 0 );
  textY=textY+12; textX=0;
  PrintMiniMini( &textX, &textY, (unsigned char*)"screen or to reset the Main Memory.", 0, TEXT_COLOR_BLACK, 0 );
  Bdisp_PutDisp_DD();
  selcolor = ColorIndexDialog1( *keycolor, 0 );
  if(selcolor != (unsigned char)0xFF) {
    //user didn't press EXIT, QUIT or AC/ON. input is validated.
    *keycolor = selcolor;
  }
}