#ifndef __IMAGEGUI_H
#define __IMAGEGUI_H

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
#include "tjpgd.h"

typedef struct {
  int fp;        /* File pointer for input function */
  INT xoff;    /* Image cursor */
  INT yoff;    /* Image cursor */
} IODEV;
void viewImage(char* filename);
UINT in_func (JDEC* jd, BYTE* buff, UINT nbyte);
UINT out_func (JDEC* jd, void* bitmap, JRECT* rect);

#endif