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

#include "linkProvider.hpp"
#include "menuGUI.hpp"
#include "graphicsProvider.hpp"
#include "textGUI.hpp"

// code based on example by Simon Lothar
// send a file through the 3-pin serial. Compatible with Casio's functions.
// remote calculator should be in receive mode with cable type set to 3-pin
// pay attention to CPU clock speeds as they affect the baud rate, causing errors

#ifdef DISABLED_EXPERIMENTAL_CODE
void endSerialComm(int error) {
  if(error) {
    Comm_Terminate(2); // display "Receive ERROR" on remote
  } else {
    Comm_Terminate(0); // display "Complete!" on remote
  }
  while(1) {
    // wait for pending transmissions and close serial
    if(Comm_Close(0) != 5) break;
  }

  mMsgBoxPush(4);
  if(error) {
    mPrintXY(3, 2, "An error", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    char buffer[20];
    char buffer2[20];
    strcpy(buffer, "occurred (");
    itoa(error, (unsigned char*)buffer2);
    strcat(buffer, buffer2);
    strcat(buffer, ").");
    mPrintXY(3, 3, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    mPrintXY(3, 2, "Transfer", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, "successful.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  closeMsgBox();
}

void serialTransferSingleFile(char* filename) {
  if(strncmp(filename, SMEM_PREFIX, 7)) return; // ERROR

  textArea text;
  text.type=TEXTAREATYPE_INSTANT_RETURN;
  text.scrollbar=0;
  text.title = (char*)"3-pin file sending";
  
  textElement elem[25];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor
  
  elem[text.numelements].text = (char*)"Preparing...";
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  TTransmitBuffer sftb;

  memset(&sftb, 0, sizeof(sftb));
  strcpy(sftb.device, SMEM_DEVICE);
  strcpy(sftb.fname1, (char*)filename+7);

  Bfile_StrToName_ncpy(sftb.filename, filename, 0x10A);
  // get filesize:
  int handle = Bfile_OpenFile_OS(sftb.filename, READWRITE, 0);
  if(handle < 0) {
    endSerialComm(-1);
    return;
  }
  int fsize = sftb.filesize = Bfile_GetFileSize_OS(handle);
  Bfile_CloseFile_OS(handle);

  sftb.command = 0x45;
  sftb.datatype = 0x80;
  sftb.handle = -1;
  sftb.source = 1; // SMEM:1
  sftb.zero = 0;

  // start communicating with remote
  App_LINK_SetReceiveTimeout_ms(6000);
  if(Comm_Open(0x1000)) {
    endSerialComm(1);
    return;
  }

  elem[text.numelements].text = (char*)"Checking connection...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  if(Comm_TryCheckPacket(0)) {
    endSerialComm(2);
    return;
  }

  elem[text.numelements].text = (char*)"Changing connection settings...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  if(App_LINK_SetRemoteBaud()) {
    endSerialComm(3);
    return;
  }

  OS_InnerWait_ms(20); // wait for remote calculator to change link settings

  elem[text.numelements].text = (char*)"Checking new settings...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  if(Comm_TryCheckPacket(1)) {
    endSerialComm(4);
    return;
  }

  elem[text.numelements].text = (char*)"Preparing file sending...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  // start file transfer
  if(App_LINK_TransmitInit(&sftb)) {
    endSerialComm(7);
    return;
  }

  elem[text.numelements].text = (char*)"Sending file...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  int sTime = RTC_GetTicks();
  if(App_LINK_Transmit(&sftb)) {
    endSerialComm(8);
    return;
  }
  int elTime = RTC_GetTicks() - sTime;

  elem[text.numelements].text = (char*)"Ending connection...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  // terminate transfer and close serial
  endSerialComm(0);

  elem[text.numelements].text = (char*)"Done.";
  elem[text.numelements].newLine = 1;
  elem[text.numelements].spaceAtEnd = 1;
  text.numelements++;

  if(elTime > 0) {
    elem[text.numelements].text = (char*)"Transfer took";
    elem[text.numelements].spaceAtEnd = 1;
    text.numelements++;

    int seconds = elTime/128;

    char sbuf[10];
    itoa(seconds, (unsigned char*)sbuf);
    elem[text.numelements].text = sbuf;
    elem[text.numelements].spaceAtEnd = 1;
    text.numelements++;

    elem[text.numelements].text = (char*)"seconds, average speed";
    elem[text.numelements].spaceAtEnd = 1;
    text.numelements++;

    char spbuf[10];
    itoa(fsize/seconds, (unsigned char*)spbuf);
    elem[text.numelements].text = spbuf;
    elem[text.numelements].spaceAtEnd = 1;
    text.numelements++;

    elem[text.numelements].text = (char*)"bytes/s.";
    elem[text.numelements].spaceAtEnd = 1;
    text.numelements++;
  }

  elem[text.numelements].text = (char*)"Press EXIT.";
  text.numelements++;

  text.type = TEXTAREATYPE_NORMAL;
  text.scrollbar = 1;
  doTextArea(&text);

  // done!
  return;
}
#endif