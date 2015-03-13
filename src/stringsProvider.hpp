#ifndef __STRINGSPROVIDER_H
#define __STRINGSPROVIDER_H

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

char *toksplit(char *src, char tokchar, char *token, int lgh);
int EndsIWith(const char *str, const char *suffix);
void* memmem(char* haystack, int hlen, char* needle, int nlen, int matchCase=1);
int strncpy_retlen(char* dest, const char* src, int n);
void stringToMini(char* dest, const char* orig);
int base32_decode(const unsigned char *encoded, unsigned char *result, int size);
int base32_validate(const char* string);
void itoa_zeropad(int n, char* buffer, int digits);

#endif