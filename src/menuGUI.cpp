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
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "settingsProvider.hpp"

int doMenu(Menu* menu, MenuItemIcon* icontable) { // returns code telling what user did. selection is on menu->selection. menu->selection starts at 1!
  int itemsStartY=menu->startY; // char Y where to start drawing the menu items. Having a title increases this by one
  int itemsHeight=menu->height;
  if (menu->showtitle) {
    itemsStartY++;
    itemsHeight--;
  }

  if(menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
    menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
  if(menu->selection-1 < menu->scroll)
    menu->scroll = menu->selection -1;
  
  // prepare item background filler string according to menu width
  while(1) {
    if(menu->useStatusText) DefineStatusMessage((char*)menu->statusText, 1, 0, 0);
    // Clear the area of the screen we are going to draw on
    drawRectangle(18*(menu->startX-1), 24*(menu->miniMiniTitle ? itemsStartY:menu->startY), 18*menu->width, 24*menu->height-(menu->miniMiniTitle ? 24:0), COLOR_WHITE);
    if (menu->numitems>0) {
      for(int curitem=0; curitem < menu->numitems; curitem++) {
        // print the menu item only when appropriate
        if(menu->scroll < curitem+1 && menu->scroll > curitem-itemsHeight) {
          char menuitem[70] = "";
          if(menu->type == MENUTYPE_MULTISELECT) strcpy(menuitem, "  "); //allow for the folder and selection icons on MULTISELECT menus (e.g. file browser)
          strcat(menuitem, (char*)menu->items[curitem].text);
          if(menu->items[curitem].type != MENUITEM_SEPARATOR) {
            //make sure we have a string big enough to have background when item is selected:          
            // MB_ElementCount is used instead of strlen because multibyte chars count as two with strlen, while graphically they are just one char, making fillerRequired become wrong
            int fillerRequired = menu->width - MB_ElementCount(menu->items[curitem].text) - (menu->type == MENUTYPE_MULTISELECT ? 2 : 0);
            for(int i = 0; i < fillerRequired; i++) strcat(menuitem, " ");
            mPrintXY(menu->startX,curitem+itemsStartY-menu->scroll,(char*)menuitem, (menu->selection == curitem+1 ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL), menu->items[curitem].color);
          } else {
            int textX = (menu->startX-1) * 18;
            int textY = curitem*24+itemsStartY*24-menu->scroll*24-24+6;
            clearLine(menu->startX, curitem+itemsStartY-menu->scroll, (menu->selection == curitem+1 ? textColorToFullColor(menu->items[curitem].color) : COLOR_WHITE));
            drawLine(textX, textY+24-4, LCD_WIDTH_PX-2, textY+24-4, COLOR_GRAY);
            PrintMini(&textX, &textY, (unsigned char*)menuitem, 0, 0xFFFFFFFF, 0, 0, (menu->selection == curitem+1 ? COLOR_WHITE : textColorToFullColor(menu->items[curitem].color)), (menu->selection == curitem+1 ? textColorToFullColor(menu->items[curitem].color) : COLOR_WHITE), 1, 0);
          }
          // deal with menu items of type MENUITEM_CHECKBOX
          if(menu->items[curitem].type == MENUITEM_CHECKBOX) {
            mPrintXY(menu->startX+menu->width-1,curitem+itemsStartY-menu->scroll,
              (menu->items[curitem].value == MENUITEM_VALUE_CHECKED ? (char*)"\xe6\xa9" : (char*)"\xe6\xa5"),
              (menu->selection == curitem+1 ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL), menu->items[curitem].color);
          }
          // deal with multiselect menus
          if(menu->type == MENUTYPE_MULTISELECT) {
            if((curitem+itemsStartY-menu->scroll)>=itemsStartY &&
              (curitem+itemsStartY-menu->scroll)<=(itemsStartY+itemsHeight) &&
              icontable != NULL
            ) {
              if (menu->items[curitem].isfolder == 1) {
                // assumes first icon in icontable is the folder icon
                CopySpriteMasked(icontable[0].data, (menu->startX)*18, (curitem+itemsStartY-menu->scroll)*24, 0x12, 0x18, 0xf81f  );
              } else {
                if(menu->items[curitem].icon >= 0) CopySpriteMasked(icontable[menu->items[curitem].icon].data, (menu->startX)*18, (curitem+itemsStartY-menu->scroll)*24, 0x12, 0x18, 0xf81f  );
              }
            }
            if (menu->items[curitem].isselected) {
              if (menu->selection == curitem+1) {
                mPrintXY(menu->startX,curitem+itemsStartY-menu->scroll,(char*)"\xe6\x9b", TEXT_MODE_TRANSPARENT_BACKGROUND, (menu->items[curitem].color ==  TEXT_COLOR_GREEN ? TEXT_COLOR_BLUE : TEXT_COLOR_GREEN));
              } else {
                mPrintXY(menu->startX,curitem+itemsStartY-menu->scroll,(char*)"\xe6\x9b", TEXT_MODE_NORMAL, TEXT_COLOR_PURPLE);
              }
            }
          }
        }
      }
      if (menu->scrollbar) {
        TScrollbar sb;
        sb.I1 = 0;
        sb.I5 = 0;
        sb.indicatormaximum = menu->numitems;
        sb.indicatorheight = itemsHeight;
        sb.indicatorpos = menu->scroll;
        sb.barheight = itemsHeight*24;
        sb.bartop = (itemsStartY-1)*24;
        sb.barleft = menu->startX*18+menu->width*18 - 18 - (menu->scrollout ? 0 : 5);
        sb.barwidth = 6;
        Scrollbar(&sb);
      }
      if(menu->type==MENUTYPE_MULTISELECT && menu->fkeypage == 0) {
        int iresult;
        GetFKeyPtr(0x0037, &iresult); // SELECT (white)
        FKey_Display(0, (int*)iresult);
      }
    } else {
      printCentered((unsigned char*)menu->nodatamsg, (itemsStartY*24)+(itemsHeight*24)/2-12, COLOR_BLACK, COLOR_WHITE);
    }
    if(menu->showtitle) {
      if(menu->miniMiniTitle) {
        int textX = 0, textY=(menu->startY-1)*24;
        PrintMiniMini( &textX, &textY, (unsigned char*)menu->title, 16, menu->titleColor, 0 );
      } else mPrintXY(menu->startX, menu->startY, menu->title, TEXT_MODE_TRANSPARENT_BACKGROUND, menu->titleColor);
      if(menu->showsubtitle) {
        int textX=(MB_ElementCount(menu->title)+menu->startX-1)*18+10, textY=6;
        PrintMini(&textX, &textY, (unsigned char*)menu->subtitle, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      }
    }
    DisplayStatusArea();
    if(menu->darken) {
      if (GetSetting(SETTING_DISPLAY_STATUSBAR) == 1) darkenStatusbar();
      DrawFrame(COLOR_BLACK);
      VRAMInvertArea(menu->startX*18-18, menu->startY*24, menu->width*18-(menu->scrollout || !menu->scrollbar ? 0 : 5), menu->height*24);
    }
    if(menu->type == MENUTYPE_NO_KEY_HANDLING) return MENU_RETURN_INSTANT; // we don't want to handle keys
    int key;
    mGetKey(&key);
    switch(key) {
      case KEY_CTRL_DOWN:
        if(menu->selection == menu->numitems)
        {
          if(menu->returnOnInfiniteScrolling) {
            return MENU_RETURN_SCROLLING;
          } else {
            menu->selection = 1;
            menu->scroll = 0;
          }
        }
        else
        {
          menu->selection++;
          if(menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
            menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
        }
        if(menu->type == MENUTYPE_INSTANT_RETURN) return MENU_RETURN_INSTANT;
        break;
      case KEY_CTRL_UP:
        if(menu->selection == 1)
        {
          if(menu->returnOnInfiniteScrolling) {
            return MENU_RETURN_SCROLLING;
          } else {
            menu->selection = menu->numitems;
            menu->scroll = menu->selection-(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
          }
        }
        else
        {
          menu->selection--;
          if(menu->selection-1 < menu->scroll)
            menu->scroll = menu->selection -1;
        }
        if(menu->type == MENUTYPE_INSTANT_RETURN) return MENU_RETURN_INSTANT;
        break;
      case KEY_CTRL_F1:
        if(menu->type==MENUTYPE_MULTISELECT && menu->fkeypage == 0 && menu->numitems > 0) {
          if(menu->items[menu->selection-1].isselected) {
            menu->items[menu->selection-1].isselected=0;
            menu->numselitems = menu->numselitems-1;
          } else {
            menu->items[menu->selection-1].isselected=1;
            menu->numselitems = menu->numselitems+1;
          }
          return key; //return on F1 too so that parent subroutines have a chance to e.g. redraw fkeys
        } else if (menu->type == MENUTYPE_FKEYS) {
          return key;
        } else if(menu->type == MENUTYPE_INSTANT_RETURN) return MENU_RETURN_INSTANT;
        break;
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
      case KEY_CTRL_F6:
        if (menu->type == MENUTYPE_FKEYS || menu->type==MENUTYPE_MULTISELECT) return key; // MULTISELECT also returns on Fkeys
        break;
      case KEY_CTRL_PASTE:
        if (menu->type==MENUTYPE_MULTISELECT) return key; // MULTISELECT also returns on paste
      case KEY_CTRL_OPTN:
        if (menu->type==MENUTYPE_FKEYS || menu->type==MENUTYPE_MULTISELECT) return key;
        break;
      case KEY_CTRL_FORMAT:
        if (menu->type==MENUTYPE_FKEYS) return key; // return on the Format key so that event lists can prompt to change event category
        break;
      case KEY_CTRL_RIGHT:
        if(menu->type != MENUTYPE_MULTISELECT) break;
        // else fallthrough
      case KEY_CTRL_EXE:
        if(menu->numitems>0) { return MENU_RETURN_SELECTION; }
        break;
      case KEY_CTRL_LEFT:
        if(menu->type != MENUTYPE_MULTISELECT) break;
        // else fallthrough
      case KEY_CTRL_EXIT: return MENU_RETURN_EXIT;
        break;
      case KEY_CHAR_1:
      case KEY_CHAR_2:
      case KEY_CHAR_3:
      case KEY_CHAR_4:
      case KEY_CHAR_5:
      case KEY_CHAR_6:
      case KEY_CHAR_7:
      case KEY_CHAR_8:
      case KEY_CHAR_9:
        if(menu->numitems>=(key-0x30)) {menu->selection = (key-0x30); return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_0:
        if(menu->numitems>=10) {menu->selection = 10; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CTRL_XTT:
        if(menu->numitems>=11) {menu->selection = 11; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_LOG:
        if(menu->numitems>=12) {menu->selection = 12; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_LN:
        if(menu->numitems>=13) {menu->selection = 13; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_SIN:
        if(menu->numitems>=14) {menu->selection = 14; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_COS:
        if(menu->numitems>=15) {menu->selection = 15; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_TAN:
        if(menu->numitems>=16) {menu->selection = 16; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_FRAC:
        if(menu->numitems>=17) {menu->selection = 17; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CTRL_FD:
        if(menu->numitems>=18) {menu->selection = 18; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_LPAR:
        if(menu->numitems>=19) {menu->selection = 19; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_RPAR:
        if(menu->numitems>=20) {menu->selection = 20; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_COMMA:
        if(menu->numitems>=21) {menu->selection = 21; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_STORE:
        if(menu->numitems>=22) {menu->selection = 22; return MENU_RETURN_SELECTION; }
        break;
    }
  }
  return MENU_RETURN_SELECTION;
}

int getMenuSelectionIgnoringSeparators(Menu* menu) {
  if(menu->items[menu->selection-1].type == MENUITEM_SEPARATOR) return 0; //current selection is not a "traditional" menu item, return invalid position
  int selNS = 0;
  for(int i = 0; i<=menu->selection-1; i++) {
    if(menu->items[i].type != MENUITEM_SEPARATOR) selNS++;
  }
  return selNS;
}

int getMenuSelectionOnlySeparators(Menu* menu) {
  // exact opposite of function above
  if(menu->items[menu->selection-1].type != MENUITEM_SEPARATOR) return 0; //current selection is not a separator, return invalid position
  int selS = 0;
  for(int i = 0; i<=menu->selection-1; i++) {
    if(menu->items[i].type == MENUITEM_SEPARATOR) selS++;
  }
  return selS;
}

// not really related to the menu, but had to go somewhere as a general GUI helper:

void closeMsgBox() {
  // waits for user to exit a simple info box, and calls MsgBoxPop for you!
  int key,inscreen=1;
  while(inscreen) {
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_EXIT:
      case KEY_CTRL_AC:
        inscreen=0;
        break;
    }
  }
  mMsgBoxPop(); 
}