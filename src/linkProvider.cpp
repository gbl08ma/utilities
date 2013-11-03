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

#include "linkProvider.hpp"

// code based on example by Simon Lothar
// send a file through the 3-pin serial. Compatible with Casio's functions.
// remote calculator should be in receive mode with cable type set to 3-pin
// pay attention to CPU clock speeds as they affect the baud rate, causing errors
#ifdef UNUSEDCODE_1234
int SerialFileTransfer( unsigned char*_filename ) {
  TTransmitBuffer sftb;
  int l = strlen( (char*)_filename );
  unsigned int iii, jjj, phase;
  //FILE_INFO fileinfo;
  unsigned int calcType;
  unsigned short osVer;

  if ( l <= 7 ) return -1;
  if ( _filename[0] != '\\' ) return -2;
  if ( _filename[1] != '\\' ) return -3;
  if ( _filename[6] != '\\' ) return -4;

  memset( &sftb, 0, sizeof( sftb ) );

  strncpy( sftb.device, (char*)_filename+2, 4 );
  if ( strcmp( sftb.device, "fls0" ) ) return -5;

  strcpy( sftb.fname1, (char*)_filename+7 );
  // strcpy( sftb.fname2, (char*)_filename+7 );

  Bfile_StrToName_ncpy( sftb.filename, _filename, 0x10A );
  // get filesize:
  int handle = Bfile_OpenFile_OS(sftb.filename, READWRITE, 0);
  sftb.filesize = Bfile_GetFileSize_OS(handle);
  Bfile_CloseFile_OS(handle);
  /*iii = Bfile_GetFileInfo( sftb.filename, &fileinfo );
  sftb.filesize = fileinfo.fsize;*/

  sftb.command = 0x45;
  sftb.datatype = 0x80;
  sftb.handle = -1;
  sftb.source = 1; // SMEM:1
  sftb.zero = 0;

  phase = 0;

  while( 1 ){
    phase = 1;
    //App_LINK_SetReceiveTimeout_ms( 360000 );
    App_LINK_SetReceiveTimeout_ms( 6000 );
    iii = Comm_Open( 0x1000 );
    if ( iii ) break;

    phase = 2;
    // Serial_Send_0Ax10_Sequence();
    //SetGetKeyPhaseFlag(); // 02CA
    iii = Comm_TryCheckPacket( 0 ); // 1396
    if ( iii ) break;

    phase = 3;
    iii = App_LINK_SetRemoteBaud(); // 1397
    if ( iii ) break;

    OS_InnerWait_ms( 20 );

    phase = 4;
    iii = Comm_TryCheckPacket( 1 ); // 1396
    if ( iii ) break;

    phase = 5;
    // iii = App_LINK_Send_ST9_Packet();
    if ( iii && ( iii != 0x14 ) ) break;

    phase = 6;
    // iii = App_LINK_GetDeviceInfo( &calcType, &osVer );
    if ( iii ) break;

    phase = 7;
    iii = App_LINK_TransmitInit( &sftb );
    //iii = App_LINK_TransmitInit2( &sftb, OVERWRITE );
    if ( iii ) break;

    phase = 8;
    iii = App_LINK_Transmit( &sftb );
    if ( iii ) break;

    phase = 0xFF;
    break;
  };
  Comm_Terminate( 0 );
  jjj = 1;
  while ( Comm_Close( 0 ) == 5 ) jjj++;

  phase<<=24;
  jjj<<=16;
  return phase + jjj + iii;
}
#endif