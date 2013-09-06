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
#define CALENDARFOLDER (char*)"@UTILS"
#endif

#ifndef HASHFILE
#define HASHFILE (unsigned char*)"Hash"
#endif

#ifndef SELFFILE
#define SELFFILE (char*)"utilities.g3a"
#endif

#ifndef KNOWN_PAST_TIMESTAMP
#define KNOWN_PAST_TIMESTAMP 1378249200*1000 // a timestamp that's known to be in the past for all timezones, so we can detect a unadjusted RTC
#endif