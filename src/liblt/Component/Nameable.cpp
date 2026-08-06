#include "Nameable.h"
#include "Game/Object.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static Function const Object_GetName_Registration = Function_Bind(
  "Object_GetName",
  "Return the name of 'object'",
  [](Object const& object) -> String
  {
  return object->GetName();
  },
  "object");
static int const Object_GetName_Alias = Function_Alias("Object_GetName", "GetName");

static Function const Object_SetName_Registration = Function_Bind(
  "Object_SetName",
  "Set the name of 'object' to 'name'",
  [](Object const& object, String const& name)
  {
  return object->SetName(name);
  },
  "object", "name");
static int const Object_SetName_Alias = Function_Alias("Object_SetName", "SetName");
