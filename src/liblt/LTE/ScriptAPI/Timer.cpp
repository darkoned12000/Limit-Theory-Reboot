#include "LTE/Timer.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static Function const Timer_Registration = Function_Bind(
  "Timer",
  "Create and reset a new timer",
  []() -> Timer*
  {
  return new Timer;
  });

static Function const Timer_GetElapsed_Registration = Function_Bind(
  "Timer_GetElapsed",
  "Return the number of seconds elapsed since 'timer' was reset",
  [](Timer* const& timer) -> float
  {
  return timer->GetElapsed();
  },
  "timer");
static int const Timer_GetElapsed_Alias = Function_Alias("Timer_GetElapsed", "GetElapsed");

static Function const Timer_Reset_Registration = Function_Bind(
  "Timer_Reset",
  "Reset 'timer'",
  [](Timer* const& timer)
  {
  Mutable(*timer).Reset();
  },
  "timer");
static int const Timer_Reset_Alias = Function_Alias("Timer_Reset", "Reset");
