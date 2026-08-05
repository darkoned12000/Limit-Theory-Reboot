#include "Time.h"
#include <ctime>
#include "LTE/FunctionBind.h"

Time Time_Current() {
  Time self;
  time_t time = std::time(0);
  struct std::tm* localTime = std::localtime(&time);
  self.second = localTime->tm_sec;
  self.minute = localTime->tm_min;
  self.hour = localTime->tm_hour;
  self.day = localTime->tm_mday;
  self.month = localTime->tm_mon + 1;
  self.year = 1900 + localTime->tm_year;
  return self;
}
static Function const Time_Current_Registration = Function_Bind(
  "Time_Current",
  "None",
  &Time_Current);


