#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "version.h"
void getVersion(char* buffer) {
  strcpy(buffer, __GIT_VERSION);
}
void getTimestamp(char* buffer) {
  strcpy(buffer, __GIT_TIMESTAMP);
}