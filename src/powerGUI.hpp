#ifndef __POWERGUI_H
#define __POWERGUI_H

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

void setPoweroffTimeout();
void setBacklightTimeout();
void setBacklightLevel();
void powerInformation();
int getPLLinfo(unsigned int PLL, char** freqstr, char** statusstr, int* color);
void updateCurrentFreq();
void setCPUclock();

#endif