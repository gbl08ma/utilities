#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "versionProvider.hpp"
#include "constantsProvider.hpp"
#include "timeProvider.hpp"

const char* getVersion() {
  #ifdef ENABLE_PICOC_SUPPORT
    return __GIT_VERSION "\xE7\x63";
  #else
    return __GIT_VERSION;
  #endif
}
const char* getTimestamp() {
  return __GIT_TIMESTAMP;
}

int isBuildExpired() {
  // returns true if build is expired
  return ((long long int)BUILD_EXPIRE_TIMESTAMP < currentUEBT());
}