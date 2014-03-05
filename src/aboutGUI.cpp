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
#include "keyboardProvider.hpp"

void showAbout() {
  int key;
  DefineStatusMessage((char*)"About Utilities", 1, 0, 0);
  
  textArea ftext;
  ftext.showtitle=0;
  ftext.scrollbar=0;
  ftext.type=TEXTAREATYPE_INSTANT_RETURN;

  textElement felem[14];
  ftext.elements = felem;
  
  felem[0].text = (char*)"Version";
  felem[0].spaceAtEnd = 1;
  felem[0].color=COLOR_BLUE;
  felem[1].text = (char*)getVersion();
  felem[1].color=COLOR_BLUE;
  
  felem[2].newLine = 1;
  felem[2].text = (char*)getTimestamp();
  felem[2].color=COLOR_GRAY;
  
  felem[3].newLine = 1;
  felem[3].lineSpacing = 8;
  felem[3].text = (char*)"Developed by gbl08ma at";
  
  felem[4].newLine = 1;
  felem[4].lineSpacing = 42;
  felem[4].text = (char*)"tny.";
  felem[4].spaceAtEnd = 1;
  
  felem[5].text = (char*)"i";
  felem[5].color = TNYIM_ORANGE;
  
  felem[6].text = (char*)"nternet";
  felem[6].spaceAtEnd = 1;
  
  felem[7].text = (char*)"m";
  felem[7].color = TNYIM_ORANGE;
  
  felem[8].text = (char*)"edia";
 
  felem[9].newLine = 1;
  felem[9].lineSpacing = 8;
  felem[9].text = (char*)"http://i.tny.im | http://gbl08ma.com";
  
  felem[10].newLine = 1;
  felem[10].text = (char*)"gbl08ma@gmail.com";
  
  felem[11].newLine = 1;
  felem[11].lineSpacing = 15;
  felem[11].text = (char*)"Press any key";
  
  ftext.numelements = 12;
  doTextArea(&ftext);
  drawtnyimLogo(10, 59+24); //24 pixels for the status bar
  mGetKey(&key);
  
  textArea text;
  text.showtitle=0;

  textElement elem[5];
  text.elements = elem;
  
  elem[0].text = (char*)"Contains code by AHelper, merthsoft and KermMartian at Cemetech (http://cemetech.net), by Simon Lothar (http://casiopeia.net) and public domain code by C.B. Falconer. Uses the Heatshrink library by Scott Vokes.";
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
}

void buildExpiredMessage() {
  textArea text;
  strcpy(text.title, (char*)"Check for updates");
  
  textElement elem[5];
  text.elements = elem;
  text.scrollbar = 0;
  
  elem[0].text = (char*)"A new version of Utilities has probably been released by now.";
  elem[1].newLine = 1;
  elem[1].text = (char*)"Please check for updates at the following URL:";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 5;
  elem[2].color=COLOR_BLUE;
  elem[2].text = (char*)"http://tny.im/utupd";
  elem[3].newLine = 1;
  elem[3].lineSpacing = 16;
  elem[3].text = (char*)"Press EXIT to continue.";
  
  text.numelements = 4;
  doTextArea(&text);
}