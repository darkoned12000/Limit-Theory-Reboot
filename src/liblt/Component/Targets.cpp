#include "Targets.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

AutoClass(TargetIterator,
  Object, object,
  uint, index)
  TargetIterator() = default;
};

static Function const Object_AddTarget_Registration = Function_Bind(
  "Object_AddTarget",
  "Add 'target' to 'object's list of targets",
  [](Object const& object, Object const& target)
  {
  object->GetTargets()->elements.push(target);
  },
  "object", "target");
static int const Object_AddTarget_Alias = Function_Alias("Object_AddTarget", "AddTarget");

static Function const Object_GetTargets_Registration = Function_Bind(
  "Object_GetTargets",
  "Return an iterator to the targets of 'object'",
  [](Object const& object) -> TargetIterator
  {
  return TargetIterator(object, 0);
  },
  "object");
static int const Object_GetTargets_Alias = Function_Alias("Object_GetTargets", "GetTargets");

static Function const Object_HasTarget_Registration = Function_Bind(
  "Object_HasTarget",
  "Return whether 'object' has 'target' currently targetted",
  [](Object const& object, Object const& target) -> bool
  {
  return object->GetTargets() && object->GetTargets()->elements.contains(target);
  },
  "object", "target");
static int const Object_HasTarget_Alias = Function_Alias("Object_HasTarget", "HasTarget");

static Function const Object_RemoveTarget_Registration = Function_Bind(
  "Object_RemoveTarget",
  "Remove 'target' from 'object's list of targets",
  [](Object const& object, Object const& target)
  {
  object->GetTargets()->elements.remove(target);
  },
  "object", "target");
static int const Object_RemoveTarget_Alias = Function_Alias("Object_RemoveTarget", "RemoveTarget");

static Function const TargetIterator_Advance_Registration = Function_Bind(
  "TargetIterator_Advance",
  "Advance 'iterator'",
  [](TargetIterator const& iterator)
  {
  Mutable(iterator).index++;
  },
  "iterator");
static int const TargetIterator_Advance_Alias = Function_Alias("TargetIterator_Advance", "Advance");

static Function const TargetIterator_Get_Registration = Function_Bind(
  "TargetIterator_Get",
  "Return the contents of 'iterator'",
  [](TargetIterator const& iterator) -> Object
  {
  return iterator.object->GetTargets()->elements[iterator.index];
  },
  "iterator");
static int const TargetIterator_Get_Alias = Function_Alias("TargetIterator_Get", "Get");

static Function const TargetIterator_HasMore_Registration = Function_Bind(
  "TargetIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](TargetIterator const& iterator) -> bool
  {
  return iterator.object->GetTargets() &&
    iterator.index < iterator.object->GetTargets()->elements.size();
  },
  "iterator");
static int const TargetIterator_HasMore_Alias = Function_Alias("TargetIterator_HasMore", "HasMore");

static Function const TargetIterator_Size_Registration = Function_Bind(
  "TargetIterator_Size",
  "Return the total number of elements in 'iterator'",
  [](TargetIterator const& iterator) -> int
  {
  return iterator.object->GetTargets()
    ? static_cast<int>(iterator.object->GetTargets()->elements.size())
    : 0;
  },
  "iterator");
static int const TargetIterator_Size_Alias = Function_Alias("TargetIterator_Size", "Size");
