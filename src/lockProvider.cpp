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
#include "sha2.h"

int compare(unsigned char * a, unsigned char * b, int size)
{
  int i;
  for(i=0;i<size;i++)
        if(a[i]!=b[i])
                return 1;
  return 0;
}

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
  char smemfile[50] = "";
  strcpy(smemfile, "\\\\fls0\\");
  strcat(smemfile, CALENDARFOLDER);
  strcat(smemfile, "\\Hash.plp");
  unsigned short pFile[sizeof(smemfile)*2]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, (unsigned char*)smemfile, strlen(smemfile)+1); 
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
  if (MCSsize == 0) { return RETURN_PASSWORD_NOMCS; }
  MCSGetData1(0, 32, mcsHash);
  int cmpres = compare(mcsHash, smemHash, 32);
  if (0==cmpres) {
    //hash stored in main mem matches with storage memory.
    //now compare the hash with the hash of the inserted password
    unsigned char inputHash[32] = "";    
    hashPassword(inputPassword, inputHash);
    cmpres = compare(inputHash, smemHash, 32);
    if (0==cmpres) {
      return RETURN_PASSWORD_MATCH;
    } else {
      return RETURN_PASSWORD_MISMATCH;
    }
  } else {
    return RETURN_PASSWORD_STORAGE_MISMATCH; 
  }
}
int savePassword(unsigned char* password) {
  unsigned char hash[32] = "";
  hashPassword(password, hash);
  //now that we hashed, save in the main memory and storage memory.
  //Save to SMEM
  //create a folder, if it doesn't exist already
  char foldername[128] = "";
  strcpy(foldername, "\\\\fls0\\");
  strcat(foldername, CALENDARFOLDER);
  unsigned short pFolder[256];
  Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file
  // now the file
  char smemfile[50] = "";
  strcpy(smemfile, "\\\\fls0\\");
  strcat(smemfile, CALENDARFOLDER);
  strcat(smemfile, "\\Hash.plp");
  unsigned short pFile[sizeof(smemfile)*2]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, (unsigned char*)smemfile, strlen(smemfile)+1); 
  hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  int size = 32;
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist. create it
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres >= 0) // Did it create?
    {
      hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0);
      if(hFile < 0) // Still failing?
      {
        return 1; //error: file doesn't exist, was created but still can't be opened.
      } else {
        //file didn't exist, but was created. save hash.
        Bfile_WriteFile_OS(hFile, hash, size);
        Bfile_CloseFile_OS(hFile);
      }
    } else {
      return 2; //error: file doesn't exist and yet can't be created.
    }
  } else {
    //file exists. delete it and create a new one with the new hash.
    Bfile_CloseFile_OS(hFile);
    Bfile_DeleteEntry(pFile);
    if(Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size) >= 0) // Did it create?
    {
      hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0);
      if(hFile < 0) // Still failing?
      {
        return 3; //error: file doesn't exist (because it was deleted), was created but still can't be opened.
      } else {
        //file didn't exist, but was created. save hash.
        Bfile_WriteFile_OS(hFile, hash, size);
        Bfile_CloseFile_OS(hFile);
      }
    }
    else
    {
      return 4; //error: file doesn't exist (it was deleted) and yet can't be created.
    }
  }
  
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
  char smemfile[50] = "";
  strcpy(smemfile, "\\\\fls0\\");
  strcat(smemfile, CALENDARFOLDER);
  strcat(smemfile, "\\Hash.plp");
  unsigned short pFile[sizeof(smemfile)*2]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, (unsigned char*)smemfile, strlen(smemfile)+1); 
  hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist.
    return 0;
  } else {
    Bfile_CloseFile_OS(hFile);
  }
  
  //check MCS
  int size;
  MCSGetDlen2(DIRNAME, HASHFILE, &size);
  if (size == 0) { return 0; }
  return 1; //code exists in both SMEM and MCS
}