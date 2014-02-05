#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "versionProvider.hpp"
#include "constantsProvider.hpp"
#include "timeProvider.hpp"

const char* getVersion() {
  return __GIT_VERSION;
}
const char* getTimestamp() {
  return __GIT_TIMESTAMP;
}

int getBuildIsExpired() {
  // returns true if build is expired
  return ((long long int)BUILD_EXPIRE_TIMESTAMP < currentUnixTime());
}