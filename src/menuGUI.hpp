#ifndef __MENUGUI_H
#define __MENUGUI_H

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

#define MENUITEM_NORMAL 0
#define MENUITEM_CHECKBOX 1
#define MENUITEM_SEPARATOR 2
#define MENUITEM_VALUE_NONE 0
#define MENUITEM_VALUE_CHECKED 1
typedef struct
{
  char text[42]; // text to be shown on screen. This shouldn't take more than 21 chars (20 in case of checkboxes) but I'm giving 42 because of multibyte codes...
  //char tag[30]; // internal var of the menu item, may be useful for some thing. NOTE: commented because it is yet to be needed, and so it was wasting memory.
  //void (*handler)(void); // routine to call when user performs an action on this menu item. for normal menuitems, this is when it is selected (press EXE). for checkboxes, this is when the checkbox is toggled.
  int type=MENUITEM_NORMAL; // type of the menu item. use MENUITEM_* to set this
  int value=MENUITEM_VALUE_NONE; // value of the menu item. For example, if type is MENUITEM_CHECKBOX and the checkbox is checked, the value of this var will be MENUITEM_VALUE_CHECKED
  int color=TEXT_COLOR_BLACK; // color of the menu item (use TEXT_COLOR_* to define)
  // The following two settings require the menu type to be set to MENUTYPE_MULTISELECT
  int isfolder=0; // for file browsers, this will signal the item is a folder
  int isselected=0; // for file browsers and other multi-select screens, this will show an arrow before the item
  int icon=-1; //for file browsers, to show a file icon. -1 shows no icon (default)
} MenuItem;

typedef struct
{
  unsigned short data[0x12*0x18];
} MenuItemIcon;

#define MENUTYPE_NORMAL 0
#define MENUTYPE_MULTISELECT 1
#define MENUTYPE_INSTANT_RETURN 2 // this type of menu insantly returns even if user hasn't selected an option (allows for e.g. redrawing the GUI behind it). if user hasn't exited or selected an option, menu will return MENU_RETURN_INSTANT
#define MENUTYPE_NO_KEY_HANDLING 3 //this type of menu doesn't handle any keys, only draws.
#define MENUTYPE_FKEYS 4 // returns GetKey value of a Fkey when one is pressed
typedef struct {
  char statusText[75] = ""; // text to be shown on the status bar, may be empty
  int showtitle=0; // whether to show a title as the first line
  char title[42] = ""; // title to be shown on the first line if showtitle is !=0
  char* subtitle;
  int showsubtitle=0;
  int titleColor=TEXT_COLOR_BLUE; //color of the title
  char nodatamsg[42] = ""; // message to show when there are no menu items to display
  int startX=1; //X where to start drawing the menu. NOTE this is not absolute pixel coordinates but rather character coordinates
  int startY=1; //Y where to start drawing the menu. NOTE this is not absolute pixel coordinates but rather character coordinates
  int width=21; // NOTE this is not absolute pixel coordinates but rather character coordinates
  int height=8; // NOTE this is not absolute pixel coordinates but rather character coordinates
  int scrollbar=1; // 1 to show scrollbar, 0 to not show it.
  int scrollout=0; // whether the scrollbar goes out of the menu area (1) or it overlaps some of the menu area (0)
  int numitems; // number of items in menu
  int type=MENUTYPE_NORMAL; // set to MENUTYPE_* .
  int selection=1; // currently selected item. starts counting at 1
  int scroll=0; // current scrolling position
  int allowMkey=1; // 1: allow for usage of mGetKey to retrieve keyboard input when user is in the selector
  int fkeypage=0; // for MULTISELECT menu if it should allow file selecting and show the fkey label
  int numselitems=0; // number of selected items
  int returnOnInfiniteScrolling=0; //whether the menu should return when user reaches the last item and presses the down key (or the first item and presses the up key)
  int darken=0; // for dark theme on homeGUI menus
  int miniMiniTitle=0; // if true, title will be drawn in minimini. for calendar week view
  int useStatusText=1;
  MenuItem* items; // items in menu
} Menu;

#define MENU_RETURN_EXIT 0
#define MENU_RETURN_SELECTION 1
#define MENU_RETURN_INSTANT 2
#define MENU_RETURN_SCROLLING 3 //for returnOnInfiniteScrolling
#define MENU_RETURN_INTOSETTINGS 4 //basically this is to request a redraw when the user accesses the settings. Restoring the VRAM may not work when the user accesses the main menu while the settings menu is open
int doMenu(Menu* menu, MenuItemIcon* icontable=NULL);
int getMenuSelectionIgnoringSeparators(Menu* menu);
int getMenuSelectionOnlySeparators(Menu* menu);

void closeMsgBox(int mgetkey=1);

#endif