#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "versionProvider.hpp"
#include "constantsProvider.hpp"
#include "timeProvider.hpp"

void getVersion(char* buffer) {
  strcpy(buffer, __GIT_VERSION);
}
void getTimestamp(char* buffer) {
  strcpy(buffer, __GIT_TIMESTAMP);
}

int getBuildIsExpired() {
  // returns true if build is expired
  return ((long long int)BUILD_EXPIRE_TIMESTAMP < currentUnixTime());
}