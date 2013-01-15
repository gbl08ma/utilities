#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/serial.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define CREATEMODE_FILE 1
#define CREATEMODE_FOLDER 5
#define READ 0
#define READ_SHARE 1
#define WRITE 2
#define READWRITE 3
#define READWRITE_SHARE 4
#include "lock.hpp"
#include "mainmenulocker.hpp"
#include "sha2.h"

#define DIRNAME_MCS (unsigned char*)"@UTILS"
#define HASHFILE_MCS (unsigned char*)"Hash"

/*Password hash is saved in two places: Main Memory and Storage Memory.
  The password is never saved as-is; only the hash is saved.
  The password in the two locations must match when loading the password, otherwise system is "permanently" locked.
*/
void dbgNoPause(unsigned char*msg, int line) {
  locate_OS(1,line);
  Print_OS((unsigned char*)"                     ", 0, 0);
  locate_OS(1,line);
	Print_OS(msg, 0, 0);
	Bdisp_PutDisp_DD();
}
void hashPassword(unsigned char* password, unsigned char* hash) {
  int len = strlen((char*)password);
  sha2( password, len, hash, 0 );
}
int compare(unsigned char * a, unsigned char * b, int size)
{
  int i;
  for(i=0;i<size;i++)
  	if(a[i]!=b[i])
  		return 1;
  return 0;
}

int comparePasswordHash(unsigned char* inputPassword) {
  //returns 0 if password's hash matches the hash stored in SMEM and MCS
  //returns 1 if there's no hash stored in MCS, 2 if no hash in SMEM
  //returns 3 if hashed password and stored hashes don't match
  //returns 4 if hashes don't match
  //Load hash from SMEM
  char smemfile[50] = "\\\\fls0\\@CALNDAR\\Hash.plp";
  unsigned short pFile[sizeof(smemfile)*2]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, (unsigned char*)smemfile, strlen(smemfile)+1); 
  hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  unsigned char smemHash[32] = "";
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist.
    return 2;
  } else {
  	Bfile_ReadFile_OS(hFile, smemHash, 32, 0);
    Bfile_CloseFile_OS(hFile);
  }
  //Load hash from MCS
  int MCSsize;
  unsigned char mcsHash[32] = "";
  MCSGetDlen2(DIRNAME_MCS, HASHFILE_MCS, &MCSsize);
  if (MCSsize == 0) { return 1; }
  MCSGetData1(0, 32, mcsHash);
  int cmpres = compare(mcsHash, smemHash, 32);
  if (0==cmpres) {
    //hash stored in main mem matches with storage memory.
    //now compare the hash with the hash of the inserted password
    unsigned char inputHash[32] = "";
    hashPassword(inputPassword, inputHash);
    cmpres = compare(inputHash, smemHash, 32);
    if (0==cmpres) {
      return 0;
    } else {
      return 4;
    }
  } else {
    return 3; 
  }
}
int savePassword(unsigned char* password) {
  unsigned char hash[32] = "";
  hashPassword(password, hash);
  //now that we hashed, save in the main memory and storage memory.
  //Save to SMEM
  //create a folder, if it doesn't exist already
  char foldername[128] = "\\\\fls0\\@CALNDAR";
  unsigned short pFolder[256];
  Bfile_StrToName_ncpy(pFolder, (unsigned char*)foldername, strlen(foldername)+1);
  Bfile_CreateEntry_OS(pFolder, CREATEMODE_FOLDER, 0); //create a folder for the file
  // now the file
  char smemfile[50] = "\\\\fls0\\@CALNDAR\\Hash.plp";
  unsigned short pFile[sizeof(smemfile)*2]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, (unsigned char*)smemfile, strlen(smemfile)+1); 
  hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  int size = 32;
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist. create it
    int BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if(BCEres >= 0) // Did it create?
    {
      hFile = Bfile_OpenFile_OS(pFile, READWRITE);
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
      hFile = Bfile_OpenFile_OS(pFile, READWRITE);
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
  int createResult = MCS_CreateDirectory(DIRNAME_MCS);
  if(createResult != 0) // Check directory existence
  { // directory already exists, so delete the existing file that may be there
    MCSDelVar2(DIRNAME_MCS, HASHFILE_MCS);
  }
  //write the file
  MCSPutVar2(DIRNAME_MCS, HASHFILE_MCS, size, hash);
  return 0;
}

int isPasswordSet(void) {
  //returns 1 if user has already set a code for locking the calc, 0 if not.
  // check SMEM.
  char smemfile[50] = "\\\\fls0\\@CALNDAR\\Hash.plp";
  unsigned short pFile[sizeof(smemfile)*2]; // Make buffer
  int hFile;
  Bfile_StrToName_ncpy(pFile, (unsigned char*)smemfile, strlen(smemfile)+1); 
  hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0) // Check if it opened
  {
    //error, file doesn't exist.
    return 0;
  } else {
    Bfile_CloseFile_OS(hFile);
  }
  
  //check MCS
  int size;
  MCSGetDlen2(DIRNAME_MCS, HASHFILE_MCS, &size);
  if (size == 0) { return 0; }
  return 1; //code exists in both SMEM and MCS
}

int passwordInput(int x, int y, unsigned char* buffer, int display_statusbar, int showlastchar, int lock) {
  //returns: 0 on user abort (EXIT), 1 on EXE
  int start = 0, cursor = 0, key;
  int charlimit = 256;
  unsigned char dispbuffer[256] = ""; //will hold asterisks instead of real characters...
  //clean buffer and display buffer
  strcpy((char*)buffer, "");
  strcpy((char*)dispbuffer, "");
  while(1)
  {   
    strcpy((char*)dispbuffer, "");
    int numchars = strlen((char*)buffer);
    if(showlastchar)
    {
      if(numchars > 0) {
        for (int k = 0; k < numchars-1; k++) {
          strcat((char*)dispbuffer, "*");
        }
        //show last character for easier typing
        strcat((char*)dispbuffer, " ");
        dispbuffer[(strlen((char*)dispbuffer)-1)] = buffer[(strlen((char*)buffer)-1)];
      }
    } else {
      if(numchars > 0) {
        for (int k = 0; k < numchars; k++) {
          strcat((char*)dispbuffer, "*");
        }
      }
    }
    int iresult;
    GetFKeyPtr(0x0307, &iresult); // A<>a
    FKey_Display(5, (int*)iresult);
    Cursor_SetFlashOn(5);
    DisplayMBString((unsigned char*)dispbuffer, start, cursor, x,y);

    int keyflag = GetSetupSetting( (unsigned int)0x14);
    GetKey(&key);
    if (GetSetupSetting( (unsigned int)0x14) == 0x01 || GetSetupSetting( (unsigned int)0x14) == 0x04 || GetSetupSetting( (unsigned int)0x14) == 0x84) {
      keyflag = GetSetupSetting( (unsigned int)0x14); //make sure the flag we're using is the updated one.
      //we can't update always because that way alpha-not-lock will cancel when F5 is pressed.
    }
    if(key == KEY_CTRL_F6)
    {
      if (keyflag == 0x04 || keyflag == 0x08 || keyflag == 0x84 || keyflag == 0x88) {
        // ^only applies if some sort of alpha (not locked) is already on
        if (keyflag == 0x08 || keyflag == 0x88) { //if lowercase
          SetSetupSetting( (unsigned int)0x14, keyflag-0x04);
          if (display_statusbar == 1) DisplayStatusArea();
          continue; //do not process the key, because otherwise we will leave alpha status
        } else {
          SetSetupSetting( (unsigned int)0x14, keyflag+0x04);
          if (display_statusbar == 1) DisplayStatusArea();
          continue; //do not process the key, because otherwise we will leave alpha status
        }
      }

    } 
    if (key == KEY_CTRL_EXIT) { Cursor_SetFlashOff(); return 0; }
    if (key == KEY_CTRL_EXE) {
      if (strlen((char*)buffer) > 0) {
        Cursor_SetFlashOff(); return 1;
      } else {
        MsgBoxPush(3);
        PrintXY(3, 3, (char*)"  Code can't be", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
        PrintXY(3, 4, (char*)"  empty.", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
        PrintXY(3, 5, (char*)"     Press:[EXIT]", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
        int nkey;
        while(1) { GetKey(&nkey); if (nkey==KEY_CTRL_EXIT) { break; } }
        MsgBoxPop();
      }
    }
    if(key && key < 30000)
    {
      if ((keyflag == 0x08 || keyflag == 0x88) && key >= KEY_CHAR_A && key <= KEY_CHAR_Z) //if lowercase and key is char...
      {
        key = key + 32; //so we switch to lowercase characters... Casio is smart
      }
      cursor = EditMBStringChar((unsigned char*)buffer, charlimit, cursor, key);
    }
    else
    {
      EditMBStringCtrl((unsigned char*)buffer, charlimit, &start, &cursor, &key, x, y);
    }
  }
  Cursor_SetFlashOff();
  return 0;
}
int setPassword(int display_statusbar, int showlastchar) {
  unsigned char password[256] = "";
  
  Bdisp_AllClr_VRAM();
  if (display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Calculator lock", TEXT_MODE_NORMAL, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Set new code:", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  int res = passwordInput(1, 3, password, display_statusbar, showlastchar, 0);
  if (res == 1) savePassword(password);
  return res; //which means this returns 1 if password was set, and 0 if user aborted
}
int unlockCalc(int display_statusbar, int showlastchar) {
  //returns 1 on unlocked and 0 on still locked
  unsigned char password[256] = "";
  
  Bdisp_AllClr_VRAM();
  if (display_statusbar == 1) DisplayStatusArea();
  PrintXY(1, 1, (char*)"  Calculator lock", TEXT_MODE_NORMAL, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)"  Input code:", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
  int res = passwordInput(1, 3, password, display_statusbar, showlastchar, 1);
  int compareRes=0;
  if (res == 0) { return 0; }
  else if (res == 1) compareRes = comparePasswordHash(password);
  int key;
  switch(compareRes)
  {
    case 0:
      return 1;
      break;
    case 1:
    case 2:
    case 3:
      MsgBoxPush(3);
      PrintXY(3, 3, (char*)"  Data tampering", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      PrintXY(3, 4, (char*)"  detected!", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      PrintXY(3, 5, (char*)"     Press:[EXIT]", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      while(1) { GetKey(&key); if (key==KEY_CTRL_EXIT) { break; } }
      MsgBoxPop();
      return 0;
      break;
    case 4:
      MsgBoxPush(3);
      PrintXY(3, 3, (char*)"  Wrong code", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      PrintXY(3, 5, (char*)"     Press:[EXIT]", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
      while(1) { GetKey(&key); if (key==KEY_CTRL_EXIT) { break; } }
      MsgBoxPop();
      return 0;
      break;
    default: break;
  }
  return 0;
}
int lockCalc(int display_statusbar, int showlastchar, int autopoweroff) {
  //returns 0 on "calculator was locked and now is unlocked"
  //returns 1 on "there was no lock code and one was now set"
  if(!isPasswordSet()) {
    setPassword(display_statusbar, showlastchar);
    return 1;
  }
  int textX, textY;
  int permanentX;
  textX = 0;
  textY = LCD_HEIGHT_PX - 17 - 24;
  PrintMini(&textX, &textY, (unsigned char*)"Calculator locked", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 0, 0); //get length
  permanentX = LCD_WIDTH_PX - textX;
  if(autopoweroff) PowerOff(1);
  SetGetkeyToMainFunctionReturnFlag(0); //Disable menu return
  while(1) {
    textX = permanentX;
    int key;
    Bdisp_AllClr_VRAM();
    if (display_statusbar == 1) DisplayStatusArea();
    PrintMini(&textX, &textY, (unsigned char*)"Calculator locked", 0, 0xFFFFFFFF, 0, 0, COLOR_LIGHTGRAY, COLOR_WHITE, 1, 0);
    //Handle ALPHA (when user wants to unlock) and Shift+AC for power off
    Bdisp_PutDisp_DD();
    GetKey(&key); //oh, the pleasure of using GetKey and still have the Menu blocked
    if (key == KEY_CTRL_ALPHA) {
      SetSetupSetting( (unsigned int)0x14, 0); //avoid alpha still being triggered at start of text input
      if(1==unlockCalc(display_statusbar, showlastchar)) {
        SetGetkeyToMainFunctionReturnFlag(1); //Enable menu return      
        return 0;
      }
    }
  }
}