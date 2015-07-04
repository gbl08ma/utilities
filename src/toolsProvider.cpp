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

#include "toolsProvider.hpp" 
#include "settingsProvider.hpp"
#include "calendarProvider.hpp" // calendar functions are repurposed as part of the storage backend
#include "constantsProvider.hpp"
#include "fileProvider.hpp"
#include "timeProvider.hpp"
#include "stringsProvider.hpp"
#include "sha1.h"

long long int llabs(long long int i) {
  if(i < 0) i = -i;
  return i;
}
void currencyToString(char* dest, Currency* orig) {
  int units = (int)(orig->val / 100LL);
  int cents = (int)(llabs(orig->val) % 100LL);
  sprintf(dest, "%d.%s%d", units, cents < 10 ? "0" : "", cents);
}

int stringToCurrency(Currency* dest, char* orig) {
  // returns 0 on success, 1 on error
  char buffer[20];
  int i;
  for(i = 0; *orig; orig++) {
    if(*orig == '.') break;
    buffer[i++] = *orig;
  }
  buffer[i] = 0;
  if(i > 9) return 1; // too many digits for units
  long long int res = (long long int)atoi(buffer) * 100LL;
  int hasNegativeSign = 0;
  if(buffer[0] == '-') {
    hasNegativeSign = 1;
  }
  int cents;
  if(*orig) {
    int l = strncpy_retlen(buffer, orig+1, 20);
    if(l < 2) cents = atoi(buffer) * 10LL;
    else if(l == 2) cents = atoi(buffer);
    else return 1; // error: too many digits for cents
  } else cents = 0;

  if(res < 0) res -= (long long int)cents;
  else res += (long long int)cents;
  if(hasNegativeSign && res > 0) res = -res;
  dest->val = res;
  return 0;
}

void addCurrency(Currency* dest, Currency* operand) {
  // adds operand to dest (result in dest)
  dest->val += operand->val;
}

void subtractCurrency(Currency* dest, Currency* operand) {
  // subtracts operand from dest (result in dest)
  dest->val -= operand->val;
}

void walletNameToPath(char* dest, char* orig) {
  strcpy(dest, BALANCEFOLDER"\\");
  strcat(dest, orig);
}

void getWalletBalance(Currency* balance, const char* wallet) {
  char file[MAX_FILENAME_SIZE];
  strcpy(file, wallet);
  strcat(file, (char*)"\\Balance");
  int hFile = fileOpen(file);
  if(hFile < 0) return;
  char contents[15] = "";
  Bfile_ReadFile_OS(hFile, contents, 15, 0);
  stringToCurrency(balance, contents);
  Bfile_CloseFile_OS(hFile);
}

void setWalletBalance(Currency* balance, const char* wallet) {
  char file[MAX_FILENAME_SIZE];
  strcpy(file, wallet);
  strcat(file, (char*)"\\Balance");
  int hFile = fileOpen(file);
  if(hFile < 0) return;
  char contents[20] = "";
  currencyToString((char*)contents, balance);
  int size = strlen(contents);
  Bfile_WriteFile_OS(hFile, contents, size+4); // 4 bytes to ensure it's null-terminated
  Bfile_CloseFile_OS(hFile);
}

int getCurrentWallet(char* wallet) {
  // puts wallet folder in the buffer wallet points to, and returns 1
  // or returns 0 if there's no wallet yet
  char statusfile[MAX_FILENAME_SIZE];
  unsigned short ufile[MAX_FILENAME_SIZE];
  strcpy(statusfile, BALANCEFOLDER"\\Wallet");
  Bfile_StrToName_ncpy(ufile, statusfile, MAX_FILENAME_SIZE);
  int hFile = Bfile_OpenFile_OS(ufile, READWRITE, 0); // Get handle
  if(hFile < 0)
    return 0;
  Bfile_ReadFile_OS(hFile, wallet, MAX_FILENAME_SIZE, 0);
  Bfile_CloseFile_OS(hFile);  
  return 1;
}

void setCurrentWallet(char* wallet) {
  // sets current wallet folder to the specified one
  // doesn't check that the wallet folder actually exists
  char statusfile[MAX_FILENAME_SIZE];
  unsigned short ufile[MAX_FILENAME_SIZE];
  strcpy(statusfile, BALANCEFOLDER"\\Wallet");
  Bfile_StrToName_ncpy(ufile, statusfile, MAX_FILENAME_SIZE);
  int hFile = Bfile_OpenFile_OS(ufile, READWRITE, 0); // Get handle
  if(hFile < 0) {
    size_t size = 1;
    Bfile_CreateEntry_OS(ufile, CREATEMODE_FILE, &size);
    hFile = Bfile_OpenFile_OS(ufile, READWRITE, 0);
    if(hFile < 0) return;
  }
  strcpy(statusfile, wallet);
  size_t size = strlen(wallet);
  // must ensure it's null-terminated
  statusfile[size] = 0;
  statusfile[size+1] = 0;
  Bfile_WriteFile_OS(hFile, statusfile, size+1);
  Bfile_CloseFile_OS(hFile);  
}

void transactionToCalendarEvent(CalendarEvent* event, Transaction* tx) {
  // the transaction history for each wallet uses the calendar event code for storage
  // each wallet's folder acts as "calendar database"
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  event->startdate = date;
  event->enddate = tx->date;
  event->endtime = tx->time;
  event->timed = tx->credit;
  currencyToString((char*)event->title, &tx->amount);
  strcpy((char*)event->location, tx->description);
  strcpy((char*)event->description, (char*)"");
}

void calendarEventToTransaction(Transaction* tx, CalendarEvent* event) {
  tx->date = event->enddate;
  tx->time = event->endtime;
  tx->credit = event->timed;
  stringToCurrency(&tx->amount, (char*)event->title);
  strcpy(tx->description, (char*)event->location);
}

int getWalletTransactions(char* wallet, Transaction* tx, int limit) {
  // puts in the array of transactions "tx" "limit" number of transactions,
  // starting by the most recent (they come ordered because they are put in order in the file)
  // returns the number of transactions
  CalendarEvent events[MAX_DAY_EVENTS];
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  int txcount = getEvents(&date, wallet, events, limit);
  if(!txcount) return 0;
  for(int i = 0; i<txcount; i++) {
    calendarEventToTransaction(&tx[i], &events[i]);
  }
  return txcount;
}

void addTransaction(Transaction* tx, char* wallet) {
  // adds a single transaction "tx" to the specified wallet
  // update balance:
  Currency balance;
  getWalletBalance(&balance, wallet);
  if(tx->credit) {
    addCurrency(&balance, &tx->amount);
  } else {
    subtractCurrency(&balance, &tx->amount);
  }
  setWalletBalance(&balance, wallet);
  // update transaction history, taking into account the fact
  // that transactions must be stored with the most recent first
  Transaction txs[MAX_DAY_EVENTS];
  int txcount = getWalletTransactions(wallet, txs);
  CalendarEvent event;
  transactionToCalendarEvent(&event, tx);
  if(!txcount) {
    // no transactions, must create file
    addEvent(&event, wallet);
  } else {
    // transactions are stored ordered with the most recent first.
    // if there is the maximum number of transactions in the file, delete the oldest one.
    // iterate from the beginning until one older than the current one is found,
    // then move all transactions forward in the array by one and insert the new
    // one in the free space.
    if(txcount == MAX_DAY_EVENTS) {
      // must delete the oldest one
      long long int oldestTimestamp =
        dateTimeToUEBT(txs[0].date.year, txs[0].date.month, txs[0].date.day,
                       txs[0].time.hour, txs[0].time.minute, txs[0].time.second, 0);
      int oldestIndex = 0;
      for(int i = 1; i < txcount; i++) {
        long long int thisTxTimestamp =
          dateTimeToUEBT(txs[i].date.year, txs[i].date.month, txs[i].date.day,
                         txs[i].time.hour, txs[i].time.minute, txs[i].time.second, 0);
        if(thisTxTimestamp < oldestTimestamp) {
          oldestTimestamp = thisTxTimestamp;
          oldestIndex = i;
        }
      }
      for (int k = oldestIndex; k < txcount - 1; k++)
            txs[k] = txs[k+1];
      txcount--;
    }
    long long int newTxTimestamp = dateTimeToUEBT(tx->date.year, tx->date.month, tx->date.day,
                                                tx->time.hour, tx->time.minute, tx->time.second, 0);
    int i = 0;
    for(; i < txcount; i++) {
      long long int thisTxTimestamp =
        dateTimeToUEBT(txs[i].date.year, txs[i].date.month, txs[i].date.day,
                       txs[i].time.hour, txs[i].time.minute, txs[i].time.second, 0);
      if(thisTxTimestamp <= newTxTimestamp) break;
    }
    int newFreeIndex = i;
    for(i = txcount; i > newFreeIndex; i--) {
      txs[i] = txs[i-1];
    }
    txs[newFreeIndex] = *tx;
    replaceWalletTransactions(txs, wallet, txcount+1);
  }
}

void replaceWalletTransactions(Transaction* txs, char* wallet, int count) {
  CalendarEvent events[MAX_DAY_EVENTS];
  for(int i = 0; i < count; i++) {
    transactionToCalendarEvent(&events[i], &txs[i]);
  }
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  replaceEventFile(&date, events, wallet, count);
}

void deleteTransaction(Transaction* txs, char* wallet, int count, int pos) {
  // undo effect of transaction in wallet balance:
  Currency balance;
  getWalletBalance(&balance, wallet);
  if(txs[pos].credit) {
    subtractCurrency(&balance, &txs[pos].amount);
  } else {
    addCurrency(&balance, &txs[pos].amount);
  }
  setWalletBalance(&balance, wallet);
  // now delete from transaction history:
  CalendarEvent events[MAX_DAY_EVENTS];
  for(int i = 0; i < count; i++) {
    transactionToCalendarEvent(&events[i], &txs[i]);
  }
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  removeEvent(&date, events, wallet, count, pos);
}

void createWallet(char* name, Currency* balance) {
  // creates a wallet without checking if it already exists
  // balance is the initial balance in internal (memory) format
  char folder[MAX_FILENAME_SIZE];
  unsigned short ufolder[MAX_FILENAME_SIZE];
  walletNameToPath(folder, name);
  Bfile_StrToName_ncpy(ufolder, folder, MAX_FILENAME_SIZE);
  if(Bfile_CreateEntry_OS(ufolder, CREATEMODE_FOLDER, 0) < 0) {
    // error creating wallet folder, probably balance system isn't initialized yet
    // create recursively then
    createFolderRecursive(folder);
  }
  // now create balance file for this wallet
  char file[MAX_FILENAME_SIZE];
  strcpy(file, folder);
  strcat(file, (char*)"\\Balance");
  Bfile_StrToName_ncpy(ufolder, file, MAX_FILENAME_SIZE);
  size_t size = 1;
  Bfile_CreateEntry_OS(ufolder, CREATEMODE_FILE, &size);
  setWalletBalance(balance, folder);
}

void deleteWallet(char* wallet) {
  unsigned short path[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(path, wallet, MAX_FILENAME_SIZE+1);
  Bfile_DeleteEntry(path);
}

int randInt(int* rnd_seed) {
  int k1;
  int ix = *rnd_seed;

  k1 = ix / 127773;
  ix = 16807 * (ix - k1 * 127773) - k1 * 2836;
  if (ix < 0)
      ix += 2147483647;
  *rnd_seed = ix;
  return *rnd_seed;
}
#define INT_MAX 2147483647
unsigned int randInterval(unsigned int min, unsigned int max, int* seed) {
  // from http://stackoverflow.com/a/17554531
  int r;
  const unsigned int range = 1 + max - min;
  const unsigned int buckets = INT_MAX / range;
  const unsigned int limit = buckets * range;

  /* Create equal size buckets all in a row, then fire randomly towards
   * the buckets until you land in one of them. All buckets are equally
   * likely. If you land off the end of the line of buckets, try again. */
  do
  {
      r = randInt(seed);
  } while (r >= (int)limit);

  return min + (r / buckets);
}

void generateRandomString(char* dest, int length, int symbols, int numbers, int uppercase,
                          int similar, int vowels, int* seed) {
  int i = 0;
  int vowelSpacing = randInterval(1, 2, seed);
  while(i<length) {
    char next = randInterval(33, 126, seed);
    if(!symbols &&
       (next < 48 || (next > 57 && next < 65) || (next > 90 && next < 97) || next > 122))
      continue;
    if(!numbers && next >= '0' && next <= '9') continue;
    if(!uppercase && next >= 'A' && next <= 'Z') continue;
    if(!similar && (strchr((char*)"0OoIl1()[]{};:|-_B8", next))) continue;
    if(vowels) {
      if(!vowelSpacing) {
        if(!strchr((char*)"aeiouy", next) && !strchr((char*)"AEIOUY", next)) continue;
        vowelSpacing = randInterval(1, 2, seed);
      } else {
        if(strchr((char*)"aeiouy", next) || strchr((char*)"AEIOUY", next)) continue;
        vowelSpacing--;
      }
    }
    dest[i++] = next;
    dest[i] = 0;
  }
}

unsigned int computeTOTP(totp* t) {
  long long int curtime = currentUTCUEBT() / 1000LL; // seconds since 1 Jan 1970
  curtime /= 30LL; // 30 second intervals since 1970 like TOTP wants
  unsigned int code = (unsigned int)curtime;

  int i;
  unsigned char challenge[8];
  challenge[0] = 0;
  challenge[1] = 0;
  challenge[2] = 0;
  challenge[3] = 0;
  challenge[4] = (int)((code >> 24) & 0xFF) ;
  challenge[5] = (int)((code >> 16) & 0xFF) ;
  challenge[6] = (int)((code >> 8) & 0XFF);
  challenge[7] = (int)((code & 0XFF));

  unsigned char digest[20];
  sha1_hmac(t->key, t->keylen, challenge, 8, digest);
  unsigned char offset = digest[19] & 0xF;

  t->totpcode = 0;
  for (i = 0; i < 4; ++i) {
    t->totpcode <<= 8;
    t->totpcode  |= digest[offset + i];
  }
  t->totpcode &= 0x7FFFFFFF;
  t->totpcode %= 1000000;
  return code;
}

int loadTOTPs(totp* ts) {
  CalendarEvent events[MAX_DAY_EVENTS];
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  int tokencount = getEvents(&date, TOTPFOLDER, events);
  for(int i = 0; i<tokencount; i++) {
    ts[i].keylen = base32_decode((unsigned char*)events[i].description, ts[i].key, 32);
    // create key from secret string
    if(ts[i].keylen < 0) {
      // error decoding
      tokencount--;
      removeTOTP(i);
      continue;
    }
    strcpy(ts[i].name, (char*)events[i].title);
  }
  return tokencount;
}

void addTOTP(char* name, char* key) {
  // key is the base32 encoded key. will not be checked for validity
  CalendarEvent event;
  strcpy((char*)event.title, name);
  strcpy((char*)event.description, key);
  strcpy((char*)event.location, (char*)"");
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  event.startdate = date;
  addEvent(&event, TOTPFOLDER);
}

void removeTOTP(int index) {
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  CalendarEvent events[MAX_DAY_EVENTS];
  int c = getEvents(&date, TOTPFOLDER, events);
  removeEvent(&date, events, TOTPFOLDER, c, index);
}

void renameTOTP(int index, char* newname) {
  EventDate date;
  date.day = 0; date.month = 0; date.year = 0;
  CalendarEvent events[MAX_DAY_EVENTS];
  int c = getEvents(&date, TOTPFOLDER, events);
  strcpy((char*)events[index].title, newname);
  replaceEventFile(&date, events, TOTPFOLDER, c);
}