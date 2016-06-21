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
#include "textEditGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "fileProvider.hpp"
#include "fileGUI.hpp"
#include "sha1.h"

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
  textEdit input;
  //input.forcetext=1;
  input.charlimit=TEXT_BUFFER_SIZE;
  input.buffer = (char*)sText;

  // calculate checksum so we can check for changes
  unsigned char origHash[20] = "";
  sha1((unsigned char*)sText, strlen(sText), origHash);
  while(1) {
    input.key=0;
    int res = doTextEdit(&input);
    int exit = 0;
    switch(res) {
      case TEXTEDIT_RETURN_EXIT:
      {
        exit = 1;
        unsigned char newHash[20] = "";
        sha1((unsigned char*)sText, strlen(sText), newHash);
        if(!memcmp(origHash, newHash, 20)) return;
        else {
          mMsgBoxPush(4);
          mPrintXY(3, 2, "Save this file?", TEXT_MODE_TRANSPARENT_BACKGROUND,
                   TEXT_COLOR_BLACK);
          if(closeMsgBox(1, 4)) {
            // fall through
          } else {
            return;
          }
        }
      }
      case TEXTEDIT_RETURN_CONFIRM:
      {
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
          // clear unsaved changes "flag":
          sha1((unsigned char*)sText, strlen(sText), origHash);
        }
        if(exit) return;
      }
      break;
    }
  }
}