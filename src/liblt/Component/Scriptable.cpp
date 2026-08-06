#include "Scriptable.h"
#include "Game/Object.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static Function const Object_AddScript_Registration = Function_Bind(
  "Object_AddScript",
  "Add 'script' to 'object' to be executed each frame during update",
  [](Object const& object, Data const& script)
  {
  object->AddScript(script);
  },
  "object", "script");
static int const Object_AddScript_Alias = Function_Alias("Object_AddScript", "AddScript");
