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
#include <alloca.h>

#include "constantsProvider.hpp"
#include "timeProvider.hpp"
#include "settingsProvider.hpp"
#include "selectorGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"

void saveChronoArray(chronometer* chronoarray, int count) { // count is the amount of chrono in array
  int allClear = 1;
  for(int cur = 0; cur < count; cur++) {
    if(chronoarray[cur].state != CHRONO_STATE_CLEARED) allClear=0;
  }
  if(allClear) {
    // all chronos are clear
    // this means that saving a state file to MCS is wasting space there
    // just delete any existing file and return.
    MCSDelVar2(DIRNAME, CHRONOFILE);
    return;
  }
  // Check if directory exists:
  if(MCS_CreateDirectory(DIRNAME)) // directory already exists, so delete the exiting file that may be there
    MCSDelVar2(DIRNAME, CHRONOFILE);
  MCSPutVar2(DIRNAME, CHRONOFILE, count*sizeof(chronometer), chronoarray);
}

void loadChronoArray(chronometer* chronoarray, int count) { // count is the amount of chrono to load to array
  int size;
  MCSGetDlen2(DIRNAME, CHRONOFILE, &size);
  // check if file exists, and compare read file size to expected file size to detect incompatibility.
  // if there is, delete old file and return
  if (size != count*(int)sizeof(chronometer)) {
    // doesn't exist or is incompatible. We could return right now, but other code may be expecting a "clean" chronoarray,
    // so we must clear each chrono manually
    for(int cur=0; cur < count; cur++) clearChrono(&chronoarray[cur]);
    return;
  }
  MCSGetData1(0, count*sizeof(chronometer), chronoarray);
}

void setChrono(chronometer* tchrono, long long int duration, long long int type) {
  //timer starts when it is set
  if(tchrono->state == CHRONO_STATE_CLEARED) {
    tchrono->starttime = currentUnixTime();
    tchrono->laststop = 0;
    tchrono->duration = (type == CHRONO_TYPE_UP ? 0 : duration);
    tchrono->state = CHRONO_STATE_RUNNING;
    tchrono->type = type;
  }
}

void stopChrono(chronometer* tchrono) {
  if(tchrono->state == CHRONO_STATE_RUNNING) {
    tchrono->laststop = currentUnixTime();
    tchrono->state = CHRONO_STATE_STOPPED;
  }
}

void startChrono(chronometer* tchrono) {
  if(tchrono->state == CHRONO_STATE_STOPPED) {
    if(tchrono->type == CHRONO_TYPE_UP) tchrono->starttime = tchrono->starttime+currentUnixTime()-tchrono->laststop;
    else tchrono->duration = tchrono->duration+currentUnixTime()-tchrono->laststop;
    tchrono->state = CHRONO_STATE_RUNNING;
  }
}
void clearChrono(chronometer* tchrono) {
  tchrono->state = CHRONO_STATE_CLEARED;
}

chronometer* chrono;
chronometer* getChronoArrayPtr() {
  return chrono;
}

void setChronoArrayPtr(chronometer* achrono) {
  chrono = achrono;
}
void checkChronoComplete() {
  checkDownwardsChronoCompleteGUI(chrono, NUMBER_OF_CHRONO);
}
int setChronoExternal(int index, long long int duration, long long int type) {
  //function for setting a chrono without access to the main chrono array (as of beta 9, the only one)
  //chronometer* chrono is used (this is the main one used in the chrono screen)
  //the chrono that is set is the one specified in index (zero based).
  //returns 1 on success (timer was cleared) and 0 on error (timer is in use)
  if(chrono[index].state == CHRONO_STATE_CLEARED) {
    setChrono(&chrono[index], duration, type);
    // since this is "external", most likely the calling function doesn't have access to chrono or a way to save that array to storage
    // so lets save ourselves
    saveChronoArray(chrono, NUMBER_OF_CHRONO);
    return 1;
  } else {
    return 0;
  }
}