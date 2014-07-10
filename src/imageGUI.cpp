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

#include "imageGUI.hpp"
#include "menuGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "tjpgd.h"

void viewImage(char* filename) {
  void *work;     /* Pointer to the decompressor work area */
  JDEC jdec;    /* Decompression object */
  JRESULT res;    /* Result code of TJpgDec API */
  IODEV devid;    /* User defined device identifier */

  /* Allocate a work area for TJpgDec */
  //work = alloca(3100*2);
  work = (char*)0xE5007000; // first "additional on-chip RAM area", 8192 bytes

  devid.xoff = 0;
  devid.yoff = 0;
  int scale = 0;
  unsigned short filenameshort[0x10A];
  Bfile_StrToName_ncpy(filenameshort, (unsigned char*)filename, 0x10A);
  while(1) {
    Bdisp_AllClr_VRAM();
    /* Open a JPEG file */
    devid.fp = Bfile_OpenFile_OS(filenameshort, READWRITE, 0);
    if (devid.fp < 0) return /*-1*/;

    /* Prepare to decompress */
    res = jd_prepare(&jdec, in_func, work, 8100, &devid);
    if (res == JDR_OK) {
      /* Ready to dcompress. Image info is available here. */
      //printf("Image dimensions: %u by %u. %u bytes used.\n", jdec.width, jdec.height, 3100 - jdec.sz_pool);

      res = jd_decomp(&jdec, out_func, scale);   /* Start to decompress with set scaling */
      if (res == JDR_OK) {
        /* Decompression succeeded. You have the decompressed image in the frame buffer here. */
        //printf("\rOK  \n");

      } else {
        //printf("Failed to decompress: rc=%d\n", res);
      }

    } else {
      //printf("Failed to prepare: rc=%d\n", res);
    }

    Bfile_CloseFile_OS(devid.fp);     /* Close the JPEG file */

    int key; 
    EnableStatusArea(3);
    GetKey(&key);
    EnableStatusArea(0);
    switch(key) {
      case KEY_CTRL_EXIT:
        return;
      case KEY_CTRL_DOWN:
        devid.yoff += 64;
        break;
      case KEY_CTRL_UP:
        devid.yoff -= 64;
        break;
      case KEY_CTRL_RIGHT:
        devid.xoff += 64;
        break;
      case KEY_CTRL_LEFT:
        devid.xoff -= 64;
        break;
      case KEY_CHAR_2:
      case KEY_CTRL_PAGEDOWN:
        devid.yoff += LCD_HEIGHT_PX;
        break;
      case KEY_CHAR_8:
      case KEY_CTRL_PAGEUP:
        devid.yoff -= LCD_HEIGHT_PX;
        break;
      case KEY_CHAR_6:
        devid.xoff += LCD_WIDTH_PX;
        break;
      case KEY_CHAR_4:
        devid.xoff -= LCD_WIDTH_PX;
        break;
      case KEY_CHAR_MINUS:
        if(scale < 3) {
          scale++;
          devid.xoff /= 2;
          devid.yoff /= 2;
        }
        break;
      case KEY_CHAR_PLUS:
        if(scale > 0) {
          scale--;
          devid.xoff *= 2;
          devid.yoff *= 2;
        }
        break;
    }
  }
}

UINT in_func (JDEC* jd, BYTE* buff, UINT nbyte)
{
  IODEV *dev = (IODEV*)jd->device;   /* Device identifier for the session (5th argument of jd_prepare function) */


  if (buff) {
  /* Read bytes from input stream */
  return Bfile_ReadFile_OS(dev->fp, buff, nbyte, -1);
  } else {
  /* Remove bytes from input stream */
  int br = Bfile_SeekFile_OS(dev->fp, Bfile_TellFile_OS(dev->fp)+nbyte);
  if(br < 0) return 0;
  else return nbyte;
  }
}

UINT out_func (JDEC* jd, void* bitmap, JRECT* rect)
{
  IODEV *dev = (IODEV*)jd->device;
  /* Put progress indicator */
  /*if (rect->left == 0) {
    printf("\r%lu%%", (rect->top << jd->scale) * 100UL / jd->height);
  }*/
  TDispGraph Graph;
  Graph.x = rect->left - dev->xoff;
  Graph.y = rect->top - dev->yoff; //24 px for status bar...
  if(Graph.x > LCD_WIDTH_PX) return 1; // out of bounds, no need to display
  if(Graph.y > LCD_HEIGHT_PX) return 0; // no need to decode more, it's already out of vertical bound
  Graph.xofs = 0;
  Graph.yofs = 0;
  Graph.width = rect->right - rect->left + 1;
  Graph.height = rect->bottom - rect->top + 1;
  Graph.colormode = 0x02;
  Graph.zero4 = 0x00;
  Graph.P20_1 = 0x20;
  Graph.P20_2 = 0x20;
  Graph.bitmap = (int)bitmap;
  Graph.color_idx1 = 0x00;
  Graph.color_idx2 = 0x05;
  Graph.color_idx3 = 0x01;
  Graph.P20_3 = 0x01;
  Graph.writemodify = 0x01;
  Graph.writekind = 0x00;
  Graph.zero6 = 0x00;
  Graph.one1 = 0x00;
  Graph.transparency = 0;
  Bdisp_WriteGraphVRAM( &Graph );

  return 1;  /* Continue to decompress */
}