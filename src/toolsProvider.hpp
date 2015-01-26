#ifndef __TOOLSPROVIDER_H
#define __TOOLSPROVIDER_H

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

#include "calendarProvider.hpp"

typedef struct
{
  long long int val=0;
} Currency;

typedef struct
{
  Currency amount;
  int credit; // 1 if the transaction increased the balance, 0 if decreased
  char description[135];
  EventDate date;
  EventTime time;
} Transaction;

#define MAX_WALLETS 50
#define MAX_WALLETNAME_SIZE 64

void currencyToString(char* dest, Currency* orig);
int stringToCurrency(Currency* dest, char* orig);
void addCurrency(Currency* dest, Currency* operand);
void subtractCurrency(Currency* dest, Currency* operand);
void niceNameToWallet(char* dest, char* orig);
void getWalletBalance(Currency* balance, const char* wallet);
int getCurrentWallet(char* wallet);
void setCurrentWallet(char* wallet);
void transactionToCalendarEvent(CalendarEvent* event, Transaction* tx);
void calendarEventToTransaction(Transaction* tx, CalendarEvent* event);
int getWalletTransactions(char* wallet, Transaction* tx, int limit=0);
void addTransaction(Transaction* tx, char* wallet);
void replaceWalletTransactions(Transaction* txs, char* wallet, int count);
void deleteTransaction(Transaction* txs, char* wallet, int count, int pos);
void createWallet(char* name, Currency* balance);
void deleteWallet(char* wallet);
void renameWallet(char* wallet, char* newWallet);

#endif 
 
