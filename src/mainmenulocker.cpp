#include "mainmenulocker.hpp"
/* CODE BY SIMON LOTHAR, AVAILABLE ON "fx_calculators_SuperH_based.chm" version 16 */

// the function assumes, that the RAM-pointer to GetkeyToMainFunctionReturnFlag is loaded 
// immediately by a "Move Immediate Data"-instruction
unsigned int SetGetkeyToMainFunctionReturnFlag( unsigned int enabled ){
int addr, addr2;
// get the pointer to the syscall table
	addr = *(unsigned char*)0x80020071;	// get displacement
	addr++;
	addr *= 4;
	addr += 0x80020070;
	addr = *(unsigned int*)addr;
	if ( addr < (int)0x80020070 ) return 0x101;
	if ( addr >= (int)0x81000000 ) return 0x102;
// get the pointer to syscall 1E99
	addr += 0x1E99*4;
	if ( addr < (int)0x80020070 ) return 0x103;
	if ( addr >= (int)0x81000000 ) return 0x104;

	addr = *(unsigned int*)addr;
	if ( addr < (int)0x80020070 ) return 0x105;
	if ( addr >= (int)0x81000000 ) return 0x106;

	switch ( *(unsigned char*)addr ){
		case 0xD0 : // MOV.L @( disp, PC), Rn (REJ09B0317-0400 Rev. 4.00 May 15, 2006 page 216)
		case 0xD1 :
		case 0xD2 :
		case 0xD3 :
		case 0xD4 :
		case 0xD5 :
		case 0xD6 :
		case 0xD7 :
		case 0xD8 :
			addr2 = *(unsigned char*)( addr + 1 );	// get displacement
			addr2++;
			addr2 *= 4;
			addr2 += addr;
			addr2 &= ~3;

			if ( addr2 < (int)0x80020070 ) return 0x107;
			if ( addr2 >= (int)0x81000000 ) return 0x108;

			addr = *(unsigned int*)addr2;
			if ( ( addr & 0xFF000000 ) != 0x88000000 ) return 0x109;

// finally perform the desired operation and set the flag:
			if ( enabled ) *(unsigned char*)addr = 0;
			else *(unsigned char*)addr = 3;

			break;

		default : addr = 0x100;
}
return addr;
}