#ifndef LTE_Profiler_h__
#define LTE_Profiler_h__

#include "Common.h"

#define ENABLE_PROFILING

#ifdef ENABLE_PROFILING

LT_API void Profiler_Flush();
LT_API void Profiler_Pop();
LT_API void Profiler_Push(char const* name);

#else

inline void Profiler_Flush() {}
inline void Profiler_Pop() {}
inline void Profiler_Push(char const* name) {}

#endif

LT_API void Profiler_Auto(
  float const& duration);
LT_API void Profiler_Start();
LT_API void Profiler_Stop();

LT_API void Profiler_SetFlushes(bool flushes);

#endif
