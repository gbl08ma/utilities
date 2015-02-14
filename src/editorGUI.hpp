#ifndef __EDITORGUI_H
#define __EDITORGUI_H

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

#define TEXT_BUFFER_SIZE 128*1024 /* Make sure linker script is set to 512kb of ram */

void fileTextEditor(char* filename=NULL, char* basefolder=NULL);

#endif 
