#include "Game/Task.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

TypeAlias(Reference<TaskT>, Task);

static Function const Task_GetName_Registration = Function_Bind(
  "Task_GetName",
  "Return the name of 'task'",
  [](Task const& task) -> String
  {
  return task->GetName();
  },
  "task");
static int const Task_GetName_Alias = Function_Alias("Task_GetName", "GetName");
