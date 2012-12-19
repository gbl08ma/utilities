/////////////////////////////////////////////////////////////////////
// Script Library Contribution by Flennan Roffo
// Logic Scripted Products & Script Services
// Peacock Park (183,226,69)
// (c) 2007 - Flennan Roffo
//
// Distributed as GPL, donated to wiki.secondlife.com on 19 sep 2007
// Modified by gbl08ma for use in the Casio Prizm Utilities add-in
//
// SCRIPT:  Unix2DateTimev1.0.lsl
//
// FUNCTION: 
// Perform conversion from return value of llGetUnixTime() to
// date and time char*s and vice versa.
/////////////////////////////////////////////////////////////////////

///////////////////////////// Unix Time conversion //////////////////

long long int DAYS_PER_YEAR        = 365;           // Non leap year
long long int SECONDS_PER_YEAR     = 31536000;      // Non leap year
long long int SECONDS_PER_DAY      = 86400;
long long int SECONDS_PER_HOUR     = 3600;
long long int SECONDS_PER_MINUTE   = 60;
 
/*list MonthNameList = [  "JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" ];*/
const char *umonthNames[] = {"January",
  "February",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
 }; 
typedef struct {
 long long int year;
 long long int month;
 long long int day;
 long long int hour;
 long long int minute;
 long long int second;
 
 } datetime;
////////////////////////////// LeapYear() /////////////////////////////
 
long long int LeapYear(long long int year)
{
    if (year % 4 == 0)
    {
        if (year % 100 == 0)
        {
            if (year % 400 == 0)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 0;
    }
}
 
////////////////////////////// DaysPerMonth() ///////////////////////////////////////
 
long long int DaysPerMonth(long long int year,long long int month)
{
    if (month < 8)
    {
        if (month % 2 == 0)
        {
            if (month == 2)
            {
                if (LeapYear(year))
                {
                    return 29;
                }
                else
                {
                    return 28;
                }
            }
            else
            {
                return 30;
            }
        }
        else
        {
            return 31;
        }
    }
    else
    {
        if (month % 2 == 0)
        {
            return 31;
        }
        else
        {
            return 30;
        }
    }
}
 
/////////////////////////// DaysPerYear() ////////////////////////////////////
 
long long int DaysPerYear(long long int year)
{
    if (LeapYear(year))
        return DAYS_PER_YEAR + 1;
    else
        return DAYS_PER_YEAR;
}
 
///////////////////////////////////////////////////////////////////////////////////////
// Convert Unix time (unsigned int) to a Date and Time char*
///////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////// Unix2DataTime() ///////////////////////////////////////
 
//list Unix2DateTime(unsigned int unixtime)
datetime Unix2DateTime(long long int unixtime)
{
    datetime dt;
    
    long long int days_since_1_1_1970     = unixtime / SECONDS_PER_DAY;
    long long int day = days_since_1_1_1970 + 1;
    long long int year  = 1970;
    long long int days_per_year = DaysPerYear(year);
 
    while (day > days_per_year)
    {
        day -= days_per_year;
        ++year;
        days_per_year = DaysPerYear(year);
    }
 
    long long int month = 1;
    long long int days_per_month = DaysPerMonth(year,month);
 
    while (day > days_per_month)
    {
        day -= days_per_month;
 
        if (++month > 12)
        {    
            ++year;
            month = 1;
        }
 
        days_per_month = DaysPerMonth(year,month);
    }
 
    long long int seconds_since_midnight  = unixtime % SECONDS_PER_DAY;
    long long int hour        = seconds_since_midnight / SECONDS_PER_HOUR;
    long long int second      = seconds_since_midnight % SECONDS_PER_HOUR;
    long long int minute      = second / SECONDS_PER_MINUTE;
    second              = second % SECONDS_PER_MINUTE;
 
    //return [ year, month, day, hour, minute, second ];
    dt.year = year;
    dt.month = month;
    dt.day = day;
    dt.hour = hour;
    dt.minute = minute;
    dt.second = second;
    return dt;
}
 
///////////////////////////////// MonthName() ////////////////////////////

char* MonthName(long long int month)
{
    /*if (month >= 0 && month < 12)
        return llList2char*(MonthNameList, month);
    else
        return "";*/
    return (char*)umonthNames[month];
}
 
///////////////////////////////// Datechar*() ///////////////////////////
 // declared not useful by gbl08ma
/*char* Datechar(datetime dt)
{
    unsigned int year       = dt.year;
    unsigned int month      = dt.month;
    unsigned int day        = dt.day;
 
    return (char*)day + "-" + MonthName(month - 1) + "-" + (char*)year;
}*/
 
///////////////////////////////// Timechar*() ////////////////////////////
 // declared not useful by gbl08ma
/*char* Timechar(datetime dt)
{
    unsigned int hour        = llList2unsigned int(timelist,3);
    unsigned int minute      = llList2unsigned int(timelist,4);
    unsigned int second      = llList2unsigned int(timelist,5);
    char*  hourstr     = (char*)hour;
    char*  minutestr   = (char*)minute;
    char*  secondstr   = (char*)second;
 
    if (hour < 10)      hourstr     = "0" + hourstr;
    if (minute < 10)    minutestr     = "0" + minutestr;
    if (second < 10)    secondstr    = "0" + secondstr;
    return hourstr + ":" + minutestr + ":" + secondstr;
}*/
 
///////////////////////////////////////////////////////////////////////////////
// Convert a date and time to a Unix time unsigned int
///////////////////////////////////////////////////////////////////////////////
 
////////////////////////// DateTime2Unix() ////////////////////////////////////
 //doesn't return seconds, but milliseconds since epoch.
long long int DateTime2Unix(long long int year, long long int month, long long int day, long long int hour, long long int minute, long long int second, long long int millisecond)
{
	long long int time = 0;
	long long int yr = 1970;
	long long int mt = 1;
	long long int days;
 
	while(yr < year)
	{
		days = DaysPerYear(yr++);
		time += days * SECONDS_PER_DAY*1000;
	}
 
	while (mt < month)
	{
		days = DaysPerMonth(year,mt++);
		time += days * SECONDS_PER_DAY*1000;
	}
 
	days = day - 1;
	time += days * SECONDS_PER_DAY*1000;
	time += hour * SECONDS_PER_HOUR*1000;
	time += minute * SECONDS_PER_MINUTE*1000;
	time += second*1000;
	time += millisecond;
 
	return time;
}
//////////////////////////////////////////////
// End Unix2DateTimev1.0.lsl
//////////////////////////////////////////////