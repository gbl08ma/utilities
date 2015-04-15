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
#include <ctype.h>

#include "toolsGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "fileProvider.hpp"
#include "graphicsProvider.hpp"
#include "stringsProvider.hpp"
#include "timeProvider.hpp" 
#include "toolsProvider.hpp"
#include "calendarProvider.hpp"
#include "calendarGUI.hpp"
#include "selectorGUI.hpp"
#include "timeGUI.hpp"
#include "fileGUI.hpp"

// Balance manager:

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
      if(createWalletWizard(1) < 0) return; // user aborted first wallet creation, can't continue
      if(!getCurrentWallet(currentWallet)) return; // error: won't get current wallet not even after creating one and setting it
    }
    res = balanceManagerChild(&menu, currentWallet);
  }
}

int balanceManagerChild(Menu* menu, char* currentWallet) {
  Bdisp_AllClr_VRAM();
  Currency balance;
  getWalletBalance(&balance, currentWallet);
  char balanceStr[15];
  currencyToString(balanceStr, &balance);
  char subtitle[21];
  strcpy(subtitle, (char*)"Balance: ");
  strcat(subtitle, balanceStr);
  Transaction txs[MAX_DAY_EVENTS];
  char menulabels[MAX_DAY_EVENTS][44];
  menu->numitems = getWalletTransactions(currentWallet, txs);
  MenuItem items[menu->numitems];
  for(int i = 0; i < menu->numitems; i++) {
    char amount[15];
    currencyToString(amount, &txs[i].amount);

    // build menu item so that the text is cut and the values aligned on the right column
    // independently of the description containing multibyte chars and their location
    char* s = txs[i].description;
    int len = 0, glen = 0;
    while(*s) {
      len++;
      glen++;
      if(MB_IsLead(*s)) glen--;
      if(glen >= 14) { // this way we only care about the section that is to be displayed
        break;
      }
      s++;
    }
    memset(menulabels[i], ' ', 43); // blank parts that would not be touched
    int offset = len-glen;
    memcpy(menulabels[i], txs[i].description, (glen > 14 ? 14+offset : len));
    if(MB_IsLead(menulabels[i][14+offset]))
      menulabels[i][14+offset] = ' '; // eliminate half-MB char
    strncpy(menulabels[i]+15+offset, amount, 6);
    menulabels[i][21+offset] = 0;

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
    if(menu->numitems > 0) drawFkeyLabels(0x049F, -1, 0x0038);
    int res = doMenu(menu);
    switch(res) {
      case MENU_RETURN_EXIT:
        return 0;
        break;
      case KEY_CTRL_F1:
      case MENU_RETURN_SELECTION:
      case MENU_RETURN_SELECTION_RIGHT:
        if(menu->numitems) viewTransaction(&txs[menu->selection-1]);
        break;
      case KEY_CTRL_F2:
        if(addTransactionWizard(currentWallet)) {
          return 1;
        }
        break;
      case KEY_CTRL_F3:
      case KEY_CTRL_DEL:
        if(menu->numitems && deleteTransactionPrompt(txs, currentWallet, menu->numitems, menu->selection-1)) {
          return 1;
        }
        break;
      case KEY_CTRL_F6:
        if(changeWalletScreen(currentWallet)) {
          menu->selection = 0;
          menu->scroll = 0;
          return 1;
        }
        break;
    }
  }
  return 0;
}

int addTransactionWizard(char* wallet) {
  Transaction tx;
  tx.date.year = getCurrentYear();
  tx.date.month = getCurrentMonth();
  tx.date.day = getCurrentDay();
  tx.time.hour = getCurrentHour();
  tx.time.minute = getCurrentMinute();
  tx.time.second = getCurrentSecond();
  strcpy(tx.description, (char*)"");
  int curstep = 0;
  while(1) {
    SetBackGround(0x0A);
    drawScreenTitle((char*)"Add transaction");
    // < (first label), SELECT of on date step, and Next or Finish (last label)
    drawFkeyLabels((curstep>0 ? 0x036F : -1), (curstep == 2 ? 0x000F : -1), -1, -1, -1, (curstep==4 ? 0x04A4 : 0x04A3));
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
      mPrintXY(7, 4, (char*)getInputDateFormatHint(), TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      textInput input;
      input.x=7;
      input.width=8;
      input.charlimit=8;
      input.acceptF6=1;
      input.type=INPUTTYPE_DATE;
      char datebuffer[15];
      fillInputDate(tx.date.year, tx.date.month, tx.date.day, datebuffer);
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
        } else if (res==INPUT_RETURN_KEYCODE) {
          if(input.key==KEY_CTRL_F1) {
            curstep=curstep-1; break;
          } else if(input.key==KEY_CTRL_F2) {
            int ey=0, em=0, ed=0;
            if(!selectDateScreen(&ey, &em, &ed,
                                  (char*)"Select transaction date:", NULL, 1)) {
              tx.date.year = ey;
              tx.date.month = em;
              tx.date.day = ed;
              curstep++; break; // continue to next step
            }
            break; //redraw
          }
        }
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
      fillInputTime(tx.time.hour, tx.time.minute, tx.time.second, tbuffer);
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

int deleteTransactionPrompt(Transaction* txs, char* wallet, int count, int pos) {
  mMsgBoxPush(5);
  multiPrintXY(3, 2, "Delete the\nSelected\nTransaction?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  if(closeMsgBox(1, 5)) {
    deleteTransaction(txs, wallet, count, pos);
    return 1;
  }
  return 0;
}

void viewTransaction(Transaction* tx) {
  textArea text;
  if(tx->credit) {
    text.title = (char*)"Credit information";
  } else {
    text.title = (char*)"Debit information";
  }
  text.allowLeft = 1;
  
  textElement elem[15];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor
  
  elem[text.numelements].text = (char*)"Description:";
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  
  elem[text.numelements].text = tx->description;
  text.numelements++;
    
  elem[text.numelements].text = (char*)"Date & time:";
  elem[text.numelements].newLine = 1;
  elem[text.numelements].spaceAtEnd=1;
  elem[text.numelements].color=COLOR_LIGHTGRAY; 
  text.numelements++;
  
  char date[50];
  dateToString(date, tx->date.year, tx->date.month, tx->date.day);
  
  char buffer[15]="";
  timeToString(buffer, tx->time.hour, tx->time.minute, tx->time.second);
  strcat(date, (char*)" ");
  strcat(date, buffer);
  
  elem[text.numelements].text = date;
  text.numelements++;
  
  char amount[20];
  currencyToString(amount, &tx->amount);

  elem[text.numelements].text = (char*)"Amount:";
  elem[text.numelements].newLine = 1;
  elem[text.numelements].spaceAtEnd=1;
  elem[text.numelements].color=COLOR_LIGHTGRAY;
  text.numelements++;
  
  elem[text.numelements].text = amount;
  text.numelements++;

  doTextArea(&text);
}

int createWalletWizard(int isFirstUse) {
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
    walletNameToPath(fname, wallet);
    setCurrentWallet(fname);
  }
  return 0;
}

int changeWalletScreen(char* currentWallet) {
  // returns 1 if user changes to another wallet
  char currentWalletNice[MAX_WALLETNAME_SIZE];
  filenameToName(currentWallet, currentWalletNice, MAX_WALLETNAME_SIZE);
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
        if(!strcmp(buffer, currentWalletNice)) menu.selection = i;
        if(i == MAX_WALLETS) break;
      }
      ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
    }
    menu.items = items;
    menu.numitems = i;
    Bfile_FindClose(findhandle);
    drawFkeyLabels(0x000F, 0x0186, 0x0188, 0x0038, 0, 0); // SELECT, NEW, RENAME, DELETE
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_EXIT:
        return mustRefresh;
        break;
      case KEY_CTRL_F1:
      case MENU_RETURN_SELECTION:
        if(strcmp(currentWalletNice, wallets[menu.selection-1])) {
          walletNameToPath(buffer, wallets[menu.selection-1]);
          setCurrentWallet(buffer);
          return 1;
        } return mustRefresh;
        break;
      case KEY_CTRL_F2:
        if(menu.numitems >= MAX_WALLETS) {
          AUX_DisplayErrorMessage( 0x2E );
        } else {
          createWalletWizard(0);
        }
        break;
      case KEY_CTRL_F3:
        char newWallet[MAX_FILENAME_SIZE];
        if(renameWalletScreen(wallets[menu.selection-1], newWallet)) {
          walletNameToPath(buffer, wallets[menu.selection-1]);
          if(!strcmp(currentWallet, buffer)) {
            // if the renamed wallet was the current one, we must set the current wallet to the
            // new name.
            setCurrentWallet(newWallet);
            mustRefresh = 1;
          }
        }
        break;
      case KEY_CTRL_F4:
      case KEY_CTRL_DEL:
        walletNameToPath(buffer, wallets[menu.selection-1]);
        if(deleteWalletPrompt(buffer)) {
          if(menu.numitems <= 1) {
            // this was the only wallet: delete pointer file too, so that user is prompted to create a new wallet.
            unsigned short path[MAX_FILENAME_SIZE+1];
            strcpy(buffer, BALANCEFOLDER"\\Wallet");
            Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE);
            Bfile_DeleteEntry(path);
            return 1;
          }
          if(!strcmp(currentWallet, buffer)) {
            // if the deleted wallet was the current one, we must set the current wallet to the
            // first one on the list that is not the deleted one.
            // (by now we know there is more than one wallet in the list)
            for(int i = 0; i < menu.numitems; i++) {
              walletNameToPath(buffer, wallets[i]);
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

int deleteWalletPrompt(char* wallet) {
  mMsgBoxPush(4);
  multiPrintXY(3, 2, "Delete the\nSelected Wallet?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  if(closeMsgBox(1, 4)) {
    deleteWallet(wallet);
    return 1;
  }
  return 0;
}

int renameWalletScreen(char* wallet, char* newWallet) {
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
      walletNameToPath(newWallet, newname);
      walletNameToPath(fwallet, wallet);
      renameFile(fwallet, newWallet);
      return 1;
    }
  }
  return 0;
}

// Password generator:

void passwordGenerator() {
  Menu menu;
  menu.type = MENUTYPE_FKEYS;
  menu.scrollbar = 0;
  menu.title = (char*)"Password Generator";
  menu.height = 7;
  MenuItem items[10];
  int length = 10;
  int seed = RTC_GetTicks() * (GetMainBatteryVoltage(1) % 100);
  char lstr[10];
  items[1].text = (char*)"Include symbols";
  items[1].type = MENUITEM_CHECKBOX;
  items[2].text = (char*)"Include numbers";
  items[2].type = MENUITEM_CHECKBOX;
  items[2].value = MENUITEM_VALUE_CHECKED;
  items[3].text = (char*)"Include uppercase";
  items[3].type = MENUITEM_CHECKBOX;
  items[3].value = MENUITEM_VALUE_CHECKED;
  items[4].text = (char*)"Include confusable";
  items[4].type = MENUITEM_CHECKBOX;
  items[4].value = MENUITEM_VALUE_CHECKED;
  items[5].text = (char*)"Memorable vowel mix";
  items[5].type = MENUITEM_CHECKBOX;
  menu.numitems = 6;
  menu.items = items;
  while(1) {
    Bdisp_AllClr_VRAM();
    drawFkeyLabels(0x03B3, 0, 0, 0, 0, 0x0184); // FILE, EXE (white)
    itoa(length, (unsigned char*)lstr);
    char t[20] = "Length: ";
    strcat(t, lstr);
    items[0].text = t;
    switch(doMenu(&menu)) {
      case MENU_RETURN_EXIT:
        return;
      case MENU_RETURN_SELECTION:
        if(menu.selection > 1) items[menu.selection-1].value = !items[menu.selection-1].value;
        else {
          Selector sel;
          sel.min = 6;
          sel.value = length;
          sel.max = 30;
          sel.cycle = 1;
          sel.title = (char*)"Password Generator";
          sel.subtitle = (char*)"Length";
          if(doSelector(&sel) == SELECTOR_RETURN_SELECTION) {
            length = sel.value;
          }
        }
        break;
      case KEY_CTRL_F1:
      {
        Selector sel;
        sel.min = 1;
        sel.value = 10;
        sel.max = 1000;
        sel.cycle = 1;
        sel.title = (char*)"Generate to file";
        sel.subtitle = (char*)"Number of passwords";
        if(doSelector(&sel) != SELECTOR_RETURN_SELECTION) break;

        SetBackGround(10);
        drawScreenTitle((char*)"Generate to file", (char*)"Filename:");
        char newname[MAX_NAME_SIZE];
        newname[0] = 0;
        textInput input;
        input.forcetext=1;
        input.charlimit=MAX_NAME_SIZE;
        input.buffer = (char*)newname;
        int inscreen = 1;
        while(inscreen) {
          input.key=0;
          int res = doTextInput(&input);
          if (res==INPUT_RETURN_EXIT) break; // user aborted
          else if (res==INPUT_RETURN_CONFIRM) {
            inscreen = 0;
          }
        }
        if(inscreen) break;
        char newfilename[MAX_FILENAME_SIZE];
        strcpy(newfilename, SMEM_PREFIX);
        strcat(newfilename, newname);
        strcat(newfilename, ".txt");
        unsigned short pFile[0x10A];
        Bfile_StrToName_ncpy(pFile, newfilename, 0x10A);
        unsigned int size = 1;
        int ntry = 0;
        while(ntry < 2) {
          ntry++;
          int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
          if(BCEres >= 0) {
            int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
            if(hFile >= 0) {
              char password[35];
              char line[37];
              for(int i = 0; i < sel.value; i++) {
                generateRandomString(password, length, items[1].value, items[2].value, items[3].value, items[4].value, items[5].value, &seed);
                sprintf(line, "%s\r\n", password);
                Bfile_WriteFile_OS(hFile, line, length+2);
              }
              Bfile_CloseFile_OS(hFile);
            } else AUX_DisplayErrorMessage(0x2B);
            break;
          } else if(ntry < 2) {
            // File creation probably failed due to the presence of a file with the same name in SMEM
            if(overwriteFilePrompt(newfilename))
              Bfile_DeleteEntry(pFile);
            else
              break;
          } else AUX_DisplayErrorMessage(0x2B);
        }
        break;
      }
      case KEY_CTRL_F6:
        int inscreen = 1;
        while(inscreen) {
          Bdisp_AllClr_VRAM();
          drawScreenTitle((char*)"Password Generator", (char*)"Generated passwords:");
          textArea text;
          text.type = TEXTAREATYPE_INSTANT_RETURN;
          text.scrollbar = 0;
          text.y = 48+3;
          text.lineHeight = 20;
          textElement e[5];
          char passwords[5][35];
          for(int i = 0; i < 5; i++) {
            generateRandomString(passwords[i], length, items[1].value, items[2].value, items[3].value, items[4].value, items[5].value, &seed);
            e[i].text = passwords[i];
            if(i) e[i].newLine = 1;
          }
          text.elements = e;
          text.numelements = 5;
          doTextArea(&text);
          drawFkeyLabels(0x036F, 0, 0, 0, 0, 0x02B9); // <, REPEAT (white)
          while(1) {
            int key;
            mGetKey(&key);
            if(key == KEY_CTRL_F6) break;
            if(key == KEY_CTRL_F1 || key == KEY_CTRL_EXIT) {
              inscreen = 0;
              break;
            }
          }
        }
        break;
    }
  }
}

// TOTP authenticator:

void totpClient() {
  RTCunadjustedWizard(1);
  if(isRTCunadjusted()) {
    mMsgBoxPush(6);
    multiPrintXY(3, 2, "The OATH TOTP\nauthenticator\ncan't work with\nthe clock\nunadjusted.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    closeMsgBox(0, 7);
    return;
  }
  totp ts[MAX_DAY_EVENTS];
  Menu menu;
  menu.type = MENUTYPE_FKEYS;
  MenuItem items[MAX_DAY_EVENTS];
  menu.items = items;
  reload:
  menu.numitems = loadTOTPs(ts);
  for(int i = 0; i < menu.numitems; i++) {
    items[i].text = ts[i].name;
  }
  menu.title = (char*)"TOTP authenticator";
  menu.scrollbar = 1;
  menu.scrollout = 1;
  menu.height = 7;
  menu.nodatamsg = (char*)"No tokens - press F2";
  while(1) {
    drawFkeyLabels(0, 0x0186, 0, 0, 0, 0x012A); // NEW, TIME
    if(menu.numitems) {
      drawFkeyLabels(0x0235, -1, 0x0188, 0x0038); // FACTOR, RENAME, DELETE
    }
    int res = doMenu(&menu);
    switch(res) {
      case MENU_RETURN_EXIT:
        return;
      case KEY_CTRL_F1:
        if(!menu.numitems) break;
      case MENU_RETURN_SELECTION:
      case MENU_RETURN_SELECTION_RIGHT:
        viewTOTPcode(&ts[menu.selection-1]);
        break;
      case KEY_CTRL_F2:
        if(menu.numitems >= MAX_DAY_EVENTS) {
          AUX_DisplayErrorMessage(0x2E);
        } else {
          if(addTOTPwizard()) goto reload;
        }
        break;
      case KEY_CTRL_F3:
        if(menu.numitems && renameTOTPscreen(menu.selection-1, ts[menu.selection-1].name))
          goto reload;
        break;
      case KEY_CTRL_F4:
      case KEY_CTRL_DEL:
        if(menu.numitems && deleteTOTPprompt(menu.selection-1)) goto reload;
        break;
      case KEY_CTRL_F6:
        setTimezone();
        break;
    }
  }
}

void viewTOTPcode(totp* tkn) {
  unsigned short key = 0; int keyCol, keyRow;
  Bdisp_AllClr_VRAM();
  drawScreenTitle(tkn->name);
  int shown_since_beginning = 0;
  while(key != KEY_PRGM_EXIT && key != KEY_PRGM_LEFT) {
    int ThirtySecCode = computeTOTP(tkn);
    char buffer[10];
    itoa_zeropad(tkn->totpcode, buffer, 6);
    long long int ms_spent_ll = currentUTCUEBT() - (long long int)ThirtySecCode * 30LL * 1000LL;
    int ms_spent = (int)(ms_spent_ll);

    drawCircularCountdownIndicator(LCD_WIDTH_PX/2, 104, 44, COLOR_BLACK, COLOR_WHITE, (ms_spent*43)/30000, getCurrentSecond() < 30 ? 0 : 1);
    // fade in/out animation for text
    int val = 0;
    if(ms_spent >= 29000) {
      val += (-29000 + ms_spent)/4;
    } else if (ms_spent <= 1020) {
      val += (1020 - ms_spent)/4;
    }
    int color = drawRGB24toRGB565(val, val, val);
    printCentered(buffer, 164, color, COLOR_WHITE);
    if(ms_spent < 2500) shown_since_beginning = 1;
    else if(shown_since_beginning) {
      if(ms_spent < 5000) {
        DefineStatusMessage((char*)"Having problems with the code?", 1, 0, 0);
      } else if(ms_spent < 7500) {
        DefineStatusMessage((char*)"If yes, please make sure that the", 1, 0, 0);
      } else if(ms_spent < 10000) {
        DefineStatusMessage((char*)"calculator's clock is adjusted and", 1, 0, 0);
      } else if(ms_spent < 12500) {
        DefineStatusMessage((char*)"that the timezone is correctly set.", 1, 0, 0);
      } else if(ms_spent < 15000) {
        DefineStatusMessage((char*)"You can press OPTN to adjust both.", 1, 0, 0);
      } else if(ms_spent < 17500) {
        DefineStatusMessage((char*)"", 1, 0, 0);
      }
    }
    DisplayStatusArea();
    Bdisp_PutDisp_DD();
    key = PRGM_GetKey();
    if(key == KEY_PRGM_MENU) GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key); //this is here just to handle the Menu key
    if(key == KEY_PRGM_OPTN) {
      DefineStatusMessage((char*)"", 1, 0, 0);
      GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key); // clear keybuffer
      RTCunadjustedWizard(0, 1);
      setTimezone();
      return; // so we don't have to redraw etc.
      // Also, this way the Shift+Menu instruction shown in the adjustment wizard becomes valid immediately,
      // which is great if the user wants to repeat the adjustment immediately.
    }
  }
  DefineStatusMessage((char*)"", 1, 0, 0);
  // clear keybuffer:
  GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key);
}

int addTOTPwizard() {
  char friendlyname[25] = "";
  char key[32] = "";
  int curstep = 0;
  while(1) {
    SetBackGround(0x0A);
    drawScreenTitle((char*)"Add TOTP token");
    // < (first label) and Next or Finish (last label)
    drawFkeyLabels((curstep>0 ? 0x036F : 0), 0, 0, 0, 0, (curstep==1 ? 0x04A4 : 0x04A3));
    if(curstep == 0) {
      drawScreenTitle(NULL, (char*)"Friendly name:");
      textInput input;
      input.charlimit=21;
      input.acceptF6=1;
      input.forcetext = 1;
      input.buffer = friendlyname;
      while(1) {
        input.key=0;
        int res = doTextInput(&input);
        if (res==INPUT_RETURN_EXIT) return 0; // user aborted
        else if (res==INPUT_RETURN_CONFIRM) {
          curstep++;
          break;
        }
      }
    } else if(curstep == 1) {
      drawScreenTitle(NULL, (char*)"Base32 key:");
      int textX = 0;
      int textY = 3*24+2;
      PrintMini(&textX, &textY, (char*)"This is often shown as a QR code.", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textX=0; textY += 17;
      PrintMini(&textX, &textY, (char*)"Try looking for the \"I can't scan a QR", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textX=0; textY += 17;
      PrintMini(&textX, &textY, (char*)"code\" option. A key usually looks", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textX=0; textY += 17;
      PrintMini(&textX, &textY, (char*)"like this: JBSWY3DPEHPK3PXP", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textX=0; textY += 17;
      PrintMini(&textX, &textY, (char*)"Some keys are longer.", 0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      textInput input;
      input.charlimit=32;
      input.acceptF6=1;
      input.forcetext = 1;
      input.buffer = key;
      int inloop = 1;
      while(inloop) {
        input.key=0;
        int res = doTextInput(&input);
        if (res==INPUT_RETURN_EXIT) return 0; // user aborted
        else if (res==INPUT_RETURN_CONFIRM) {
          char* b = key;
          for(; *b; b++) {
            *b = toupper(*b);
            // commonly mistyped characters:
            if(*b == '0') *b = 'O';
            if(*b == '1') *b = 'I';
          }
          if(base32_validate(key)) {
            inloop = 0; // all fields complete, continue with token adding
          } else {
            AUX_DisplayErrorMessage(0x3E);
          }
        }
        else if (res==INPUT_RETURN_KEYCODE && input.key == KEY_CTRL_F1) {
          curstep--;
          break;
        }
      }
      if(!inloop) break;
    }
  }
  addTOTP(friendlyname, key);
  return 1;
}

int renameTOTPscreen(int index, char* oldname) {
  //reload the files array after using this function!
  //returns 0 if user aborts, 1 if renames.
  SetBackGround(6);
  clearLine(1,8);
  drawScreenTitle((char*)"Rename TOTP token");
  char newname[25];
  strcpy(newname, oldname);
  textInput input;
  input.forcetext=1;
  input.charlimit=21;
  input.buffer = (char*)newname;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return 0; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      renameTOTP(index, newname);
      return 1;
    }
  }
  return 0;
}

int deleteTOTPprompt(int index) {
  mMsgBoxPush(5);
  multiPrintXY(3, 2, "Remove the\nSelected Token?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  int textX = 2*18; int textY = 3*24;
  PrintMiniMini(&textX, &textY, (char*)"Make sure you do not lock yourself out", 16, TEXT_COLOR_RED, 0);
  textX = 2*18; textY += 12;
  PrintMiniMini(&textX, &textY, (char*)"of any website or other system.", 16, TEXT_COLOR_RED, 0);
  if(closeMsgBox(1, 5)) {
    removeTOTP(index);
    return 1;
  }
  return 0;
}