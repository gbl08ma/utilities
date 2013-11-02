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

#include "aboutGUI.hpp"
#include "textGUI.hpp"
#include "versionProvider.hpp"
#include "graphicsProvider.hpp" 
#include "hardwareProvider.hpp" 

void showAbout() {
  int key;
  Bdisp_AllClr_VRAM();
  Bdisp_EnableColor(1); 
  DefineStatusMessage((char*)"About Utilities", 1, 0, 0);
  DisplayStatusArea();
  //y increment between each line: 17; between paragraphs: 20
  int orange = drawRGB24toRGB565(210, 68, 19);
  int textX = 0;
  int textY = 5;
  char verBuffer[100] = "";
  getVersion(verBuffer);
  PrintMini(&textX, &textY, (unsigned char*)"Version ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)verBuffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
  textY = textY + 17; textX = 0;
  getTimestamp(verBuffer);
  PrintMini(&textX, &textY, (unsigned char*)verBuffer, 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);
  textY = 42;
  textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"Developed by gbl08ma at", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  drawtnyimLogo(10, 59+24, 0); //24 pixels for the status bar
  textY = 101;
  textX = 0;
  // PrintMini and its x,y pointers allow for awesome easy color formatting... let's try
  PrintMini(&textX, &textY, (unsigned char*)"tny. ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"i", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"nternet ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"m", 0, 0xFFFFFFFF, 0, 0, orange, COLOR_WHITE, 1, 0);
  PrintMini(&textX, &textY, (unsigned char*)"edia", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = 125;
  textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"http://i.tny.im | http://gbl08ma.com", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textY = 142;
  textX = 0;
  PrintMini(&textX, &textY, (unsigned char*)"gbl08ma@gmail.com", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  
  textX = 0; textY = 175;
  PrintMini(&textX, &textY, (unsigned char*)"Press any key", 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
  GetKey(&key);

  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  
  textArea text;
  strcpy(text.title, (char*)"");
  text.showtitle=0;

  textElement elem[5];
  text.elements = elem;
  
  elem[0].text = (char*)"Contains code by AHelper, merthsoft and KermMartian at Cemetech (http://cemetech.net), code by Simon Lothar (http://casiopeia.net) and public domain code by C.B. Falconer.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)"This Utilities add-in is licensed under the GNU GPL v2, or (at your option) any later version.";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 8;
  elem[2].color = COLOR_RED;
  elem[2].text = (char*)"USE AT YOUR OWN RISK!";
  elem[3].newLine = 1;
  elem[3].color = COLOR_RED;
  elem[3].text = (char*)"PROVIDED WITHOUT WARRANTY!";
  elem[4].newLine = 1;
  elem[4].lineSpacing = 8;
  elem[4].text = (char*)"In no event will the authors be held liable for any damages arising from the use of this software.";
  text.numelements = 5;
  doTextArea(&text);
  DefineStatusMessage((char*)"", 1, 0, 0);
}