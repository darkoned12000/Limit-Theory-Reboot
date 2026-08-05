#ifndef LTE_Time_h__
#define LTE_Time_h__

#include "AutoClass.h"
#include "Common.h"
#include "LTE/AutoClass.h"

AutoClass(Time,
  uint, second,
  uint, minute,
  uint, hour,
  uint, day,
  uint, month,
  uint, year)
  Time() = default;
};

LT_API Time Time_Current();

#endif
