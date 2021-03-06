#ifndef ATOI_REPLACEMENT
#undef atoi
#define atoi sys_atoi // the strtol-provided atoi defined in libfxcg appears to be broken
#define ATOI_REPLACEMENT
#endif

#ifndef ADDIN_FRIENDLYNAME
#define ADDIN_FRIENDLYNAME "Utilities"
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

#ifndef SMEM_DEVICE
#define SMEM_DEVICE "fls0"
#endif

#ifndef SMEM_PREFIX
#define SMEM_PREFIX "\\\\" SMEM_DEVICE "\\"
#endif

#ifndef CALENDARFOLDER_NAME
#define CALENDARFOLDER_NAME "@UTILS"
#endif

#ifndef CALENDARFOLDER
#define CALENDARFOLDER (char*)SMEM_PREFIX CALENDARFOLDER_NAME
#endif

#ifndef BALANCEFOLDER
#define BALANCEFOLDER (char*)SMEM_PREFIX CALENDARFOLDER_NAME "\\Balance"
#endif

#ifndef TOTPFOLDER
#define TOTPFOLDER (char*)SMEM_PREFIX CALENDARFOLDER_NAME "\\TOTP"
#endif

#ifndef SMEMHASHFILE
#define SMEMHASHFILE (char*)SMEM_PREFIX CALENDARFOLDER_NAME "\\Hash.plp"
#endif

#ifndef TEMPFILE
#define TEMPFILE (char*)SMEM_PREFIX "UTILSTMP.PCT"
#endif

#ifndef TEMPFILE2
#define TEMPFILE2 (char*)SMEM_PREFIX "UTILSTM2.PCT"
#endif

#ifndef SELFFILE
#define SELFFILE "utilities.g3a"
#endif

#ifndef KNOWN_PAST_TIMESTAMP
// a timestamp that's known to be in the past for all timezones, so we can detect a unadjusted RTC
#define KNOWN_PAST_TIMESTAMP 1466380800*1000
#endif

#ifndef BUILD_EXPIRE_TIMESTAMP
// after the calc RTC is in a timestamp past this, users will be prompted to check for updates
#define BUILD_EXPIRE_TIMESTAMP 1479600000*1000
#endif

#ifndef HIGHEST_SUPPORTED_YEAR
// can't be above 9999 because calendar database format and code expects year to have at most four digits
#define HIGHEST_SUPPORTED_YEAR 9999
#endif

#ifndef LOWEST_SUPPORTED_YEAR
#define LOWEST_SUPPORTED_YEAR 0
#endif

/*
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 1
#endif
*/

#ifndef ENABLE_PICOC_SUPPORT
#ifdef __ENABLE_PICOC_SUPPORT
#define ENABLE_PICOC_SUPPORT
#endif
#endif