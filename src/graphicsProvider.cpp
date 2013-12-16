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

const short empty[18] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int PrintMiniFix( int x, int y, const char*Msg, const int flags, const short color, const short bcolor )
{
  int i = 0, dx;
  unsigned short width;
  void*p;

  while ( Msg[ i ] )
  {
    p = GetMiniGlyphPtr( Msg[ i ], &width );
    dx = ( 12 - width ) / 2;
    if ( dx > 0 )
    {
      PrintMiniGlyph( x, y, (void*)empty, flags, dx, 0, 0, 0, 0, color, bcolor, 0 );
    }
   
    else dx = 0;
    PrintMiniGlyph( x + dx, y, p, flags, width, 0, 0, 0, 0, color, bcolor, 0 );
    if ( width + dx < 12 )
    {
      PrintMiniGlyph( x + width + dx, y, (void*)empty, flags, 12 - width - dx, 0, 0, 0, 0, color, bcolor, 0 );
    }
    x += 12;
    i++;
  }
  return x;
}
//draws a point of color color at (x0, y0) 
void plot(int x0, int y0, unsigned short color) {
  unsigned short* VRAM = (unsigned short*)0xA8000000; 
  VRAM += (y0*LCD_WIDTH_PX + x0); 
  *VRAM = color;
  return; 
}

inline static void iplot(int x0, int y0, unsigned short color) {
  unsigned short* VRAM = (unsigned short*)0xA8000000; 
  VRAM += (y0*LCD_WIDTH_PX + x0); 
  *VRAM = color;
  return; 
}

void drawRectangle(int x, int y, int width, int height, unsigned short color) {
  unsigned short*VRAM = (unsigned short*)0xA8000000;
  for(int j = y; j < y+height; j++) {
    for(int i = x; i < x+width; i++) {
      *(j*LCD_WIDTH_PX+i+VRAM) = color;      
    }
  }
}
//Uses the Bresenham line algorithm 
void drawLine(int x1, int y1, int x2, int y2, int color) { 
    signed char ix; 
    signed char iy; 
  
    // if x1 == x2 or y1 == y2, then it does not matter what we set here 
    int delta_x = (x2 > x1?(ix = 1, x2 - x1):(ix = -1, x1 - x2)) << 1; 
    int delta_y = (y2 > y1?(iy = 1, y2 - y1):(iy = -1, y1 - y2)) << 1; 
  
    iplot(x1, y1, color);  
    if (delta_x >= delta_y) { 
        int error = delta_y - (delta_x >> 1);        // error may go below zero 
        while (x1 != x2) { 
            if (error >= 0) { 
                if (error || (ix > 0)) { 
                    y1 += iy; 
                    error -= delta_x; 
                }                           // else do nothing 
         }                              // else do nothing 
            x1 += ix; 
            error += delta_y; 
            iplot(x1, y1, color); 
        } 
    } else { 
        int error = delta_x - (delta_y >> 1);      // error may go below zero 
        while (y1 != y2) { 
            if (error >= 0) { 
                if (error || (iy > 0)) { 
                    x1 += ix; 
                    error -= delta_y; 
                }                           // else do nothing 
            }                              // else do nothing 
            y1 += iy; 
            error += delta_x;  
            iplot(x1, y1, color); 
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
  
  while(x < y) 
    { 
      // ddF_x == 2 * x + 1; 
      // ddF_y == -2 * y; 
      // f == x*x + y*y - radius*radius + 2*x - y + 1; 
      if(f >= 0)  
   { 
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

//ReplaceColor By Kerm:
void VRAMReplaceColorInRect(int x, int y, int width, int height, color_t color_old, color_t color_new) { 
   //color_t* VRAM = GetVRAMAddress();
   color_t* VRAM = (color_t*)0xA8000000; 
   VRAM += (y*LCD_WIDTH_PX)+x; 
   for(int j=0; j<height; VRAM += (LCD_WIDTH_PX-width), j++) { 
      for(int i=0; i<width; VRAM++, i++) { 
         if (*VRAM == color_old) *VRAM = color_new; 
      } 
   } 
}

void VRAMInvertArea(short x, short y, short width, short height) {
   color_t* VRAM = (color_t*)0xA8000000; 
   VRAM += (y*LCD_WIDTH_PX)+x; 
   for(int j=0; j<height; VRAM += (LCD_WIDTH_PX-width), j++) { 
      for(int i=0; i<width; VRAM++, i++) { 
         *VRAM ^= 0xFFFF;
      } 
   } 
}

void darkenStatusbar() {
  if(GetSetting(SETTING_DISPLAY_STATUSBAR)) {
    VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_BLACK, COLOR_CYAN);
    VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_WHITE, COLOR_BLACK);
    VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_CYAN, COLOR_GRAY);
    VRAMReplaceColorInRect(0, 0, LCD_WIDTH_PX, 24, COLOR_BLUE, COLOR_ORANGE);
  }
}
void darkenFkeys(int numkeys) {
  VRAMReplaceColorInRect(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_BLACK, COLOR_CYAN);
  VRAMReplaceColorInRect(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_WHITE, COLOR_BLACK);
  VRAMReplaceColorInRect(0, LCD_HEIGHT_PX-24, LCD_WIDTH_PX-64*(6-numkeys), 24, COLOR_CYAN, COLOR_GRAY);  
}
void drawArrowDown(int bottomX, int bottomY, int color) {
  drawLine(bottomX-7,bottomY-7,bottomX,bottomY,color);
  drawLine(bottomX-8,bottomY-7,bottomX,bottomY+1,color); //double thickness of line
  drawLine(bottomX+1,bottomY,bottomX+8,bottomY-7,color);
  drawLine(bottomX+1,bottomY+1,bottomX+9,bottomY-7,color); //double thickness of line
}
void drawFkeyPopup(int Fkey, int darktheme, int showclosemessage) {
//draws a big popup pointing to a Fkey (index zero based, F1 = 0, F6 = 5) with the selected color scheme.
//showclosemessage - select to show a minimini message on the right left saying "...or press: [EXIT]"
// PrintXY text inside the popup starts at X=2 and Y=2
  int fgcolor = COLOR_BLACK;
  int bgcolor = COLOR_WHITE;
  if (darktheme) {
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


  if (showclosemessage) {
    int textX = 0;
    int textY = FKEY_C3Y-14-20;
    PrintMiniMini( &textX, &textY, (unsigned char*)"...or press: [EXIT]", (darktheme == 1 ? 4 : 0), TEXT_COLOR_BLACK, 1 ); //fake draw
    textX = FKEY_C3X-textX-4;
    PrintMiniMini( &textX, &textY, (unsigned char*)"...or press: [EXIT]", (darktheme == 1 ? 4 : 0), TEXT_COLOR_BLACK, 0 ); //draw
  }
}
void CopySprite(const void* datar, int x, int y, int width, int height) { 
   color_t*data = (color_t*) datar; 
   color_t* VRAM = (color_t*)0xA8000000; 
   VRAM += LCD_WIDTH_PX*y + x; 
   for(int j=y; j<y+height; j++) { 
      for(int i=x; i<x+width; i++) { 
         *(VRAM++) = *(data++); 
     } 
     VRAM += LCD_WIDTH_PX-width; 
   } 
} 
void CopySpriteMasked(const unsigned char* data, int x, int y, int width, int height, int maskcolor) { 
   char* VRAM = (char*)0xA8000000; 
   VRAM += 2*(LCD_WIDTH_PX*y + x); 
   for(int j=y; j<y+height; j++) { 
      for(int i=x; i<x+width;  i++) { 
         if ((((((int)(*data))&0x000000FF)<<8) | ((((int)(*(data+1))))&0x000000FF)) != maskcolor) { 
            *(VRAM++) = *(data++); 
            *(VRAM++) = *(data++); 
         } else { VRAM += 2; data += 2; } 
      } 
      VRAM += 2*(LCD_WIDTH_PX-width); 
   } 
}
void CopySpriteNbit(const unsigned char* data, int x, int y, int width, int height, const color_t* palette, unsigned int bitwidth) {
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
         *VRAM = palette[(color_t)thisthis];
         VRAM++;
         buf<<=bitwidth;
         availbits-=bitwidth;
      }
      VRAM += (LCD_WIDTH_PX-width);
   }
} 
int drawRGB24toRGB565(int r, int g, int b)  
{  
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);  
}
//the following does not update the screen automatically; it will draw the tny.im logo starting at screen coordinates x,y
//the tny.im logo is great enough not to require any sprites! yay!
//w:138
//h:42
int alphaBlend(int newcc, int oldcc, float alpha) {
  return alpha*newcc+(1-alpha)*oldcc;
}
/* commented because isn't needed
void alphaRGB(int r1, int g1, int b1, int r2, int g2, int b2, int* nr, int* ng, int* nb, float alpha) {
  *nr = alphaBlend(r1, r2, alpha);
  *ng = alphaBlend(g1, g2, alpha);
  *nb = alphaBlend(b1, b2, alpha);
}*/
void drawtnyimLogo( int x, int y, float alpha) {
  //draw t
  int black = drawRGB24toRGB565(alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha), alphaBlend(255, 0, alpha));
  drawRectangle(x, y+6, 6, 24, black);
  drawRectangle(x+6, y+12, 6, 6, black);
  drawRectangle(x+6, y+30, 6, 6, black);
  //draw n
  drawRectangle(x+18, y+12, 6, 24, black);
  drawRectangle(x+24, y+12, 12, 6, black);
  drawRectangle(x+36, y+18, 6, 18, black);
  //draw y
  drawRectangle(x+48, y+12, 6, 18, black);
  drawRectangle(x+60, y+12, 6, 18, black);
  drawRectangle(x+54, y+30, 6, 6, black);
  drawRectangle(x+48, y+36, 6, 6, black);
  //draw dot
  drawRectangle(x+72, y+30, 6, 6, black);
  int orange = drawRGB24toRGB565(alphaBlend(255, 210, alpha), alphaBlend(255, 68, alpha), alphaBlend(255, 19, alpha));
  //draw i (orange)
  drawRectangle(x+84, y, 6, 6, orange);
  drawRectangle(x+84, y+12, 6, 24, orange);
  //draw m (orange)
  drawRectangle(x+96, y+12, 6, 24, orange);
  drawRectangle(x+102, y+12, 12, 6, orange);
  drawRectangle(x+114, y+18, 6, 18, orange);
  drawRectangle(x+120, y+12, 12, 6, orange);
  drawRectangle(x+132, y+18, 6, 18, orange);
}

int textColorToFullColor(int textcolor) {
  switch(textcolor) {
    case TEXT_COLOR_BLACK: return COLOR_BLACK;
    case TEXT_COLOR_BLUE: return COLOR_BLUE;
    case TEXT_COLOR_GREEN: return COLOR_GREEN;
    case TEXT_COLOR_CYAN: return COLOR_CYAN;
    case TEXT_COLOR_RED: return COLOR_RED;
    case TEXT_COLOR_PURPLE: return COLOR_MAGENTA;
    case TEXT_COLOR_YELLOW: return COLOR_YELLOW;
    case TEXT_COLOR_WHITE: return COLOR_LIGHTGRAY;
    case 8: return 0x08; // used in menu separators in week view.
    default: return COLOR_BLACK;
  }
}

void progressMessage(char* message, int cur, int total) {
  // when you're done with this, close with closeProgressMessage!
  // otherwise, resources won't be freed.
  // always call with cur==0 and total > 0 first, so that the progressbar is initialized!
  // if message is empty, the default OS "please wait" message will be used.
  if(strlen(message) == 0) {
    ProgressBar( cur, total );
  } else {
    ProgressBar2( (unsigned char*)message, cur, total );
  }
}
void closeProgressMessage() {
  MsgBoxPop(); //closes progressbar
}

void printCentered(unsigned char* text, int y, int FGC, int BGC) {
  int len = strlen((char*)text);
  int pixlen = len*18;
  int x = LCD_WIDTH_PX/2-pixlen/2;
  int cur = 0;
  while(cur<len) {
    PrintCXY(x, y, &text[cur], 0x40, -1, FGC, BGC, 1, 0 );
    x=x+18;
    cur++;
  }
}

void clearLine(int x, int y, color_t color) {
  // clear text line. x and y are text cursor coordinates
  // this is meant to achieve the same effect as using PrintXY with a line full of spaces (except it doesn't waste strings).
  int width=LCD_WIDTH_PX;
  if(x>1) width = 24*(21-x);
  drawRectangle((x-1)*18, y*24, width, 24, color);
}

void mPrintXY(int x, int y, char*msg, int mode, int color) {
  char nmsg[50] = "";
  nmsg[0] = 0x20;
  nmsg[1] = 0x20;
  strncat(nmsg, msg, 48);
  PrintXY(x, y, nmsg, mode, color );
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
int getNumberOfMsgBoxPushed() {
  return numberOfMsgBoxPushed;
}