#include "LTE/Reference.h"
#include "LTE/RNG.h"
#include "LTE/VectorNP.h"
#include "LTE/FunctionBind.h"

TypeAlias(ListNP, List);

namespace Priv1 {
  static Function const List_Registration = Function_Bind(
  "List",
  "Create an empty list",
  []() -> ListNP
  {
    return new ListNPT;
  
  });
}

namespace Priv2 {
  static Function const List_Registration = Function_Bind(
  "List",
  "Create a list",
  [](Data const& elem) -> ListNP
  {
    ListNP list = new ListNPT(elem.type);
    list->Append(DataRef(elem.type, elem.data));
    return list;
  
  },
  "elem");
}

static Function const List_Append_Registration = Function_Bind(
  "List_Append",
  "Append 'elem' to 'list'",
  [](ListNP const& list, Data const& elem)
  {
  list->Append(DataRef(elem.type, elem.data));
  },
  "list", "elem");
static int const List_Append_Alias = Function_Alias("List_Append", "+=");

static Function const List_Clear_Registration = Function_Bind(
  "List_Clear",
  "Clear the contents of 'list'",
  [](ListNP const& list)
  {
  list->Clear();
  },
  "list");
static int const List_Clear_Alias = Function_Alias("List_Clear", "Clear");

static Function const List_Get_Registration = Function_Bind(
  "List_Get",
  "Return element at index 'i' in 'list'",
  [](ListNP const& list, int const& index) -> Data
  {
  return (*list)[index];
  },
  "list", "index");
static int const List_Get_Alias = Function_Alias("List_Get", "Get");

static Function const List_GetRandom_Registration = Function_Bind(
  "List_GetRandom",
  "Returns a random element of 'list' using 'rng'",
  [](ListNP const& list, RNG const& rng) -> Data
  {
  return (*list)[rng->GetInt(0, list->size - 1)];
  },
  "list", "rng");
static int const List_GetRandom_Alias = Function_Alias("List_GetRandom", "GetRandom");

static Function const List_Set_Registration = Function_Bind(
  "List_Set",
  "Set element at index 'i' in 'list' to 'elem'",
  [](ListNP const& list, int const& index, Data const& elem)
  {
  (*list)[index] = elem;
  },
  "list", "index", "elem");
static int const List_Set_Alias = Function_Alias("List_Set", "Set");

static Function const List_Size_Registration = Function_Bind(
  "List_Size",
  "Return the number of elements in 'list'",
  [](ListNP const& list) -> int
  {
  return list->size;
  },
  "list");
static int const List_Size_Alias = Function_Alias("List_Size", "Size");

static Function const List_Shuffle_Registration = Function_Bind(
  "List_Shuffle",
  "Randomize the order of elements in 'list' using 'rng'",
  [](ListNP const& list, RNG const& rng)
  {
  for (size_t i = 0; i + 1 < list->Size(); ++i) {
    int index = rng->GetInt2(i, list->Size() - 1);
    Data elem = (*list)[i];
    (*list)[i] = (*list)[index];
    (*list)[index] = elem;
  }
  },
  "list", "rng");
static int const List_Shuffle_Alias = Function_Alias("List_Shuffle", "Shuffle");
