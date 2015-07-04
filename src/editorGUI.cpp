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
#include "inputGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "fileProvider.hpp"
#include "fileGUI.hpp"

void textfileEditor(char* filename, char* basefolder) {
  int newfile = (filename == NULL);
  char sText[TEXT_BUFFER_SIZE] = "";
  if(!newfile) {
    newfile = 0;
    int openerror = 0;
    int hFile = fileOpen(filename); // Get handle
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
      AUX_DisplayErrorMessage(0x2B); // Data ERROR
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
    drawScreenTitle("Text Editor", "File contents:");
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      char newfilename[MAX_FILENAME_SIZE];
      unsigned short newfilenameshort[0x10A];
      if(newfile) {
        int backToEditor = 0;
        SetBackGround(13);
        drawScreenTitle("Text Editor", "Save file as:");
        drawFkeyLabels(0x036F); // <
        textInput ninput;
        ninput.forcetext=1;
        ninput.charlimit=MAX_NAME_SIZE;
        char nfilename[MAX_NAME_SIZE];
        nfilename[0] = 0;
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
              multiPrintXY(3, 2, "g3a files can't\nbe created by\nan add-in.",
                           TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              closeMsgBox();
            } else {
              // create and save file
              strcpy(newfilename, basefolder);
              strcat(newfilename, nfilename);
              Bfile_StrToName_ncpy(newfilenameshort, newfilename, 0x10A);
              break;
            }
          }
        }
        if(backToEditor) continue;
      } else {
        // delete, then create and save file
        Bfile_StrToName_ncpy(newfilenameshort, filename, 0x10A);
        Bfile_DeleteEntry(newfilenameshort);
      }
      size_t size = strlen(sText);
      if(Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FILE, &size) < 0) { //create the file
        // it appears file exists, overwrite?
        if(overwriteFilePrompt(newfilename)) {
          Bfile_DeleteEntry(newfilenameshort);
          Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FILE, &size);
        }
        else continue; // abort file save so user can discard the file, or type another filename.
      }
      
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