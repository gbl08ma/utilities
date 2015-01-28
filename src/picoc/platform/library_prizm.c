#include "../interpreter.h"
#include "../picoc.h"

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

void PrizmSetupFunc()
{    
}
extern void PlatformLibraryInit_cpp();
void PlatformLibraryInit()
{
    PlatformLibraryInit_cpp();
}

