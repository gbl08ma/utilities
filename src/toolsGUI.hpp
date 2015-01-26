#ifndef __TOOLSGUI_H
#define __TOOLSGUI_H

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
#include "fileProvider.hpp"
#include "toolsProvider.hpp"

void balanceManager();
int balanceManagerSub(Menu* menu, char* currentWallet);
int addTransactionGUI(char* wallet);
int deleteTransactionGUI(Transaction* txs, char* wallet, int count, int pos);
int createWalletGUI(int isFirstUse);
int changeWalletGUI(char* currentWallet);
int deleteWalletGUI(char* wallet);
int renameWalletGUI(char* wallet, char* newWallet);

#endif 
