#ifndef __LOCKPROVIDER_H
#define __LOCKPROVIDER_H
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

#define RETURN_PASSWORD_MATCH 0
#define RETURN_PASSWORD_NOMCS 1
#define RETURN_PASSWORD_NOSMEM 2
#define RETURN_PASSWORD_STORAGE_MISMATCH 3
#define RETURN_PASSWORD_MISMATCH 4


void hashPassword(unsigned char* password, unsigned char* hash);
int comparePasswordHash(unsigned char* inputPassword);
int savePassword(unsigned char* password);
int isPasswordSet(void);

#endif 
