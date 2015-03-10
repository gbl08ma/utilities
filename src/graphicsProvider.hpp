#ifndef __GRAPHICSPROVIDER_H
#define __GRAPHICSPROVIDER_H
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

#define TNYIM_ORANGE 0xd222
#define plot(x,y,c) Bdisp_SetPoint_VRAM(x, y, c)
void drawRectangle(int x, int y, int width, int height, unsigned short color);
void drawLine(int x1, int y1, int x2, int y2, int color);
void drawFilledCircle(int centerx, int centery, int radius, color_t color);
void drawCircularCountdownIndicator(int centerx, int centery, int radius, color_t colorfg, color_t colorbg, int inner, int swap);
void VRAMReplaceColorInRect(int x, int y, int width, int height, color_t color_old, color_t color_new);
void VRAMInvertArea(short x, short y, short width, short height);
void darkenStatusbar();
void darkenFkeys(int numkeys);
void drawArrowDown(int bottomX, int bottomY, int color);
void drawFkeyPopup(int Fkey, const char* title);
void CopySpriteMasked(const unsigned short* data, int x, int y, int width, int height, unsigned short maskcolor);
void CopySpriteNbitMasked(const unsigned char* data, int x, int y, int width, int height, const color_t* palette, color_t maskColor, unsigned int bitwidth);
void drawtnyimLogo(int x, int y);
int textColorToFullColor(int textcolor);
void progressMessage(const char* message, int cur, int total);
void closeProgressMessage();
void printCentered(char* text, int y, int FGC, int BGG);
void clearLine(int x, int y, color_t color=COLOR_WHITE);
void mPrintXY(int x, int y, const char*msg, int mode, int color);
void multiPrintXY(int x, int y, const char* msg, int mode, int color);
void mMsgBoxPush(int lines);
void mMsgBoxPop();
void popAllMsgBoxes();
int getNumberOfMsgBoxPushed();
void drawScreenTitle(const char* title, const char* subtitle = NULL);
void drawFkeyLabels(int f1=-1, int f2=-1, int f3=-1, int f4=-1, int f5=-1, int f6=-1);
int drawRGB24toRGB565(int r, int g, int b);

// needed for analog clock and some other... circular things:
double sine(double x);
//double cosine(double x);
#define cosine(x) sine((M_PI_2)+(x)) 

#endif
