#ifndef __LOCKGUI_H
#define __LOCKGUI_H

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
#include <setjmp.h>
int passwordInput(int x, int y, unsigned char* buffer);
int setPassword();
int unlockCalc();
int lockCalc();
void lockApp();
extern jmp_buf utilities_return;
void openRunMat();
void returnToUtilitiesHandler();
#endif