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
  EventDate date;
  EventTime time;
  char credit; // 1 if the transaction increased the balance, 0 if decreased
  char description[134];
} Transaction;

#define MAX_WALLETS 50
#define MAX_WALLETNAME_SIZE 64

void currencyToString(char* dest, Currency* orig);
int stringToCurrency(Currency* dest, char* orig);
void addCurrency(Currency* dest, Currency* operand);
void subtractCurrency(Currency* dest, Currency* operand);
void walletNameToPath(char* dest, char* orig);
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

void generateRandomString(char* dest, int length, int symbols, int numbers, int uppercase, int similar, int vowels, int* seed);

typedef struct {
  int keylen;            // key length when decoded
  int totpcode;          // TOTP computed code
  char name[24];         // human-friendly name for this token (email address, web site name, etc.)
  unsigned char key[20]; // decoded secret key
} totp;

unsigned int computeTOTP(totp* t);
int loadTOTPs(totp* ts);
void addTOTP(char* name, char* key);
void removeTOTP(int index);
void renameTOTP(int index, char* newname);

#endif 
 
