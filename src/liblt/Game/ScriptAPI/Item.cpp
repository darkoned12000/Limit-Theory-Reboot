#include "Game/Item.h"
#include "Game/Object.h"
#include "Game/Task.h"

#include "LTE/Color.h"
#include "LTE/Data.h"
#include "LTE/Renderable.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

#include "UI/Icon.h"

TypeAlias(Reference<ItemT>, Item);

static Function const Item_Equal_Registration = Function_Bind(
  "Item_Equal",
  "Return a == b",
  [](Item const& a, Item const& b) -> bool
  {
  return a == b;
  },
  "a", "b");
static int const Item_Equal_Alias = Function_Alias("Item_Equal", "==");

static Function const Item_GetTypeString_Registration = Function_Bind(
  "Item_GetTypeString",
  "Return the type of 'item'",
  [](Item const& item) -> String
  {
  return ItemType_String[item->GetType()];
  },
  "item");
static int const Item_GetTypeString_Alias = Function_Alias("Item_GetTypeString", "GetTypeString");

static Function const Item_NotEqual_Registration = Function_Bind(
  "Item_NotEqual",
  "Return a != b",
  [](Item const& a, Item const& b) -> bool
  {
  return a != b;
  },
  "a", "b");
static int const Item_NotEqual_Alias = Function_Alias("Item_NotEqual", "!=");

static Function const Item_Instantiate_Registration = Function_Bind(
  "Item_Instantiate",
  "Return an object instantiation of 'item'",
  [](Item const& item) -> Object
  {
  return item->Instantiate();
  },
  "item");
static int const Item_Instantiate_Alias = Function_Alias("Item_Instantiate", "Instantiate");

#define X(type, name, init)                                                    \
  static Function const Item_Has##name##_Registration =                        \
    Function_Bind(                                                             \
      "Item_Has" #name,                                                        \
      "Return whether 'item' has the " #name " attribute",                     \
      [](Item const& item) -> bool { return item->Has##name(); },              \
      "item");                                                                 \
  static int const Item_Has##name##_Alias = Function_Alias(                    \
    "Item_Has" #name, "Has" #name);                                            \
                                                                               \
  static Function const Item_Get##name##_Registration =                        \
    Function_Bind(                                                             \
      "Item_Get" #name,                                                        \
      "Return the " #name " of 'item'",                                        \
      [](Item const& item) -> type { return item->Get##name(); },              \
      "item");                                                                 \
  static int const Item_Get##name##_Alias = Function_Alias(                    \
    "Item_Get" #name, "Get" #name);                                            \
                                                                               \
  static Function const Item_Set##name##_Registration =                        \
    Function_Bind(                                                             \
      "Item_Set" #name,                                                        \
      "Set the " #name " of 'item' to 'value'",                                \
      [](Item const& item, type const& value)                                  \
      {                                                                        \
        if (item->Has##name())                                                 \
          (type&)item->Get##name() = value;                                    \
      },                                                                       \
      "item", "value");                                                        \
  static int const Item_Set##name##_Alias = Function_Alias(                    \
    "Item_Set" #name, "Set" #name);
ITEMPROPERTY_X
#undef X

#define Z(x, y, z)                                                             \
  static Function const Item_Is##y##_Registration =                            \
    Function_Bind(                                                             \
      "Item_Is" #y,                                                            \
      "Return whether 'item' is a " z,                                         \
      [](Item const& item) -> bool                                             \
      { return item->GetType() == ItemType_##y; },                             \
      "item");                                                                 \
  static int const Item_Is##y##_Alias = Function_Alias(                        \
    "Item_Is" #y, "Is" #y);
ITEM_X
#undef Z
