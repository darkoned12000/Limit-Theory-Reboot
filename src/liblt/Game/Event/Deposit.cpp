#include "../Events.h"

#include "Game/Player.h"

#include "LTE/AutoClass.h"
#include "LTE/Math.h"
#include "LTE/Transform.h"
#include "LTE/FunctionBind.h"

Event Event_Deposit(Event_Deposit_Args const& args) {
  Quantity quantity = Min(args.quantity, args.object->GetItemCount(args.item));
  args.object->RemoveItem(args.item, quantity);
  args.target->GetStorageLocker(args.object->GetOwner())->AddItem(args.item, quantity);
  return nullptr;
}
static Function const Event_Deposit_Registration = Function_Bind(
  "Event_Deposit",
  "None",
  [](Object const& object, Object const& target, Item const& item, Quantity const& quantity) -> Event { return Event_Deposit(object, target, item, quantity); },
  "object", "target", "item", "quantity");


