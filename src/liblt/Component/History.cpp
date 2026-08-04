#include "History.h"

#include "Game/Object.h"
#include "LTE/FunctionBind.h"

void ComponentHistory::Run(ObjectT* self, UpdateState& state) {
}

DefineFunction(Object_AddHistory) {
  ComponentHistory* history = args.object->GetHistory();
  LTE_ASSERT(history != nullptr);
  history->elements.push(args.event);
}

AutoClass(HistoryIterator,
  Object, object,
  uint, index)
  HistoryIterator() = default;
};

static Function const Object_GetHistory_Registration = Function_Bind(
  "Object_GetHistory",
  "Return an iterator to the elements of 'object's history",
  [](Object const& object) -> HistoryIterator
  {
  return HistoryIterator(object, 0);
  },
  "object");
static int const Object_GetHistory_Alias = Function_Alias("Object_GetHistory", "GetHistory");

static Function const HistoryIterator_Advance_Registration = Function_Bind(
  "HistoryIterator_Advance",
  "Advance 'iterator'",
  [](HistoryIterator const& iterator)
  {
  Mutable(iterator).index++;
  },
  "iterator");
static int const HistoryIterator_Advance_Alias = Function_Alias("HistoryIterator_Advance", "Advance");

static Function const HistoryIterator_Get_Registration = Function_Bind(
  "HistoryIterator_Get",
  "Return the contents of 'iterator'",
  [](HistoryIterator const& iterator) -> Event
  {
  return iterator.object->GetHistory()->elements[iterator.index];
  },
  "iterator");
static int const HistoryIterator_Get_Alias = Function_Alias("HistoryIterator_Get", "Get");

static Function const HistoryIterator_HasMore_Registration = Function_Bind(
  "HistoryIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](HistoryIterator const& iterator) -> bool
  {
  return iterator.object->GetHistory() &&
    iterator.index < iterator.object->GetHistory()->elements.size();
  },
  "iterator");
static int const HistoryIterator_HasMore_Alias = Function_Alias("HistoryIterator_HasMore", "HasMore");

static Function const HistoryIterator_Size_Registration = Function_Bind(
  "HistoryIterator_Size",
  "Return the total number of elements in 'iterator'",
  [](HistoryIterator const& iterator) -> int
  {
  return iterator.object->GetHistory()
    ? static_cast<int>(iterator.object->GetHistory()->elements.size())
    : 0;
  },
  "iterator");
static int const HistoryIterator_Size_Alias = Function_Alias("HistoryIterator_Size", "Size");
