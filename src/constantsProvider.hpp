#ifndef ATOI_REPLACEMENT
#undef atoi
#define atoi sys_atoi // the strtol-provided atoi defined in libfxcg appears to be broken
#define ATOI_REPLACEMENT
#endif

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

#ifndef CALENDARFOLDER_NAME
#define CALENDARFOLDER_NAME (char*)"@UTILS"
#endif

#ifndef HASHFILE
#define HASHFILE (unsigned char*)"Hash"
#endif

#ifndef SMEMHASHFILE
#define SMEMHASHFILE (char*)"\\\\fls0\\@UTILS\\Hash.plp"
#endif

#ifndef TEMPFILE
#define TEMPFILE (char*)"\\\\fls0\\UTILSTMP.PCT"
#endif

#ifndef TEMPFILE2
#define TEMPFILE2 (char*)"\\\\fls0\\UTILSTM2.PCT"
#endif

#ifndef SELFFILE
#define SELFFILE (char*)"utilities.g3a"
#endif

#ifndef KNOWN_PAST_TIMESTAMP
#define KNOWN_PAST_TIMESTAMP 1402704000*1000 // a timestamp that's known to be in the past for all timezones, so we can detect a unadjusted RTC
#endif

#ifndef BUILD_EXPIRE_TIMESTAMP
#define BUILD_EXPIRE_TIMESTAMP 1412035200*1000 // after the calc RTC is in a timestamp past this, users will be prompted to check for updates
#endif

#ifndef HIGHEST_SUPPORTED_YEAR
#define HIGHEST_SUPPORTED_YEAR 9999 // can't be above 9999 because calendar database format and code expects year to have at most four digits
#endif

#ifndef LOWEST_SUPPORTED_YEAR
#define LOWEST_SUPPORTED_YEAR 0
#endif


#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 1
#endif