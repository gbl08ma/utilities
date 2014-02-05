#ifndef __VERSIONPROVIDER_H
#define __VERSIONPROVIDER_H

# ifdef __cplusplus
extern "C" {
# endif
const char* getVersion();
const char* getTimestamp();
# ifdef __cplusplus
}
# endif
int getBuildIsExpired();

#endif