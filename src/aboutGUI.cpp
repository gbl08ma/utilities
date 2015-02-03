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
#include "settingsProvider.hpp"

void showAbout() {
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

  #ifdef ENABLE_PICOC_SUPPORT
  textElement elem[17];
  #else
  textElement elem[5];
  #endif
  text.elements = elem;
  
  elem[0].text = (char*)"Contains code by AHelper, merthsoft and KermMartian at Cemetech (http://cemetech.net), by Simon Lothar (http://casiopeia.net) and public domain code by C.B. Falconer. Uses the Heatshrink library by Scott Vokes. Uses TJpgDec - (C)ChaN, 2012.";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 8;
  elem[1].text = (char*)"This " ADDIN_FRIENDLYNAME " add-in is licensed under the GNU GPL v2, or (at your option) any later version.";
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

  #ifdef ENABLE_PICOC_SUPPORT
  elem[5].newLine = 1;
  elem[5].lineSpacing = 8;
  elem[5].color = COLOR_BLUE;
  elem[5].text = (char*)"Licenses for built-in included software:";

  elem[6].newLine = 1;
  elem[6].text = (char*)"PicoC - C Interpreter";

  elem[7].newLine = 1;
  elem[7].text = (char*)"Copyright (c) 2009-2011, Zik Saleeba";

  elem[8].newLine = 1;
  elem[8].text = (char*)"All rights reserved.";

  elem[9].newLine = 1;
  elem[9].text = (char*)"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:";
  
  elem[10].newLine = 1;
  elem[10].text = (char*)"1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.";
  
  elem[11].newLine = 1;
  elem[11].text = (char*)"2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.";
  
  elem[12].newLine = 1;
  elem[12].text = (char*)"3. Neither the name of the project nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.";
  
  elem[13].newLine = 1;
  elem[13].text = (char*)"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.";
  elem[13].spaceAtEnd = 1;
  elem[14].text = (char*)"IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;";
  elem[14].spaceAtEnd = 1;
  elem[15].text = (char*)"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,";
  elem[15].spaceAtEnd = 1;
  elem[16].text = (char*)"EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";
  text.numelements = 17;
  #endif
  doTextArea(&text);
}

void buildExpiredMessage() {
  if(GetSetting(SETTING_IS_FIRST_RUN) == 2) return;
  textArea text;
  text.title = (char*)"Check for updates";
  
  textElement elem[5];
  text.elements = elem;
  text.scrollbar = 0;
  text.allowF1 = 1;
  
  elem[0].text = (char*)"A new version of " ADDIN_FRIENDLYNAME " may have been released by now.";
  elem[1].newLine = 1;
  elem[1].text = (char*)"Please check for updates at the following URL:";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 5;
  elem[2].color=COLOR_BLUE;
  elem[2].text = (char*)"http://tny.im/utupd";
  elem[3].newLine = 1;
  elem[3].lineSpacing = 16;
  elem[3].text = (char*)"Press EXIT to continue, or F1 if you don't want to see this message again.";
  
  text.numelements = 4;
  if(doTextArea(&text) == TEXTAREA_RETURN_F1) {
    SetSetting(SETTING_IS_FIRST_RUN, 2, 1);
  }
}