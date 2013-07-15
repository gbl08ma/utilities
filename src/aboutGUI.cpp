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
 Bdisp_EnableColor(1);
 DisplayStatusArea();
 textY = 5;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"Contains code by AHelper, merthsoft", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 22;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"and KermMartian at Cemetech", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 39;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"(http://cemetech.net), code by Simon", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 56;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"Lothar and public domain code by", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 73;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"C.B. Falconer.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 101;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"This Utilities add-in is licensed", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 118;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"under the GNU GPL v2, or at your", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 135;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"choice, any later version.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

 textX = 0; textY = 175;
 PrintMini(&textX, &textY, (unsigned char*)"Press any key", 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
 GetKey(&key);

 Bdisp_AllClr_VRAM();
 Bdisp_EnableColor(1);
 DisplayStatusArea();
 textY = 5;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"USE AT YOUR OWN RISK!", 0, 0xFFFFFFFF, 0, 0, COLOR_RED, COLOR_WHITE, 1, 0);
 textY = 22;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"PROVIDED WITHOUT WARRANTY!", 0, 0xFFFFFFFF, 0, 0, COLOR_RED, COLOR_WHITE, 1, 0);
 textY = 52;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"In no event will the authors be held", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 69;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"liable for any damages arising from", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 textY = 86;
 textX = 0;
 PrintMini(&textX, &textY, (unsigned char*)"the use of this software.", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 
 textY = 118;
 textX = 0;
 char buffer[10] = "";
 getHardwareID(buffer);
 PrintMini(&textX, &textY, (unsigned char*)"This device's ID:", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 PrintMini(&textX, &textY, (unsigned char*)" ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
 PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

 textX = 0; textY = 175;
 PrintMini(&textX, &textY, (unsigned char*)"Press any key", 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
 GetKey(&key);
 DefineStatusMessage((char*)"", 1, 0, 0);
}