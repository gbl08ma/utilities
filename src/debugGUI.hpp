#ifndef __DEBUGGUI_H
#define __DEBUGGUI_H

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

int getDebugMode();
void setDebugMode(int val);

void debugMessage(char* text1, char* text2, int value);
void setInitStackPtr(int val);
void showRAMused();
//void showRAMusedStatus();
void masterControl();

#endif