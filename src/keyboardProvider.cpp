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
extern "C" {
#include <setjmp.h>
}

#include "keyboardProvider.hpp" 
#include "settingsGUI.hpp" 
#include "debugGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"
#include "graphicsProvider.hpp"
#include "settingsProvider.hpp"

extern jmp_buf utilities_return;
static int mGetKeyMode = MGETKEY_MODE_NORMAL;

void saveVRAMandCallSettings() {
  int stackused = 0x881E0000 - (int)GetStackPtr();
  // if(stackused > 300000) then there's not enough free stack for us to save the screen
  // and still keep enough stack to call settings safely, so ask the system to save instead
  // (it will be discarded if the user accesses the menu)
  // if we have enough stack, we save it ourselves so that the screen can still be restored
  // even if the user accesses the Main Menu while in the settings.
  char* vrambackup = NULL;
  if(stackused > 300000) SaveVRAM_1();
  else {
    // no need to save status bar, because it's easy to redraw
    vrambackup = (char*)alloca((384*(216-24)*2)*sizeof(char)); // 2 bytes per pixel
    // MsgBoxMoveWB automatically accounts for a 24 pixels offset at the top
    MsgBoxMoveWB(vrambackup, 0, 0, LCD_WIDTH_PX-1, LCD_HEIGHT_PX-24-1, 1);
  }
  settingsMenu();
  if(stackused > 300000) LoadVRAM_1();
  else if(vrambackup!=NULL) MsgBoxMoveWB(vrambackup, 0, 0, LCD_WIDTH_PX-1, LCD_HEIGHT_PX-24-1, 0);
}
void mGetKey(int* key, int darkenStatus) {
  //managed GetKey. allows for entering the settings menu from most points in the add-in.
  while (1) {
    DisplayStatusArea(); // it still won't show if it is disabled
    if(getSetting(SETTING_DISPLAY_STATUSBAR) && darkenStatus) darkenStatusbar();
    checkChronoComplete(getChronoArrayPtr());
    GetKey(key);
    if (*key == KEY_CTRL_SETUP && mGetKeyMode != MGETKEY_MODE_RESTRICT_SETTINGS && mGetKeyMode != MGETKEY_MODE_RESTRICT_SETTINGS_RESTART) {
      Cursor_SetFlashOff(); // in case we were in an input
      saveVRAMandCallSettings();
      //redraw things like menu status text
      *key = 0;
      return;
    } else if (*key == KEY_CTRL_QUIT && mGetKeyMode != MGETKEY_MODE_RESTRICT_RESTART && mGetKeyMode != MGETKEY_MODE_RESTRICT_SETTINGS_RESTART) {
      Cursor_SetFlashOff(); // in case we were in an input
      stopAndUninstallStubTimer(); // in case we were in some timer screen, where the timer has been set
      // having timers running breaks Bfile functions
      popAllMsgBoxes();
      mGetKeyMode = MGETKEY_MODE_NORMAL;
      longjmp(utilities_return, 1); // this is also used for returning from Run-Mat. Basically equates to restarting the add-in.
      break;
#ifdef ENABLE_DEBUG
    } else if (*key == KEY_SHIFT_OPTN && GetDebugMode()) {
      TestMode(1);
      const unsigned int default_fkeys[] = { 0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0,0x0000FFFF,0 };
      Set_FKeys1(0, (unsigned int*)default_fkeys);
      Set_FKeys2(0);
    } else if (*key == KEY_CTRL_PRGM && GetDebugMode()) {
      showRAMused();
#endif
    } else {
      break; 
    }
  }
  return;
}

void setmGetKeyMode(int mode) {
  mGetKeyMode = mode;
}

/* CODE BY SIMON LOTHAR, AVAILABLE ON "fx_calculators_SuperH_based.chm" version 16 */

// the function assumes, that the RAM-pointer to GetkeyToMainFunctionReturnFlag is loaded 
// immediately by a "Move Immediate Data"-instruction
unsigned int SetGetkeyToMainFunctionReturnFlag(unsigned int enabled) {
  int addr, addr2;
  // get the pointer to the syscall table
  addr = *(unsigned char*)0x80020071;     // get displacement
  addr++;
  addr *= 4;
  addr += 0x80020070;
  addr = *(unsigned int*)addr;
  if (addr < (int)0x80020070) return 0x101;
  if (addr >= (int)0x81000000) return 0x102;
  // get the pointer to syscall 1E99
  addr += 0x1E99*4;
  if (addr < (int)0x80020070) return 0x103;
  if (addr >= (int)0x81000000) return 0x104;

  addr = *(unsigned int*)addr;
  if (addr < (int)0x80020070) return 0x105;
  if (addr >= (int)0x81000000) return 0x106;

  switch (*(unsigned char*)addr) {
    case 0xD0: // MOV.L @( disp, PC), Rn (REJ09B0317-0400 Rev. 4.00 May 15, 2006 page 216)
    case 0xD1:
    case 0xD2:
    case 0xD3:
    case 0xD4:
    case 0xD5:
    case 0xD6:
    case 0xD7:
    case 0xD8:
      addr2 = *(unsigned char*)(addr + 1);  // get displacement
      addr2++;
      addr2 *= 4;
      addr2 += addr;
      addr2 &= ~3;

      if (addr2 < (int)0x80020070) return 0x107;
      if (addr2 >= (int)0x81000000) return 0x108;

      addr = *(unsigned int*)addr2;
      if ((addr & 0xFF000000) != 0x88000000) return 0x109;

      // finally perform the desired operation and set the flag:
      if (enabled) *(unsigned char*)addr = 0;
      else *(unsigned char*)addr = 3;

      break;
    default: addr = 0x100;
  }
  return addr;
}
