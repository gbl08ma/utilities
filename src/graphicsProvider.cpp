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

#include "graphicsProvider.hpp"
#include "settingsProvider.hpp"
#include "stringsProvider.hpp"

void drawRectangle(int x, int y, int width, int height, unsigned short color) {
	unsigned short*s=(unsigned short*)0xA8000000;
	s+=(y*384)+x;
	while(height--){
		for(int w=width;w--;)
			*s++=color;
		s+=384-width;
	}
}

// Algorithm found on:
// http://members.chello.at/easyfilter/bresenham.html
void drawLine(int x0, int y0, int x1, int y1, int color) {
   int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
   int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
   int err = dx+dy;                                           /* error value e_xy */
                                                    
   for (;;) {                                                 /* loop */
      plot(x0,y0,color);                              
      int e2 = 2*err;                                   
      if (e2 >= dy) {                                         /* e_xy+e_x > 0 */
         if (x0 == x1) break;                       
         err += dy; x0 += sx;                       
      }                                             
      if (e2 <= dx) {                                         /* e_xy+e_y < 0 */
         if (y0 == y1) break;
         err += dx; y0 += sy;
      }
   }
}

// Wikipedia/Bresenham 
void drawFilledCircle(int centerx, int centery, int radius, color_t color) { 
  int f = 1 - radius;
  int ddF_x = 1;
  int ddF_y = -2 * radius;
  int x = 0;
  int y = radius;
  
  drawLine(centerx, centery + radius, centerx, centery - radius, color);
  drawLine(centerx + radius, centery, centerx - radius, centery, color);
  
  while(x < y) {
    if(f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    drawLine(centerx + x, centery + y, centerx - x, centery + y, color);
    drawLine(centerx + x, centery - y, centerx - x, centery - y, color);
    drawLine(centerx + y, centery + x, centerx - y, centery + x, color);
    drawLine(centerx + y, centery - x, centerx - y, centery - x, color);
  } 
}

void drawCircularCountdownIndicator(int centerx, int centery, int radius, color_t colorfg, color_t colorbg, int inner, int swap) {
  drawFilledCircle(centerx, centery, radius, colorfg);
  drawFilledCircle(centerx, centery, radius-2, swap ? colorfg : colorbg);
  drawFilledCircle(centerx, centery, inner, swap ? colorbg : colorfg);
}

//ReplaceColor By Kerm:
void replaceColorInArea(int x, int y, int width, int height, color_t color_old, color_t color_new) { 
   //color_t* VRAM = GetVRAMAddress();
   color_t* VRAM = (color_t*)0xA8000000; 
   VRAM += (y*LCD_WIDTH_PX)+x; 
   for(int j=0; j<height; VRAM += (LCD_WIDTH_PX-width), j++) { 
      for(int i=0; i<width; VRAM++, i++) { 
         if (*VRAM == color_old) *VRAM = color_new; 
      } 
   } 
}

void invertArea(short x, short y, short width, short height) {
   color_t* VRAM = (color_t*)0xA8000000; 
   VRAM += (y*LCD_WIDTH_PX)+x; 
   for(int j=0; j<height; VRAM += (LCD_WIDTH_PX-width), j++) { 
      for(int i=0; i<width; VRAM++, i++) { 
         *VRAM ^= 0xFFFF;
      } 
   } 
}

void darkenStatusbar() {
  if(getSetting(SETTING_DISPLAY_STATUSBAR)) {
    replaceColorInArea(0, 0, LCD_WIDTH_PX, 24, COLOR_BLACK, COLOR_GRAY);
    replaceColorInArea(0, 0, LCD_WIDTH_PX, 24, COLOR_WHITE, COLOR_BLACK);
    replaceColorInArea(0, 0, LCD_WIDTH_PX, 24, COLOR_BLUE, COLOR_ORANGE);
  }
}
void darkenFkeys(int numkeys) {
  replaceColorInArea(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_BLACK, COLOR_GRAY);
  replaceColorInArea(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_WHITE, COLOR_BLACK);
}
void drawArrowDown(int bottomX, int bottomY, int color) {
  drawLine(bottomX-7,bottomY-7,bottomX,bottomY,color);
  drawLine(bottomX-8,bottomY-7,bottomX,bottomY+1,color); //double thickness of line
  drawLine(bottomX+1,bottomY,bottomX+8,bottomY-7,color);
  drawLine(bottomX+1,bottomY+1,bottomX+9,bottomY-7,color); //double thickness of line
}
void drawFkeyPopup(int Fkey, const char* title) {
//draws a big popup pointing to a Fkey (index zero based, F1 = 0, F6 = 5) with the selected color scheme.
// PrintXY text inside the popup starts at X=2 and Y=2
  int fgcolor = COLOR_BLACK;
  int bgcolor = COLOR_WHITE;
  if (getSetting(SETTING_THEME)) {
    fgcolor = COLOR_WHITE;
    bgcolor = COLOR_BLACK;
  }
#define FKEY_C1X 18
#define FKEY_C1Y 24*2
#define FKEY_C2X 18*20
#define FKEY_C2Y 24*2
#define FKEY_C3X 18*20
#define FKEY_C3Y 24*8-12
  drawRectangle(FKEY_C1X-2, FKEY_C1Y-2, FKEY_C3X-FKEY_C1X+4, FKEY_C3Y-FKEY_C1X-25, fgcolor);
  drawRectangle(FKEY_C1X, FKEY_C1Y, FKEY_C3X-FKEY_C1X, FKEY_C3Y-FKEY_C1X-29, bgcolor);

  drawLine(FKEY_C1X, FKEY_C1Y+23, FKEY_C2X-1, FKEY_C2Y+23, COLOR_GRAY);
  drawRectangle(FKEY_C1X, FKEY_C1Y, FKEY_C2X-FKEY_C1X, 23, COLOR_LIGHTGRAY);

  drawArrowDown(31+Fkey*64, 190, fgcolor);
  drawLine(23+Fkey*64, 181, 40+Fkey*64, 181, bgcolor);
  drawLine(24+Fkey*64, 182, 39+Fkey*64, 182, bgcolor);

  int textX = FKEY_C3X-111-4;
  int textY = FKEY_C3Y-14-20;
  PrintMiniMini( &textX, &textY, "...or press: [EXIT]", (getSetting(SETTING_THEME) == 1 ? 4 : 0), TEXT_COLOR_BLACK, 0 ); //draw

  mPrintXY(2, 2, title, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
}
void CopySpriteMasked(const unsigned short* data, int x, int y, int width, int height, unsigned short maskcolor) {
  unsigned short* VRAM = (unsigned short*)0xA8000000; 
  VRAM += (LCD_WIDTH_PX*y + x); 
  while(height--) {
    int i=width;
    while(i--){
      if(*data!=maskcolor) {
        *(VRAM++) = *(data++);
      } else {
        ++VRAM;
        ++data;
      }
    }
    VRAM += (LCD_WIDTH_PX-width);
  }
}

void CopySpriteNbitMasked(const unsigned char* data, int x, int y, int width, int height, const color_t* palette, color_t maskColor, unsigned int bitwidth) {
  color_t* VRAM = (color_t*)0xA8000000;
  VRAM += (LCD_WIDTH_PX*y + x);
  int offset = 0;
  unsigned char buf = 0;
  for(int j=y; j<y+height; j++) {
    int availbits = 0;
    for(int i=x; i<x+width;  i++) {
      if (!availbits) {
        buf = data[offset++];
        availbits = 8;
      }
      color_t thisthis = ((color_t)buf>>(8-bitwidth));
      color_t color = palette[(color_t)thisthis];
      if(color != maskColor) *VRAM = color;
      VRAM++;
      buf<<=bitwidth;
      availbits-=bitwidth;
    }
    VRAM += (LCD_WIDTH_PX-width);
  }
}

//the following does not update the screen automatically; it will draw the tny.im logo starting at screen coordinates x,y
//the tny.im logo is great enough not to require any sprites! yay!
//w:138
//h:42
static const unsigned char logoB[]={
//draw t
0, 6, 6, 24,
6, 12, 6, 6,
6, 30, 6, 6,
//draw n
18, 12, 6, 24,
24, 12, 12, 6,
36, 18, 6, 18,
//draw y
48, 12, 6, 18,
60, 12, 6, 18,
54, 30, 6, 6,
48, 36, 6, 6,
//draw dot
72, 30, 6, 6 };
static const unsigned char logoO[]={
//draw i (orange)
84, 0, 6, 6,
84, 12, 6, 24,
//draw m (orange)
96, 12, 6, 24,
102, 12, 12, 6,
114, 18, 6, 18,
120, 12, 12, 6,
132, 18, 6, 18 };
void drawtnyimLogo(int x, int y) {
  int i;
  for(i=0;i<11*4;i+=4)
    drawRectangle(x+logoB[i], y+logoB[i+1], logoB[i+2], logoB[i+3], COLOR_BLACK);
  for(i=0;i<7*4;i+=4)
    drawRectangle(x+logoO[i], y+logoO[i+1], logoO[i+2], logoO[i+3], TNYIM_ORANGE);
}

static const color_t fullColorsForText[] =
{COLOR_BLACK, COLOR_BLUE, COLOR_LIMEGREEN, COLOR_CYAN, COLOR_RED,
  COLOR_MAGENTA, COLOR_YELLOW, COLOR_LIGHTGRAY};
int textColorToFullColor(int textcolor) {
  return fullColorsForText[textcolor % sizeof(fullColorsForText)];
}

void progressMessage(const char* message, int cur, int total) {
  // when you're done with this, close with closeProgressMessage!
  // otherwise, resources won't be freed.
  // always call with cur==0 and total > 0 first, so that the progressbar is initialized!
  // if message is empty, the default OS "please wait" message will be used.
  if((!message)||(!message[0])) {
    ProgressBar( cur, total );
  } else {
    ProgressBar2( (unsigned char*)message, cur, total );
  }
}
void closeProgressMessage() {
  MsgBoxPop(); //closes progressbar
}

void printCentered(char* text, int y, int FGC, int BGC) {
  int len = MB_ElementCount(text);
  int x = LCD_WIDTH_PX/2-(len*18)/2;
  PrintCXY(x, y, text, 0x40, -1, FGC, BGC, 1, 0 );
}

void clearLine(int x, int y, color_t color) {
  // clear text line. x and y are text cursor coordinates
  // this is meant to achieve the same effect as using PrintXY with a line full of spaces (except it doesn't waste strings).
  int width=LCD_WIDTH_PX;
  if(x>1) width = 24*(21-x);
  drawRectangle((x-1)*18, y*24, width, 24, color);
}

void mPrintXY(int x, int y,const char*msg, int mode, int color) {
  char nmsg[50];
  nmsg[0] = 0x20;
  nmsg[1] = 0x20;
  strncpy(nmsg+2, msg, 48);
  PrintXY(x, y, nmsg, mode, color );
}

void multiPrintXY(int x, int y, const char* msg, int mode, int color) {
  char token[50];
  while(*msg) {
    msg = toksplit((char*)msg, '\n', token, 50);
    mPrintXY(x, y++, token, mode, color);
  }
}

void multiPrintMini(int x, int y, const char* msg, int fgcolor) {
  char token[100];
  while(*msg) {
    msg = toksplit((char*)msg, '\n', token, 100);
    PrintMini(&x, &y, token, 0x02, 0xFFFFFFFF, 0, 0, fgcolor, COLOR_WHITE, 1, 0);
    x=0; y += 17;
  }
}

static int numberOfMsgBoxPushed = 0;
// progressMessage and closeProgressMessage do not count towards this number even though they push and pop MsgBoxes
// this is because mGetKey can't be used to "restart" the add-in while a progressbar is shown
void mMsgBoxPush(int lines) {
  MsgBoxPush(lines);
  numberOfMsgBoxPushed++;
}

void mMsgBoxPop() {
  MsgBoxPop();
  numberOfMsgBoxPushed--;
}

// useful when "restarting" the add-in
void popAllMsgBoxes() {
  while(0<numberOfMsgBoxPushed) {
    mMsgBoxPop();
  }
}
int getMsgBoxCount() {
  return numberOfMsgBoxPushed;
}

void drawScreenTitle(const char* title, const char* subtitle) {
  if(title != NULL) mPrintXY(1, 1, title, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  if(subtitle != NULL) mPrintXY(1, 2, subtitle, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
}

void drawFkeyLabels(int f1, int f2, int f3, int f4, int f5, int f6) {
  // set arguments to negative numbers if that fkey is not to be affected.
  int f[] = {f1, f2, f3, f4, f5, f6};
  int iresult;
  for(int i = 0; i < 6; i++) {
    if(f[i] >= 0) {
      GetFKeyPtr(f[i], &iresult);
      FKey_Display(i, (int*)iresult);
    }
  }
}

int drawRGB24toRGB565(int r, int g, int b) {  
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);  
}

#define PISQUARED 9.869604401089358618834
double sine(double x) { 
  while (x > M_PI) x-=(2.f*M_PI); 
  while (x < -M_PI) x+=(2.f*M_PI); 
  const double B = 4/M_PI; 
  const double C = -4/(PISQUARED); 

  double y = B * x + C * x * fabs(x); 

  //  const float Q = 0.7775; 
  const double P = 0.224008178776; 

  y = P * (y * fabs(y) - y) + y;   // Q * y + P * y * abs(y) 
  
  return y; 
} 

int ipow(int base, int exp) {
  int result = 1;
  while (exp) {
    if (exp & 1) result *= base;
    exp >>= 1;
    base *= base;
  }
  return result;
}