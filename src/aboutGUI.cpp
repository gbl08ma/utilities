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
#include "keyboardProvider.hpp"
#include "settingsProvider.hpp"

void aboutScreen() {
  int key;
  DefineStatusMessage((char*)"About " ADDIN_FRIENDLYNAME, 1, 0, 0);
  
  textArea ftext;
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
  felem[9].text = (char*)"http://i.tny.im | http://gbl08ma.com\ngbl08ma@gmail.com";
  
  felem[10].newLine = 1;
  felem[10].lineSpacing = 15;
  felem[10].text = (char*)"Press any key";
  
  ftext.numelements = 11;
  doTextArea(&ftext);
  drawtnyimLogo(10, 59+24); //24 pixels for the status bar
  mGetKey(&key);
  
  textArea text;

  #ifdef ENABLE_PICOC_SUPPORT
  textElement elem[7];
  #else
  textElement elem[5];
  #endif
  text.elements = elem;
  
  elem[0].text = (char*)"Contains code by AHelper, Merthsoft and KermMartian at Cemetech "
                        "(http://cemetech.net), by Simon Lothar and public domain code by "
                        "C.B. Falconer. Uses the Heatshrink library by Scott Vokes. Uses TJpgDec "
                        "- (C)ChaN, 2012.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)ADDIN_FRIENDLYNAME " is licensed under the GNU GPL v2, or (at your option) "
                        "any later version.";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 8;
  elem[2].color = COLOR_RED;
  elem[2].text = (char*)"USE AT YOUR OWN RISK!\nPROVIDED WITHOUT WARRANTY!";
  elem[3].newLine = 1;
  elem[3].lineSpacing = 8;
  elem[3].text = (char*)"In no event will the authors be held liable for any damages arising from "
                        "the use of this software.";
  text.numelements = 4;

#ifdef ENABLE_PICOC_SUPPORT
  elem[4].newLine = 1;
  elem[4].lineSpacing = 8;
  elem[4].color = COLOR_BLUE;
  elem[4].text = (char*)"Licenses for built-in included software:";

  elem[5].newLine = 1;
  elem[5].text = (char*)"PicoC - C Interpreter\n"
  "Copyright (c) 2009-2011, Zik Saleeba\n"
  "All rights reserved.\n"
  "Redistribution and use in source and binary forms, with or without modification, are permitted "
  "provided that the following conditions are met:\n"
  "1. Redistributions of source code must retain the above copyright notice, this list of "
  "conditions and the following disclaimer.\n"
  "2. Redistributions in binary form must reproduce the above copyright notice, this list of "
  "conditions and the following disclaimer in the documentation and/or other materials provided "
  "with the distribution.\n"
  "3. Neither the name of the project nor the names of its contributors may be used to endorse or "
  "promote products derived from this software without specific prior written permission.\n"
  "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR"
  " IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY "
  "AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. "
  "IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, "
  "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, "
  "PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; "
  "LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF "
  "LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) "
  "ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH "
  "DAMAGE.";
  text.numelements = 6;
#endif
  doTextArea(&text);
}

void buildExpiredScreen() {
  unsigned char s = getSetting(SETTING_ACTIVATION_STATUS);
  // to understand the following code, see the description of the setting on settingsProvider.hpp
  if(s > 1)  {
    setSetting(SETTING_ACTIVATION_STATUS, --s, 1);
    return;
  }
  textArea text;
  text.title = (char*)"Check for updates";
  
  textElement elem[3];
  text.elements = elem;
  text.scrollbar = 0;
  text.allowF1 = 1;
  
  elem[0].text = (char*)"A new version of " ADDIN_FRIENDLYNAME " may have been released by now.\n"
                        "Please check for updates at the following URL:";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 5;
  elem[1].color=COLOR_BLUE;
  elem[1].text = (char*)"http://tny.im/utupd";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 16;
  elem[2].text = (char*)"Press EXIT to continue, or F1 to temporarily stop this message from"
                        "showing.";
  
  text.numelements = 3;
  if(doTextArea(&text) == TEXTAREA_RETURN_F1) {    
    setSetting(SETTING_ACTIVATION_STATUS, 255, 1);
  }
}