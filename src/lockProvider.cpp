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

#include "constantsProvider.hpp"
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "lockProvider.hpp"
#include "calendarProvider.hpp"
#include "hardwareProvider.hpp"
#include "fileProvider.hpp"
#include "debugGUI.hpp"
#include "sha2.h"

void hashPassword(unsigned char* password, unsigned char* hash) {
  char salt[16];
  getHardwareID(salt);
  strcat((char*)password, salt);
  int len = strlen((char*)password);
  sha2(password, len, hash, 0);
}

int comparePasswordHash(unsigned char* inputPassword) {
  //returns values defined as RETURN_PASSWORD_*
  //Load hash from SMEM
  unsigned char smemHash[32] = "";
  int hFile = fileOpen(SMEMHASHFILE);
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist.
    return RETURN_PASSWORD_NOHASH;
  } else {
    Bfile_ReadFile_OS(hFile, smemHash, 32, 0);
    Bfile_CloseFile_OS(hFile);
  }
  unsigned char inputHash[32] = "";    
  hashPassword(inputPassword, inputHash);
  if (!memcmp(inputHash, smemHash, 32)) {
    return RETURN_PASSWORD_MATCH;
  } else {
    return RETURN_PASSWORD_MISMATCH;
  }
}
int savePassword(unsigned char* password) {
  unsigned char hash[32] = "";
  hashPassword(password, hash);
  //now that we hashed, save in the storage memory.
  //Save to SMEM
  //create a folder, if it doesn't exist already
  unsigned short pFile[MAX_FILENAME_SIZE];
  Bfile_StrToName_ncpy(pFile, CALENDARFOLDER, MAX_FILENAME_SIZE);
  Bfile_CreateEntry_OS(pFile, CREATEMODE_FOLDER, 0); //create a folder for the file
  // now the file
  Bfile_StrToName_ncpy(pFile, SMEMHASHFILE, MAX_FILENAME_SIZE); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  size_t size = 32;
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist. create it
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres < 0) return 2; //error: file doesn't exist and yet can't be created.
    hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
    // Check if it opened now that we created it:
    if(hFile < 0) return 3;
  }
  //file exists (even if it didn't exist before) and is open. overwrite its contents with the new hash.
  Bfile_WriteFile_OS(hFile, hash, size);
  Bfile_CloseFile_OS(hFile);
  return 0;
}

int isPasswordSet(void) {
  //returns 1 if user has already set a code for locking the calc, 0 if not.
  int hFile = fileOpen(SMEMHASHFILE);
  if(hFile < 0) return 0;
  Bfile_CloseFile_OS(hFile);
  return 1;
}