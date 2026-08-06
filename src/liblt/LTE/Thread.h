#ifndef LTE_Thread_h__
#define LTE_Thread_h__

#include "Reference.h"

struct ThreadT : public RefCounted {
  virtual ~ThreadT() = default;

  virtual Job GetJob() const = 0;
  virtual bool IsFinished() const = 0;
  virtual void Terminate() = 0;
  virtual void Wait() = 0;
};

LT_API Thread Thread_Create(Job const& job);

LT_API void Thread_SleepMS(
  uint const& ms);
LT_API void Thread_SleepUS(
  uint const& us);

#endif
