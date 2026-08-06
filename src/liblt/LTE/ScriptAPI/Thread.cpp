#include "LTE/AutoClass.h"
#include "LTE/Data.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Job.h"
#include "LTE/ProgramLog.h"
#include "LTE/Script.h"
#include "LTE/Thread.h"

TypeAlias(Reference<ThreadT>, Thread);

AutoClassDerived(ScriptedJob, JobT,
  Data, object,
  ScriptFunction, function,
  void*, returnValue)

  ScriptedJob() = default;

  ~ScriptedJob() override {
    if (returnValue)
      function->returnType->Deallocate(returnValue);
  }

  char const* GetName() const override {
    return &function->name.front();
  }

  void OnBegin() override {
    if (function->returnType->allocate)
      returnValue = function->returnType->Allocate();
  }

  void OnRun(uint units) override {
    function->VoidCall(returnValue, object);
  }
};

static Function const Thread_Create_Registration = Function_Bind(
  "Thread_Create",
  "Create a thread that executes the function named 'function' in 'object'",
  [](Data const& object, String const& function) -> Thread
  {
  ScriptType type = object.type->GetAux().Convert<ScriptType>();
  ScriptFunction fn = type->GetFunction(function);
  if (!fn) {
    Log_Error(Stringize()
      | "Thread object '" | object.type->name
      | "' has no function '" | function | "'");
    return nullptr;
  }

  return Thread_Create(new ScriptedJob(object, fn, 0));
  },
  "object", "function");

static Function const Thread_IsFinished_Registration = Function_Bind(
  "Thread_IsFinished",
  "Return whether 'thread' has finished executing",
  [](Thread const& thread) -> bool
  {
  return thread->IsFinished();
  },
  "thread");
static int const Thread_IsFinished_Alias = Function_Alias("Thread_IsFinished", "IsFinished");

static Function const Thread_GetResult_Registration = Function_Bind(
  "Thread_GetResult",
  "Get the return value (if any) of 'thread'",
  [](Thread const& thread) -> Data
  {
  /* Block until the worker has finished and joined. Joining provides a
     full memory synchronization, so the result written by the worker is
     guaranteed to be visible here (a plain IsFinished poll has no such
     guarantee and previously read a torn / uninitialized value). */
  thread->Wait();
  ScriptedJob* job = (ScriptedJob*)thread->GetJob().t;
  return Data(job->function->returnType, job->returnValue);
  },
  "thread");
static int const Thread_GetResult_Alias = Function_Alias("Thread_GetResult", "GetResult");

static Function const Thread_GetResultInt_Registration = Function_Bind(
  "Thread_GetResultInt",
  "Get the integer return value of 'thread' (convenience for Int-returning jobs)",
  [](Thread const& thread) -> int
  {
  thread->Wait();
  ScriptedJob* job = (ScriptedJob*)thread->GetJob().t;
  return job->returnValue ? *(int*)job->returnValue : 0;
  },
  "thread");
static int const Thread_GetResultInt_Alias = Function_Alias("Thread_GetResultInt", "GetResultInt");
