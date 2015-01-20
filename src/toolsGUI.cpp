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

#include "toolsGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "fileProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp" 
#include "toolsProvider.hpp"
#include "calendarGUI.hpp"

void balanceManager() {
  int res=1;
  Menu menu;
  
  menu.scrollout=1;
  menu.height=5;
  menu.startY = 3;
  menu.type=MENUTYPE_FKEYS;
  menu.nodatamsg = (char*)"No data - press F2";
  while(res) {
    char currentWallet[MAX_FILENAME_SIZE] = "";
    if(!getCurrentWallet(currentWallet)) {
      // there is no wallet yet, prompt user to create one
      if(createWalletGUI(1) < 0) return; // user aborted first wallet creation, can't continue
      if(!getCurrentWallet(currentWallet)) return; // error: won't get current wallet not even after creating one and setting it
    }
    res = balanceManagerSub(&menu, currentWallet);
  }
}

int balanceManagerSub(Menu* menu, char* currentWallet) {
  Bdisp_AllClr_VRAM();
  Currency balance;
  getWalletBalance(&balance, currentWallet);
  char balanceStr[15];
  currencyToString(balanceStr, &balance);
  char subtitle[21];
  strcpy(subtitle, (char*)"Balance: ");
  strcat(subtitle, balanceStr);
  Transaction txs[5];
  char menulabels[5][22];
  menu->numitems = getWalletTransactions(currentWallet, txs, 5);
  MenuItem items[menu->numitems];
  for(int i = 0; i < menu->numitems; i++) {
    memset(menulabels[i], ' ', 21);
    int len = strlen(txs[i].description);
    memcpy(menulabels[i], txs[i].description, (len > 14 ? 14 : len));
    char amount[15];
    currencyToString(amount, &txs[i].amount);
    strncpy(menulabels[i]+15, amount, 6);
    menulabels[i][21] = 0;
    if(txs[i].credit) items[i].color = TEXT_COLOR_GREEN;
    else items[i].color = TEXT_COLOR_RED;
    items[i].text = menulabels[i];
  }
  menu->items = items;
  while(1) {
    Bdisp_AllClr_VRAM();
    drawScreenTitle((char*)"Balance Manager", subtitle);
    // VIEW, INSERT, EDIT, DELETE, empty, LOAD
    drawFkeyLabels(-1, 0x03B4, -1, -1, -1, 0x03B7);
    if(menu->numitems > 0) drawFkeyLabels(0x049F, -1, 0x0185, 0x0038);
    if(menu->selection > menu->numitems) menu->selection = menu->numitems;
    if(menu->selection < 1) menu->selection = 1;
    int res = doMenu(menu);
    switch(res) {
      case MENU_RETURN_EXIT:
        return 0;
        break;
      case KEY_CTRL_F1:
      case MENU_RETURN_SELECTION:
        
        break;
      case KEY_CTRL_F2:
        if(addTransactionGUI(currentWallet)) {
          return 1;
        }
        break;
      case KEY_CTRL_F3:
        
        break;
      case KEY_CTRL_F4:
        
        break;
      case KEY_CTRL_F6:
        if(changeWalletGUI()) {
          return 1;
        }
        break;
    }
  }
  return 0;
}

int addTransactionGUI(char* wallet) {
  Transaction tx;
  strcpy(tx.description, (char*)"");
  int curstep = 0;
  while(1) {
    SetBackGround(0x0A);
    drawScreenTitle((char*)"Add transaction");
    // < (first label) and Next or Finish (last label)
    drawFkeyLabels((curstep>0 ? 0x036F : -1), -1, -1, -1, -1, (curstep==4 ? 0x04A4 : 0x04A3));
    if(curstep == 0) {
      MenuItem menuitems[5];
      menuitems[0].text = (char*)"Debit";
      menuitems[1].text = (char*)"Credit";

      Menu menu;
      menu.items=menuitems;
      menu.type=MENUTYPE_FKEYS;
      menu.numitems=2;
      menu.height=2;
      menu.startY=3;
      menu.scrollbar=0;
      menu.scrollout=1;
      menu.pBaRtR=1;
      int inloop=1;
      while(inloop) {
        // this must be here, inside this loop:
        SetBackGround(0x0A);
        drawScreenTitle((char*)"Add transaction", (char*)"Select type:");
        drawFkeyLabels(-1, -1, -1, -1, -1, 0x04A3);
        int res = doMenu(&menu);
        if(res == MENU_RETURN_EXIT) return 0;
        else if(res == KEY_CTRL_F6 || res == MENU_RETURN_SELECTION) {
          tx.credit = menu.selection == 2;
          curstep++;
          break;
        }
      }
    } else if(curstep == 1) {
      drawScreenTitle(NULL, (char*)"Amount:");
      char samount[20] = "";
      if(tx.amount.val) {
        currencyToString(samount, &tx.amount);
      }
      textInput input;
      input.charlimit=12;
      input.acceptF6=1;
      input.symbols = 0; // allow the decimal separator
      input.forcetext = 1;
      input.buffer = (char*)samount;
      input.type = INPUTTYPE_NUMERIC;
      while(1) {
        input.key=0;
        int res = doTextInput(&input);
        if (res==INPUT_RETURN_EXIT) return 0; // user aborted
        else if (res==INPUT_RETURN_CONFIRM) {
          if(!stringToCurrency(&tx.amount, samount)) {
            if(!tx.amount.val) {
              AUX_DisplayErrorMessage(0x4B);
            } else {
              curstep++;
            }
            break;
          } else AUX_DisplayErrorMessage(0x43);
        } else if (res==INPUT_RETURN_KEYCODE && input.key == KEY_CTRL_F1) {
          curstep--;
          break;
        }
      }
    } else if(curstep == 2) {
      drawScreenTitle(NULL, (char*)"Date:");
      mPrintXY(7, 4, (char*)dateSettingToInputDisplay(), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      textInput input;
      input.x=7;
      input.width=8;
      input.charlimit=8;
      input.acceptF6=1;
      input.type=INPUTTYPE_DATE;
      char datebuffer[15];
      fillInputDate(getCurrentYear(), getCurrentMonth(), getCurrentDay(), datebuffer);
      input.buffer = (char*)datebuffer;
      while(1) {
        input.key=0;
        int res = doTextInput(&input);
        if (res==INPUT_RETURN_EXIT) return 0; // user aborted
        else if (res==INPUT_RETURN_CONFIRM) {
          int len = strlen(datebuffer);
          if(len == input.charlimit) {
            int yr,m,d;
            stringToDate(datebuffer, &yr, &m, &d);
            if(isDateValid(yr, m, d)) {
                tx.date.year = yr;
                tx.date.month = m;
                tx.date.day = d;
                curstep++;
                break; // continue to next step
            } else invalidFieldMsg(0);
          } else invalidFieldMsg(0);
        }
        else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep--; break; }
      }
    } else if(curstep == 3) {
      drawScreenTitle(NULL, (char*)"Time:");
      mPrintXY(8, 4, (char*)"HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      
      textInput input;
      input.x=8;
      input.width=6;
      input.charlimit=6;
      input.acceptF6=1;
      input.type=INPUTTYPE_TIME;
      char tbuffer[15];
      fillInputTime(getCurrentHour(), getCurrentMinute(), getCurrentSecond(), tbuffer);
      input.buffer = (char*)tbuffer;
      while(1) {
        input.key=0;
        int res = doTextInput(&input);
        if (res==INPUT_RETURN_EXIT) return 0; // user aborted
        else if (res==INPUT_RETURN_CONFIRM) {
          if((int)strlen(tbuffer) == input.charlimit) {
            int h, m, s;
            stringToTime(tbuffer, &h, &m, &s);
            if(isTimeValid(h, m, s)) {
              tx.time.hour = h;
              tx.time.minute = m;
              tx.time.second = s;
              curstep++;
              break; // continue to next step
            } else invalidFieldMsg(1);
          } else invalidFieldMsg(1);
        } 
        else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep--; break; }
      }
    } else if(curstep == 4) {
      drawScreenTitle(NULL, (char*)"Description:");
      textInput input;
      input.charlimit=128;
      input.acceptF6=1;
      input.buffer = (char*)tx.description;
      int inloop = 1;
      while(inloop) {
        input.key=0;
        int res = doTextInput(&input);
        if (res==INPUT_RETURN_EXIT) return 0; // user aborted
        else if (res==INPUT_RETURN_CONFIRM) inloop = 0; // all fields complete, continue with transaction adding
        else if (res==INPUT_RETURN_KEYCODE && input.key == KEY_CTRL_F1) {
          curstep--;
          break;
        }
      }
      if(!inloop) break;
    }
  }
  addTransaction(&tx, wallet);
  return 1;
}

int createWalletGUI(int isFirstUse) {
  SetBackGround(0x0A);
  drawScreenTitle((char*)"Create wallet", (char*)"Name:");
  drawFkeyLabels(-1, -1, -1, -1, -1, 0x04A3);
  char wallet[MAX_WALLETNAME_SIZE+2] = "";
  textInput input;
  input.charlimit=MAX_WALLETNAME_SIZE;
  input.acceptF6=1;
  input.forcetext=1;
  input.symbols = 0;
  input.buffer = (char*)wallet;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return -1; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) break; // continue to next step
  }

  SetBackGround(0x0A);
  drawScreenTitle((char*)"Create wallet", (char*)"Initial balance:");
  drawFkeyLabels(-1, -1, -1, -1, -1, 0x04A4);
  char balance[20] = "";
  Currency initialBalance;
  textInput input2;
  input2.charlimit=12;
  input2.acceptF6=1;
  input2.symbols = 0; // allow the decimal separator
  input2.buffer = (char*)balance;
  input2.type = INPUTTYPE_NUMERIC;
  while(1) {
    input2.key=0;
    int res = doTextInput(&input2);
    if (res==INPUT_RETURN_EXIT) return -1; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      if(!stringToCurrency(&initialBalance, balance)) break;
      else AUX_DisplayErrorMessage(0x43);
    }
  }
  createWallet(wallet, &initialBalance);
  if(isFirstUse) {
    char fname[MAX_FILENAME_SIZE];
    niceNameToWallet(fname, wallet);
    setCurrentWallet(fname);
  }
  return 0;
}

int changeWalletGUI() {
  // returns 1 if user changes to another wallet
  Menu menu;
  menu.title = (char*)"Wallet List";
  menu.scrollout=1;
  menu.type=MENUTYPE_FKEYS;
  menu.height = 7;
  menu.nodatamsg = (char*)"No wallets";
  MenuItem items[MAX_WALLETS];
  int mustRefresh = 0;
  while(1) {
    // TODO list wallets, let user change wallets, create, rename, delete wallets

    char wallets[MAX_WALLETS][MAX_WALLETNAME_SIZE];
    // build wallet list:
    unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
    char buffer[MAX_FILENAME_SIZE+1];

    // make the buffer
    strcpy(buffer, BALANCEFOLDER"\\*");
    
    file_type_t fileinfo;
    int findhandle;
    Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
    int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
    int i = 0;
    while(!ret) {
      Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
      if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0) && fileinfo.fsize == 0) { // find folders
        strcpy(wallets[i], buffer);
        items[i].text = wallets[i];
        i++;
        if(i == MAX_WALLETS) break;
      }
      ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
    }
    menu.items = items;
    menu.numitems = i;
    Bfile_FindClose(findhandle);
    drawFkeyLabels(0x000F, 0x0186, 0x0188, 0x0038, 0, 0); // SELECT, NEW, RENAME, DELETE
    if(menu.selection > menu.numitems) menu.selection = menu.numitems;
    if(menu.selection < 1) menu.selection = 1;
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_EXIT:
        return mustRefresh;
        break;
      case KEY_CTRL_F1:
      case MENU_RETURN_SELECTION:
        niceNameToWallet(buffer, wallets[menu.selection-1]);
        setCurrentWallet(buffer);
        return 1;
        break;
      case KEY_CTRL_F2:
        if(menu.numitems >= MAX_WALLETS) {
          AUX_DisplayErrorMessage( 0x2E );
        } else {
          createWalletGUI(0);
        }
        break;
      case KEY_CTRL_F3:
        char newWallet[MAX_FILENAME_SIZE];
        if(renameWalletGUI(wallets[menu.selection-1], newWallet)) {
          char currentWallet[MAX_WALLETNAME_SIZE] = "";
          getCurrentWallet(currentWallet);
          niceNameToWallet(buffer, wallets[menu.selection-1]);
          if(!strcmp(currentWallet, buffer)) {
            // if the renamed wallet was the current one, we must set the current wallet to the
            // new name.
            setCurrentWallet(newWallet);
            mustRefresh = 1;
          }
        }
        break;
      case KEY_CTRL_F4:
        niceNameToWallet(buffer, wallets[menu.selection-1]);
        if(deleteWalletGUI(buffer)) {
          if(menu.numitems <= 1) {
            // this was the only wallet: delete pointer file too, so that user is prompted to create a new wallet.
            unsigned short path[MAX_FILENAME_SIZE+1];
            strcpy(buffer, BALANCEFOLDER"\\Wallet");
            Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE);
            Bfile_DeleteEntry(path);
            return 1;
          }
          char currentWallet[MAX_WALLETNAME_SIZE] = "";
          getCurrentWallet(currentWallet);
          if(!strcmp(currentWallet, buffer)) {
            // if the deleted wallet was the current one, we must set the current wallet to the
            // first one on the list that is not the deleted one.
            // (by now we know there is more than one wallet in the list)
            for(int i = 0; i < menu.numitems; i++) {
              niceNameToWallet(buffer, wallets[i]);
              if(strcmp(currentWallet, buffer)) break;
            }
            setCurrentWallet(buffer);
            mustRefresh = 1;
          }
        }
        break;
    }
  }
}

int deleteWalletGUI(char* wallet) {
  mMsgBoxPush(4);
  mPrintXY(3, 2, (char*)"Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  mPrintXY(3, 3, (char*)"Selected Wallet?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  if(closeMsgBox(1, 4)) {
    deleteWallet(wallet);
    return 1;
  }
  return 0;
}

int renameWalletGUI(char* wallet, char* newWallet) {
  // reload the wallets array after using this function!
  // newWallet will receive the new file name (complete, not friendly)
  // returns 0 if user aborts, 1 if renames.
  SetBackGround(6);
  clearLine(1,8);
  char title[MAX_WALLETNAME_SIZE+6];
  strcpy(title, wallet);
  strcat(title, " to:");
  drawScreenTitle((char*)"Rename wallet", title);
  char newname[MAX_WALLETNAME_SIZE];
  strcpy(newname, wallet);
  textInput input;
  input.forcetext=1;
  input.charlimit=MAX_WALLETNAME_SIZE;
  input.buffer = (char*)newname;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return 0; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      char fwallet[MAX_FILENAME_SIZE];
      niceNameToWallet(newWallet, newname);
      niceNameToWallet(fwallet, wallet);
      renameFile(fwallet, newWallet);
      return 1;
    }
  }
  return 0;
}