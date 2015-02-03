#include "math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fxcg/misc.h>
#include "tbcdProvider.hpp"
float math_floor(float x) {
  return (((x)<0 && (x)!=(float)(int)(x))?(((float)(int)(x))-1):((float)(int)(x)));
}

double math_modf(double x, double *ipart) {
  long int i=0;
  if(x >= 10000) {
    if(x>=50000) i=50000;
    else i=10000;
  }
  while(x >= i) i++;
  *ipart = (i-1);
  return (x-(*ipart));
}


////////// TBCD //////////

TBCDvalue*TBCD::PValue(){
  return FValue;
}

int TBCD::Get( TBCDvalue&value ) {
int result = 0;
  memcpy( &value, &FValue, sizeof( TBCDvalue ) );
  return result;
}

int TBCD::Set( TBCDvalue&value ) {
int result = 0;
  memcpy( &FValue, &value, sizeof( TBCDvalue ) );
  return result;
}

int TBCD::Set( double&value ) {
  int result = 0;
  double dwork;
  double dwork2;
  int iexponent;
  int i;
  memset(FValue, 0, sizeof(TBCDvalue));
  iexponent = 100;
  dwork = value;
  if (dwork < 0) {
    iexponent += 500;
    dwork = -dwork;
  }
  while (dwork >= 10) {
    iexponent += 1;
    dwork = dwork / 10;
  }

  while (dwork < 1){
    iexponent -= 1;
    dwork = dwork * 10;
  }

  FValue[0].exponent = (iexponent / 100);
  iexponent -= (iexponent / 100) * 100;
  FValue[0].exponent <<= 4;
  FValue[0].exponent += (iexponent / 10);
  FValue[0].exponent <<= 4;
  iexponent -= (iexponent / 10) * 10;
  FValue[0].exponent += iexponent;
 
  dwork = math_modf(dwork, &dwork2) * 10;
  FValue[0].mantissa0 = math_floor( dwork2 );
  for ( i = 0; i < 7;i++ ) {
    dwork = math_modf(dwork, &dwork2) * 10;
    FValue[0].mantissa[i].hnibble = dwork2;
    dwork = math_modf(dwork, &dwork2) * 10;
    FValue[0].mantissa[i].lnibble = dwork2;
  }

  return result;
}

int TBCD::Get(double&value){
  int result = 0;
  int i;
  double factor = 1;
  TBCDInternal work;

  BCDtoInternal(&work, FValue);       // syscall 0x0160
  value = work.mantissa[0];
  for (i = 1; i < 15; i++) {
    factor /= 10;
    if(work.mantissa[i]) value += factor * work.mantissa[i];
  }
  while (work.exponent++ < 0) value /= 10;
  while (--work.exponent > 0) value *= 10;
  value *= work.sign;
 
  return result;
}

int TBCD::SetError(int error) {
  int result = (FValue[0].exponent & 0xF00) >> 4;
  FValue[0].exponent = (FValue[0].exponent & 0xFF) | (error << 4);
  return result;
}

int TBCD::GetError() {
  int result = (FValue[0].exponent & 0xF00) >> 4;
  return result;
};

void TBCD::Swap() {
  TBCDvalue work;
  FValue[0] = FValue[1];
  FValue[1] = work;
}; 
