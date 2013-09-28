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

#include "hardwareProvider.hpp"

// START OF POWER MANAGEMENT CODE
#define LCDC *(unsigned int*)(0xB4000000)
int GetBacklightSubLevel_RAW()
{
  Bdisp_DDRegisterSelect(0x5a1);
  return (LCDC & 0xFF) - 6;
}
void SetBacklightSubLevel_RAW(int level)
{
  Bdisp_DDRegisterSelect(0x5a1);
  LCDC = (level & 0xFF) + 6;
}
// END OF POWER MANAGEMENT CODE 

// Get calculator's unique ID (as told by Simon Lothar): 8 bytes starting at 0x8001FFD0

void getHardwareID(char* buffer) {
  // NOTE buffer must be at least 8 bytes long!
  memcpy(buffer, (void*)0x8001FFD0, 8);
}

// Get calculator model. There is a syscall for this, but with this we know exactly what we are checking.
// not to be confused with the "PCB model", which on the Prizm series appears to be always Ly755 or Ly755D or just 755D
// returns 0 on unknown/inconclusive model, 1 on fx-CG 10 and 2 on fx-CG 20
int getHardwareModel() {
  char* byte1 = (char*)0x80000303;
  char* byte2 = (char*)0x80000305;
  if((*byte1 == '\x41') && (*byte2 == '\x5A')) return 1;
  if((*byte1 == '\x44') && (*byte2 == '\xAA')) return 2;
  return 0;
}

// returns 1 if the calculator is emulated
// uses the power source information to detect the emulator
// obviously, if an emulator comes that emulates this info better, this function will be inaccurate
int getIsEmulated() {
  unsigned char hb[15]; int k;
  k = *(unsigned char*)P11DR;
  WordToHex( k & 0xFFFF, hb );
  if (k==0x0000) return 1;
  else return 0;
}

// CPU CLOCKING CODE:

void CPU_change_freq(int mult) { 
   __asm__( 
      "mov r4, r0\n\t" 
      "and #0x3F, r0\n\t"  
      "shll16 r0\n\t"  
      "shll8 r0\n\t"  
      "mov.l frqcr, r1\n\t"   
      "mov.l pll_mask, r3\n\t"   
      "mov.l @r1, r2\n\t"   
      "and r3, r2\n\t"   
      "or r0, r2\n\t"  
      "mov.l r2, @r1\n\t"  
      "mov.l frqcr_kick_bit, r0\n\t"  
      "mov.l @r1, r2\n\t" 
      "or r0, r2\n\t" 
      "rts\n\t" 
      "mov.l r2, @r1\n\t" 
      ".align 4\n\t" 
      "frqcr_kick_bit: .long 0x80000000\n\t" 
      "pll_mask: .long 0xC0FFFFFF\n\t"  
      "frqcr: .long 0xA4150000\n\t" 
   ); 
}