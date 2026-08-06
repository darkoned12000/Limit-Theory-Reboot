#include "Log.h"
#include "Game/Object.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

AutoClass(LogIterator,
  Object, object,
  uint, index)
  LogIterator() = default;
};

static Function const Object_AddLog_Registration = Function_Bind(
  "Object_AddLog",
  "Add 'message' to 'object's list of logged messages",
  [](Object const& object, String const& message)
  {
  object->AddLogMessage(message);
  },
  "object", "message");
static int const Object_AddLog_Alias = Function_Alias("Object_AddLog", "AddLog");

static Function const Object_GetLog_Registration = Function_Bind(
  "Object_GetLog",
  "Return an iterator to the log entries of 'object'",
  [](Object const& object) -> LogIterator
  {
  return LogIterator(object, 0);
  },
  "object");
static int const Object_GetLog_Alias = Function_Alias("Object_GetLog", "GetLog");

static Function const LogIterator_Advance_Registration = Function_Bind(
  "LogIterator_Advance",
  "Advance 'iterator'",
  [](LogIterator const& iterator)
  {
  Mutable(iterator).index++;
  },
  "iterator");
static int const LogIterator_Advance_Alias = Function_Alias("LogIterator_Advance", "Advance");

static Function const LogIterator_Get_Registration = Function_Bind(
  "LogIterator_Get",
  "Return the contents of 'iterator'",
  [](LogIterator const& iterator) -> LogEntry
  {
  return iterator.object->GetLog()->elements[iterator.index];
  },
  "iterator");
static int const LogIterator_Get_Alias = Function_Alias("LogIterator_Get", "Get");

static Function const LogIterator_HasMore_Registration = Function_Bind(
  "LogIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](LogIterator const& iterator) -> bool
  {
  return iterator.object->GetLog() &&
    iterator.index < iterator.object->GetLog()->elements.size();
  },
  "iterator");
static int const LogIterator_HasMore_Alias = Function_Alias("LogIterator_HasMore", "HasMore");

static Function const LogIterator_Size_Registration = Function_Bind(
  "LogIterator_Size",
  "Return the total number of elements in 'iterator'",
  [](LogIterator const& iterator) -> int
  {
  return iterator.object->GetLog()
    ? static_cast<int>(iterator.object->GetLog()->elements.size())
    : 0;
  },
  "iterator");
static int const LogIterator_Size_Alias = Function_Alias("LogIterator_Size", "Size");
