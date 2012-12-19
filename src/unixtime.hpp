/*unsigned int DAYS_PER_YEAR;           // Non leap year
unsigned int SECONDS_PER_YEAR;      // Non leap year
unsigned int SECONDS_PER_DAY;
unsigned int SECONDS_PER_HOUR;
unsigned int SECONDS_PER_MINUTE;*/
long long int LeapYear(long long int year);
long long int DaysPerMonth(long long int year,long long int month);
long long int DaysPerYear(long long int year);
 typedef struct {
   long long int year;
   long long int month;
   long long int day;
   long long int hour;
   long long int minute;
   long long int second;
   
   } datetime;
datetime Unix2DateTime(long long int unixtime);
char* MonthName(long long int month);
long long int DateTime2Unix(long long int year, long long int month, long long int day, long long int hour, long long int minute, long long int second, long long int millisecond);
