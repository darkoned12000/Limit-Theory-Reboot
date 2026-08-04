#include "LTE/StringList.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

TypeAlias(Reference<StringListT>, StringList);

static Function const StringList_Get_Registration = Function_Bind(
  "StringList_Get",
  "Return the 'i'th child of 'list'",
  [](StringList const& list, int const& i) -> StringList
  {
  return list->Get(i);
  },
  "list", "i");
static int const StringList_Get_Alias = Function_Alias("StringList_Get", "Get");

static Function const StringList_GetValue_Registration = Function_Bind(
  "StringList_GetValue",
  "Return the value of 'list'",
  [](StringList const& list) -> String
  {
  return list->GetValue();
  },
  "list");
static int const StringList_GetValue_Alias = Function_Alias("StringList_GetValue", "GetValue");

static Function const StringList_IsAtom_Registration = Function_Bind(
  "StringList_IsAtom",
  "Return whether 'list' is an atom",
  [](StringList const& list) -> bool
  {
  return list->IsAtom();
  },
  "list");
static int const StringList_IsAtom_Alias = Function_Alias("StringList_IsAtom", "IsAtom");

static Function const StringList_Size_Registration = Function_Bind(
  "StringList_Size",
  "Return the size of 'list'",
  [](StringList const& list) -> int
  {
  return list->GetSize();
  },
  "list");
static int const StringList_Size_Alias = Function_Alias("StringList_Size", "Size");
