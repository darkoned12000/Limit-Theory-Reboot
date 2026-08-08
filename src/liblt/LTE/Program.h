#ifndef LTE_Program_h__
#define LTE_Program_h__

#include "Window.h"

struct Program {
  Window window;
  bool deleted;

  LT_API Program();
  LT_API virtual ~Program();

  LT_API void Delete();
  LT_API void Execute();

  virtual void OnInitialize() = 0;
  virtual void OnUpdate() = 0;
  virtual void OnDelete() {}
};

/* Returns the program currently being executed by Program::Execute, or null. */
LT_API Program* Program_GetCurrent();

/* Exit code the launcher returns once the program loop ends (0 by default).
   The self-test harness calls Program_Exit(code) to gate on failures. */
LT_API void Program_SetExitCode(int code);
LT_API int Program_GetExitCode();

LT_API bool Program_InStaticSection();

#endif
