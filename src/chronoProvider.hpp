#ifndef __CHRONOPROVIDER_H
#define __CHRONOPROVIDER_H

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

#define NUMBER_OF_CHRONO 20

#define CHRONO_STATE_STOPPED 0
#define CHRONO_STATE_RUNNING 1
#define CHRONO_STATE_CLEARED 2

#define CHRONO_TYPE_UP 0
#define CHRONO_TYPE_DOWN 1

typedef struct {
  long long int starttime=0; //time stamp at which chronometer was put to run
  long long int duration=0; //duration, in seconds (increases when chronometer is paused and counting down; stays constant when chronometer is paused and counting up)
  long long int laststop=0; //time stamp at which the chronometer was last paused
  long long int state=CHRONO_STATE_CLEARED; //0 if stopped, 1 if running, 2 if clear
  long long int type=CHRONO_TYPE_DOWN; // whether it's counting up or down
} chronometer;

void chronoToBuffer(chronometer* tchrono, long long int* buffer);
void bufferToChrono(long long int* buffer, chronometer* tchrono);
void saveChronoArray(chronometer* chronoarray, int count);
void loadChronoArray(chronometer* chronoarray, int count);
void setChrono(chronometer* tchrono, long long int duration, long long int type);
void stopChrono(chronometer* tchrono);
void startChrono(chronometer* tchrono);
void clearChrono(chronometer* tchrono);

void getChronoArrayPtr(chronometer* achrono);
void setChronoArrayPtr(chronometer* achrono);
void checkChronoComplete();
int setChronoExternal(int index, long long int duration, long long int type);

#endif