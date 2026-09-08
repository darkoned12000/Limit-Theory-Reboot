#ifndef LTE_ProgramLog_h__
#define LTE_ProgramLog_h__

#include "String.h"

namespace LTE {
  namespace LogLevel {
    enum Enum {
      Nothing,
      Errors,
      Warnings,
      Everything
    };
  }

  LT_API void Log_Critical(String const& entry);
  LT_API void Log_Error(String const& entry);
  LT_API void Log_Event(String const& entry);
  LT_API void Log_Message(String const& entry);
  LT_API void Log_Warning(String const& entry);

  LT_API size_t Log_GetEntries();
  LT_API String const& Log_GetEntry(int index);

  /* P1-3 runtime error channel — scalar failure-entry access. Lets the
     debug overlay (DebugScene.lts) list recent failure entries without
     exposing Vector<String> through the script type system. An entry is a
     failure when it was logged via Log_Error/Log_Critical ([Error]/
     [CRITICAL] prefixes). */
  LT_API size_t Log_GetFailureCount();
  LT_API String const& Log_GetFailure(int index);
}

#endif
