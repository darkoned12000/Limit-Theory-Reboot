#include "Resources.h"

#include "Game/Item.h"
#include "Game/Object.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static Function const Object_AddResource_Registration = Function_Bind(
  "Object_AddResource",
  "Add 'item' to 'objects' list of natural resources with weight 'w'",
  [](Object const& object, Item const& item, float const& weight)
  {
  ComponentResources* resources = object->GetResources();
  LTE_ASSERT(resources != nullptr);
  resources->elements[item] = weight;
  },
  "object", "item", "weight");
static int const Object_AddResource_Alias = Function_Alias("Object_AddResource", "AddResource");
