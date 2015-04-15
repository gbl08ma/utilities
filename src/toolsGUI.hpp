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
int balanceManagerChild(Menu* menu, char* currentWallet);
int addTransactionWizard(char* wallet);
int deleteTransactionPrompt(Transaction* txs, char* wallet, int count, int pos);
void viewTransaction(Transaction* tx);
int createWalletWizard(int isFirstUse);
int changeWalletScreen(char* currentWallet);
int deleteWalletPrompt(char* wallet);
int renameWalletScreen(char* wallet, char* newWallet);

void passwordGenerator();
void totpClient();
void viewTOTPcode(totp* tkn);
int addTOTPwizard();
int renameTOTPscreen(int index, char* oldname);
int deleteTOTPprompt(int index);

#endif 
