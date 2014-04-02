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

#include "editorGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "hardwareProvider.hpp"
#include "graphicsProvider.hpp"
#include "selectorGUI.hpp" 
#include "fileProvider.hpp"

void fileTextEditor(char* filename, char* basefolder) {
  int newfile = (filename == NULL);
  char sText[TEXT_BUFFER_SIZE] = "";
  if(!newfile) {
    newfile = 0;
    int openerror = 0;
    unsigned short pFile[MAX_FILENAME_SIZE];
    Bfile_StrToName_ncpy(pFile, (unsigned char*)filename, MAX_FILENAME_SIZE); 
    int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
    if(hFile >= 0) // Check if it opened
    { //opened
      unsigned int filesize = Bfile_GetFileSize_OS(hFile);
      if(!filesize || filesize > TEXT_BUFFER_SIZE) {
        openerror = 1;
      } else {
        Bfile_ReadFile_OS(hFile, sText, TEXT_BUFFER_SIZE, 0);
      }
      Bfile_CloseFile_OS(hFile);
    } else {
      openerror = 1;
    }
    if(openerror) {
      //Error opening file, abort
      mMsgBoxPush(4);
      mPrintXY(3, 2, (char*)"Error opening", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      mPrintXY(3, 3, (char*)"file to edit.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      closeMsgBox();
      return;
    }
  }
  textInput input;
  input.forcetext=1;
  input.charlimit=TEXT_BUFFER_SIZE;
  input.buffer = (char*)sText;
  while(1) {
    input.key=0;
    SetBackGround(newfile ? 10 : 6);
    clearLine(1,8);
    drawScreenTitle((char*)"Text Editor", (char*)"File contents:");
    int res = doTextInput(&input);
    int backToEditor = 0;
    if (res==INPUT_RETURN_EXIT) return; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      unsigned short newfilenameshort[0x10A];
      if(newfile) {
        SetBackGround(13);
        clearLine(1,3);
        drawScreenTitle((char*)"Text Editor", (char*)"Save file as:");
        drawFkeyLabels(0x036F); // <
        textInput ninput;
        ninput.forcetext=1;
        ninput.charlimit=MAX_NAME_SIZE;
        char nfilename[MAX_NAME_SIZE] = "";
        ninput.buffer = (char*)nfilename;
        while(1) {
          ninput.key = 0;
          int nres = doTextInput(&ninput);
          if (nres==INPUT_RETURN_EXIT || (nres==INPUT_RETURN_KEYCODE && ninput.key==KEY_CTRL_F1)) {
            // user aborted
            backToEditor = 1;
            break;
          } else if (nres==INPUT_RETURN_CONFIRM) {
            if(stringEndsInG3A(nfilename)) {
              mMsgBoxPush(4);
              mPrintXY(3, 2, (char*)"g3a files can't", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              mPrintXY(3, 3, (char*)"be created by", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              mPrintXY(3, 4, (char*)"an add-in.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              closeMsgBox();
            } else {
              // create and save file
              char newfilename[MAX_FILENAME_SIZE];
              strcpy(newfilename, basefolder);
              strcat(newfilename, nfilename);
              Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)newfilename, 0x10A);
              break;
            }
          }
        }
        if(backToEditor) continue;
      } else {
        // delete, then create and save file
        Bfile_StrToName_ncpy(newfilenameshort, (unsigned char*)filename, 0x10A);
        Bfile_DeleteEntry(newfilenameshort);
      }
      int size = strlen(sText);
      Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FILE, &size); //create the file
      
      int h = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0);
      if(h >= 0) { // Still failing?
        //Write file contents
        Bfile_WriteFile_OS(h, sText, size);
        Bfile_CloseFile_OS(h);
      }
      return;
    }
  }
}