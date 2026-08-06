#include "../Events.h"

#include "LTE/Pool.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(EventMined, EventT, Event_Mined_Args, args)
    DERIVED_TYPE_EX(EventMined)
    POOLED_TYPE

    EventMined() = default;
  };
}

Event Event_Mined(Event_Mined_Args const& args) {
  return new EventMined(args);
}
static Function const Event_Mined_Registration = Function_Bind(
  "Event_Mined",
  "None",
  [](Object const& object, Object const& target, Item const& item, Quantity const& quantity) -> Event { return Event_Mined(object, target, item, quantity); },
  "object", "target", "item", "quantity");


