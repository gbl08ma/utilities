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
  char salt[16] = "";
  getHardwareID(salt);
  strcat((char*)password, salt);
  int len = strlen((char*)password);
  sha2( password, len, hash, 0 );
}

int comparePasswordHash(unsigned char* inputPassword) {
  //returns 0 if password's hash matches the hash stored in SMEM and MCS
  //returns 1 if there's no hash stored in MCS, 2 if no hash in SMEM
  //returns 3 if hashes stored don't match
  //returns 4 if hashed password and stored hashes don't match
  //Load hash from SMEM
  unsigned short pFile[MAX_FILENAME_SIZE]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, SMEMHASHFILE, MAX_FILENAME_SIZE); 
  hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  unsigned char smemHash[32] = "";
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist.
    return RETURN_PASSWORD_NOSMEM;
  } else {
    Bfile_ReadFile_OS(hFile, smemHash, 32, 0);
    Bfile_CloseFile_OS(hFile);
  }
  //Load hash from MCS
  int MCSsize;
  unsigned char mcsHash[32] = "";
  MCSGetDlen2(DIRNAME, HASHFILE, &MCSsize);
  if (!MCSsize) { return RETURN_PASSWORD_NOMCS; }
  MCSGetData1(0, 32, mcsHash);
  if (!memcmp(mcsHash, smemHash, 32)) {
    //hash stored in main mem matches with storage memory.
    //now compare the hash with the hash of the inserted password
    unsigned char inputHash[32];    
    hashPassword(inputPassword, inputHash);
    if (!memcmp(inputHash, smemHash, 32)) {
      return RETURN_PASSWORD_MATCH;
    } else {
      return RETURN_PASSWORD_MISMATCH;
    }
  } else {
    return RETURN_PASSWORD_STORAGE_MISMATCH; 
  }
}
int savePassword(unsigned char* password) {
  unsigned char hash[32];
  hashPassword(password, hash);
  //now that we hashed, save in the main memory and storage memory.
  //Save to SMEM
  //create a folder, if it doesn't exist already
  unsigned short pFolder[MAX_FILENAME_SIZE];
  Bfile_StrToName_ncpy(pFolder, CALENDARFOLDER, MAX_FILENAME_SIZE);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file
  // now the file
  unsigned short pFile[MAX_FILENAME_SIZE]; // Make buffer
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
  
  //----------
  //Now, the MCS.
  int createResult = MCS_CreateDirectory(DIRNAME);
  if(createResult != 0) // Check directory existence
  { // directory already exists, so delete the existing file that may be there
    MCSDelVar2(DIRNAME, HASHFILE);
  }
  //write the file
  MCSPutVar2(DIRNAME, HASHFILE, size, hash);
  return 0;
}

int isPasswordSet(void) {
  //returns 1 if user has already set a code for locking the calc, 0 if not.
  // check SMEM.
  unsigned short pFile[MAX_FILENAME_SIZE]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, SMEMHASHFILE, MAX_FILENAME_SIZE); 
  hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hFile < 0) return 0;
  Bfile_CloseFile_OS(hFile);
  
  //check MCS
  int size;
  MCSGetDlen2(DIRNAME, HASHFILE, &size);
  return (size != 0); //code exists in both SMEM and MCS
}