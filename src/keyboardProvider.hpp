#ifndef __KEYBOARDPROVIDER_H
#define __KEYBOARDPROVIDER_H

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

#define MGETKEY_RETURN_KEY 0
#define MGETKEY_RETURN_INTOSETTINGS 1

#define MGETKEY_MODE_NORMAL 0
#define MGETKEY_MODE_RESTRICT_SETTINGS 1
#define MGETKEY_MODE_RESTRICT_RESTART 2 // when set, user can't jump to home with Shift+Exit
#define MGETKEY_MODE_RESTRICT_SETTINGS_RESTART 3 // same as above but user can't open settings either.
int mGetKey(int* key);
void setmGetKeyMode(int mode);
unsigned int SetGetkeyToMainFunctionReturnFlag( unsigned int enabled );

#endif