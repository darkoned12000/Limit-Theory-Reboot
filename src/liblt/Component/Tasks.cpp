#include "Tasks.h"
#include "Game/Tasks.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

void ComponentTasks::Clear(ObjectT* self) {
  while (elements.size()) {
    TaskInstance& current = elements.back();
    current.task->OnEnd(self, current.data);
    elements.pop();
  }
}

void ComponentTasks::Run(ObjectT* self, UpdateState& state) {
  if (elements.size() && !elements.back().IsFinished(self))
    elements.back().OnUpdate(self, state.dt);

  while (elements.size() && elements.back().IsFinished(self)) {
    TaskInstance& current = elements.back();
    current.task->OnEnd(self, current.data);
    elements.pop();
  }
}

static Function const Object_ClearTasks_Registration = Function_Bind(
  "Object_ClearTasks",
  "Clear all tasks from 'object's task stack",
  [](Object const& object)
  {
  object->ClearTasks();
  },
  "object");
static int const Object_ClearTasks_Alias = Function_Alias("Object_ClearTasks", "ClearTasks");

static Function const Object_GetCurrentTask_Registration = Function_Bind(
  "Object_GetCurrentTask",
  "Return 'object's active task",
  [](Object const& object) -> TaskInstance*
  {
  return (TaskInstance*)object->GetCurrentTask();
  },
  "object");
static int const Object_GetCurrentTask_Alias = Function_Alias("Object_GetCurrentTask", "GetCurrentTask");

static Function const Object_PushTask_Registration = Function_Bind(
  "Object_PushTask",
  "Push 'task' to 'object's task stack",
  [](Object const& object, Task const& task)
  {
  object->PushTask(task);
  },
  "object", "task");
static int const Object_PushTask_Alias = Function_Alias("Object_PushTask", "PushTask");
