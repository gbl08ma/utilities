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
#include "fileProvider.hpp"
#include "tjpgd.h"

void viewImage(char* filename) {
  void *work;     /* Pointer to the decompressor work area */
  JDEC jdec;    /* Decompression object */
  IODEV devid;    /* User defined device identifier */

  /* Allocate a work area for TJpgDec */
  work = (char*)0xE5007000; // first "additional on-chip RAM area", 8192 bytes

  devid.xoff = 0;
  devid.yoff = 0;

  // set constant parameters for struct to pass to Bdisp_WriteGraphVRAM
  devid.Graph.xofs = 0;
  devid.Graph.yofs = 0;
  devid.Graph.colormode = 0x02;
  devid.Graph.zero4 = 0x00;
  devid.Graph.P20_1 = 0x20;
  devid.Graph.P20_2 = 0x20;
  devid.Graph.color_idx1 = 0x00;
  devid.Graph.color_idx2 = 0x05;
  devid.Graph.color_idx3 = 0x01;
  devid.Graph.P20_3 = 0x01;
  devid.Graph.writemodify = 0x01;
  devid.Graph.writekind = 0x00;
  devid.Graph.zero6 = 0x00;
  devid.Graph.one1 = 0x00;
  devid.Graph.transparency = 0;
  
  int scale = 0;
  while(1) {
    int key;
    Bdisp_AllClr_VRAM();
    /* Open a JPEG file */
    devid.fp = fileOpen(filename);
    if (devid.fp < 0) {
      AUX_DisplayErrorMessage(0x2B);
      return;
    }

    /* Prepare to decompress */
    JRESULT res = jd_prepare(&jdec, in_func, work, 8100, &devid);
    if (res == JDR_OK) {
      /* Ready to decompress. Image info is available here. */
      int sdiv = ipow(2, scale);
      if(jdec.width/sdiv < LCD_WIDTH_PX) {
        devid.xoff = -(LCD_WIDTH_PX/2 - (jdec.width/sdiv)/2);
      } else {
        if(devid.xoff<0) devid.xoff = 0;
        if(devid.xoff>(int)jdec.width/sdiv-LCD_WIDTH_PX) devid.xoff=jdec.width/sdiv-LCD_WIDTH_PX;
      }
      if(jdec.height/sdiv < LCD_HEIGHT_PX) {
        devid.yoff = -(LCD_HEIGHT_PX/2 - (jdec.height/sdiv)/2);
      } else {
        if(devid.yoff<0) devid.yoff = 0;
        if(devid.yoff>(int)jdec.height/sdiv-LCD_HEIGHT_PX) devid.yoff=jdec.height/sdiv-LCD_HEIGHT_PX;
      }

      res = jd_decomp(&jdec, out_func, scale);   /* Start to decompress with set scaling */
      if (res == JDR_OK || res == JDR_INTR) {
        /* Decompression succeeded. You have the decompressed image in the frame buffer here. */

      } else {
        Bfile_CloseFile_OS(devid.fp);
        mMsgBoxPush(4);
        multiPrintXY(3, 2, "An error occurred\n(failed to\ndecompress)", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        closeMsgBox();
        return;
      }

    } else {
      Bfile_CloseFile_OS(devid.fp);
      mMsgBoxPush(4);
      multiPrintXY(3, 2, "An error occurred\n(failed to\nprepare)", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      closeMsgBox();
      return;
    }

    Bfile_CloseFile_OS(devid.fp);     /* Close the JPEG file */

    int inkeyloop = 1;
    EnableStatusArea(3);
    while(inkeyloop) {
      GetKey(&key);
      switch(key) {
        case KEY_CTRL_EXIT:
          EnableStatusArea(0);
          return;
        case KEY_CTRL_DOWN:
          devid.yoff += 64;
          inkeyloop = 0;
          break;
        case KEY_CTRL_UP:
          devid.yoff -= 64;
          inkeyloop = 0;
          break;
        case KEY_CTRL_RIGHT:
          devid.xoff += 64;
          inkeyloop = 0;
          break;
        case KEY_CTRL_LEFT:
          devid.xoff -= 64;
          inkeyloop = 0;
          break;
        case KEY_CHAR_2:
          devid.yoff += LCD_HEIGHT_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_8:
          devid.yoff -= LCD_HEIGHT_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_6:
          devid.xoff += LCD_WIDTH_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_4:
          devid.xoff -= LCD_WIDTH_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_7:
          devid.xoff -= LCD_WIDTH_PX;
          devid.yoff -= LCD_HEIGHT_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_9:
          devid.xoff += LCD_WIDTH_PX;
          devid.yoff -= LCD_HEIGHT_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_1:
          devid.xoff -= LCD_WIDTH_PX;
          devid.yoff += LCD_HEIGHT_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_3:
          devid.xoff += LCD_WIDTH_PX;
          devid.yoff += LCD_HEIGHT_PX;
          inkeyloop = 0;
          break;
        case KEY_CHAR_MINUS:
          if(scale < 3) {
            scale++;
            devid.xoff /= 2;
            devid.yoff /= 2;
            inkeyloop = 0;
          }
          break;
        case KEY_CHAR_PLUS:
          if(scale > 0) {
            scale--;
            devid.xoff *= 2;
            devid.yoff *= 2;
            inkeyloop = 0;
          }
          break;
      }
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
  dev->Graph.x = rect->left - dev->xoff;
  dev->Graph.y = rect->top - dev->yoff; //24 px for status bar...
  if(dev->Graph.x > LCD_WIDTH_PX) return 1; // out of bounds, no need to display
  if(dev->Graph.y > LCD_HEIGHT_PX) return 0; // no need to decode more, it's already out of vertical bound
  dev->Graph.width = rect->right - rect->left + 1;
  dev->Graph.height = rect->bottom - rect->top + 1;
  dev->Graph.bitmap = (int)bitmap;
  Bdisp_WriteGraphVRAM( &dev->Graph );

  return 1;  /* Continue to decompress */
}