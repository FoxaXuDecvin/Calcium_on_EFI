#include <ctime>
#define _CRT_SECURE_NO_WARNINGS

const std::string __Time_Year = "TIMEYEAR";
const std::string __Time_Month = "TIMEMONTH";
const std::string __Time_Day = "TIMEDAY";
const std::string __Time_Hour = "TIMEHOUR";
const std::string __Time_Min = "TIMEMIN";
const std::string __Time_Sec = "TIMESEC";

//Support 
//__Time_Year
//__Time_Month
//__Time_Day
//__Time_Hour 
//__Time_Min
//___Time_Sec
//_SH_TIME_H
//Get Current Time API
int tmcache;
std::string tmscache;
std::string __GetCurrentTimeAPI(std::string __TimeMode, bool return_RAW) {
	time_t now = time(NULL);
	tm* tm_t = localtime(&now);

	tmcache = 0;

	if (__TimeMode == __Time_Year) {
		tmcache = tm_t->tm_year;
		if (!return_RAW) {
			tmcache = tmcache + 1900;
		}
	}
	if (__TimeMode == __Time_Month) {
		tmcache = tm_t->tm_mon;
		if (!return_RAW) {
			tmcache = tmcache + 1;
		}
	}
	if (__TimeMode == __Time_Day) {
		tmcache = tm_t->tm_mday;
	}

	if (__TimeMode == __Time_Hour) {
		tmcache = tm_t->tm_hour;
	}
	if (__TimeMode == __Time_Min) {
		tmcache = tm_t->tm_min;
	}
	if (__TimeMode == __Time_Sec) {
		tmcache = tm_t->tm_sec;
	}

	tmscache = std::to_string(tmcache);

	if (!return_RAW) {
		if (tmscache.size() == 1) {
			tmscache = "0" + tmscache;
		}
	}

	return tmscache;
}
std::string fullcache;

//Return 
// Year/Month/Day,H:M:S
std::string __GetFullTime(void) {
	fullcache = __GetCurrentTimeAPI(__Time_Year, false) + "/" + __GetCurrentTimeAPI(__Time_Month, false) + "/" + __GetCurrentTimeAPI(__Time_Day, false) + "," + __GetCurrentTimeAPI(__Time_Hour, false) + ":" + __GetCurrentTimeAPI(__Time_Min, false) + ":" + __GetCurrentTimeAPI(__Time_Sec, false);

	return fullcache;
}

//Return 
// H:M:S
std::string __GetFullClock(void) {
	fullcache = __GetCurrentTimeAPI(__Time_Hour, false) + ":" + __GetCurrentTimeAPI(__Time_Min, false) + ":" + __GetCurrentTimeAPI(__Time_Sec, false);

	return fullcache;
}

//Return
// Year/Month/Day
std::string __GetFullDate(void) {
	fullcache = __GetCurrentTimeAPI(__Time_Year, false) + "/" + __GetCurrentTimeAPI(__Time_Month, false) + "/" + __GetCurrentTimeAPI(__Time_Day, false);

	return fullcache;
}


//Support 
//__Time_Year
//__Time_Month
//__Time_Day
//__Time_Hour 
//__Time_Min
//___Time_Sec
//_SH_TIME_H
//Get Current Time API
//Return Int Number
int _GetCurrentTimeAPI(std::string __TimeMode, bool return_RAW) {
	return atoi(__GetCurrentTimeAPI(__TimeMode, return_RAW).c_str());
}