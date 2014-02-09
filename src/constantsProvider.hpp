#ifndef DIRNAME
#define DIRNAME (unsigned char*)"@UTILS"
#endif

#ifndef SETTINGSFILE
#define SETTINGSFILE (unsigned char*)"Set"
#endif

#ifndef CHRONOFILE
#define CHRONOFILE (unsigned char*)"Chrono"
#endif

#ifndef CALENDARFOLDER
#define CALENDARFOLDER (char*)"\\\\fls0\\@UTILS"
#endif

#ifndef HASHFILE
#define HASHFILE (unsigned char*)"Hash"
#endif

#ifndef SMEMHASHFILE
#define SMEMHASHFILE (unsigned char*)"\\\\fls0\\@UTILS\\Hash.plp"
#endif

#ifndef SELFFILE
#define SELFFILE (char*)"utilities.g3a"
#endif

#ifndef KNOWN_PAST_TIMESTAMP
#define KNOWN_PAST_TIMESTAMP 1388966400*1000 // a timestamp that's known to be in the past for all timezones, so we can detect a unadjusted RTC
#endif

#ifndef BUILD_EXPIRE_TIMESTAMP
#define BUILD_EXPIRE_TIMESTAMP 1397516400*1000 // after the calc RTC is in a timestamp past this, users will be prompted to check for updates
#endif