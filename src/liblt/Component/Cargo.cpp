#include "Cargo.h"
#include "Game/Object.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

bool ComponentCargo::Add(Item const& item, Quantity count, bool force) {
  if (count == 0)
    return true;

  /* Check if we have enough room. */
  Mass requiredMass = item->GetMass() * static_cast<float>(count);
  Mass freeMass = capacity - currentMass;
  if (!force &&
      requiredMass > 0 &&
      freeMass < requiredMass)
    return false;

  if ((elements[item] += count) == 0)
    elements.erase(item);

  /* NOTE : We need to be careful when using floats. Error will accumulate,
   *        so instead of using accumulation we recompute current mass after
   *        each operation. Keep an eye on performance. */
#if 0
   currentMass += requiredMass;
#else
  currentMass = 0;
  for (CargoIter it = elements.begin(); it != elements.end(); ++it)
    currentMass += it->first->GetMass() * static_cast<float>(it->second);
#endif

  return true;
}

AutoClass(CargoIterator,
  CargoIter, iterator,
  Object, object)

  CargoIterator() = default;
};

static Function const Object_GetCargo_Registration = Function_Bind(
  "Object_GetCargo",
  "Return an iterator to the cargo contents of 'object'",
  [](Object const& object) -> CargoIterator
  {
  return CargoIterator(object->GetCargo()->elements.begin(), object);
  },
  "object");
static int const Object_GetCargo_Alias = Function_Alias("Object_GetCargo", "GetCargo");

static Function const CargoIterator_Advance_Registration = Function_Bind(
  "CargoIterator_Advance",
  "Advance 'iterator'",
  [](CargoIterator const& iterator)
  {
  ++((CargoIterator&)iterator).iterator;
  },
  "iterator");
static int const CargoIterator_Advance_Alias = Function_Alias("CargoIterator_Advance", "Advance");

static Function const CargoIterator_HasMore_Registration = Function_Bind(
  "CargoIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](CargoIterator const& iterator) -> bool
  {
  return iterator.iterator != iterator.object->GetCargo()->elements.end();
  },
  "iterator");
static int const CargoIterator_HasMore_Alias = Function_Alias("CargoIterator_HasMore", "HasMore");

static Function const CargoIterator_Item_Registration = Function_Bind(
  "CargoIterator_Item",
  "Return the item in 'iterator'",
  [](CargoIterator const& iterator) -> Item
  {
  return iterator.iterator->first;
  },
  "iterator");
static int const CargoIterator_Item_Alias = Function_Alias("CargoIterator_Item", "GetItem");

static Function const CargoIterator_Quantity_Registration = Function_Bind(
  "CargoIterator_Quantity",
  "Return the quantity in 'iterator'",
  [](CargoIterator const& iterator) -> Quantity
  {
  return iterator.iterator->second;
  },
  "iterator");
static int const CargoIterator_Quantity_Alias = Function_Alias("CargoIterator_Quantity", "GetQuantity");

static Function const Object_AddItem_Registration = Function_Bind(
  "Object_AddItem",
  "Add 'quantity' of 'item' to 'object', return whether operation was successful",
  [](Object const& object, Item const& item, Quantity const& quantity) -> bool
  {
  return object->AddItem(item, quantity);
  },
  "object", "item", "quantity");
static int const Object_AddItem_Alias = Function_Alias("Object_AddItem", "AddItem");

static Function const Object_GetCapacity_Registration = Function_Bind(
  "Object_GetCapacity",
  "Return the total cargo capacity of 'object'",
  [](Object const& object) -> Quantity
  {
  return object->GetCapability().Storage;
  },
  "object");
static int const Object_GetCapacity_Alias = Function_Alias("Object_GetCapacity", "GetCapacity");

static Function const Object_GetItemCount_Registration = Function_Bind(
  "Object_GetItemCount",
  "Return the the number of 'item' in 'object's cargo",
  [](Object const& object, Item const& item) -> Quantity
  {
  return object->GetItemCount(item);
  },
  "object", "item");
static int const Object_GetItemCount_Alias = Function_Alias("Object_GetItemCount", "GetItemCount");

static Function const Object_GetUsedCapacity_Registration = Function_Bind(
  "Object_GetUsedCapacity",
  "Return the capacity of 'object's cargo that is in use",
  [](Object const& object) -> Quantity
  {
  return object->GetUsedCapacity();
  },
  "object");
static int const Object_GetUsedCapacity_Alias = Function_Alias("Object_GetUsedCapacity", "GetUsedCapacity");

static Function const Object_RemoveItem_Registration = Function_Bind(
  "Object_RemoveItem",
  "Remove 'quantity' of 'item' from 'object', return whether operation was successful",
  [](Object const& object, Item const& item, Quantity const& quantity) -> bool
  {
  return object->RemoveItem(item, quantity);
  },
  "object", "item", "quantity");
static int const Object_RemoveItem_Alias = Function_Alias("Object_RemoveItem", "RemoveItem");
