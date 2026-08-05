#ifndef Game_Events_h__
#define Game_Events_h__

#include "Event.h"
#include "Item.h"
#include "Object.h"
#include "LTE/AutoClass.h"


AutoClass(Event_Damage_Args,
  Object, source,
  Object, dest,
  Damage, damage)
  Event_Damage_Args() {}
};

LT_API Event Event_Damage(Event_Damage_Args const& args);
inline Event Event_Damage(
  Object const& source, Object const& dest, Damage const& damage) {
  return Event_Damage(Event_Damage_Args(source, dest, damage));
}

AutoClass(Event_Deposit_Args,
  Object, object,
  Object, target,
  Item, item,
  Quantity, quantity)
  Event_Deposit_Args() {}
};

LT_API Event Event_Deposit(Event_Deposit_Args const& args);
inline Event Event_Deposit(
  Object const& object, Object const& target, Item const& item, Quantity const& quantity) {
  return Event_Deposit(Event_Deposit_Args(object, target, item, quantity));
}

AutoClass(Event_Destroyed_Args,
  Object, source,
  Object, dest)
  Event_Destroyed_Args() {}
};

LT_API Event Event_Destroyed(Event_Destroyed_Args const& args);
inline Event Event_Destroyed(
  Object const& source, Object const& dest) {
  return Event_Destroyed(Event_Destroyed_Args(source, dest));
}

AutoClass(Event_Mined_Args,
  Object, object,
  Object, target,
  Item, item,
  Quantity, quantity)
  Event_Mined_Args() {}
};

LT_API Event Event_Mined(Event_Mined_Args const& args);
inline Event Event_Mined(
  Object const& object, Object const& target, Item const& item, Quantity const& quantity) {
  return Event_Mined(Event_Mined_Args(object, target, item, quantity));
}

#if 0
DeclareFunction(Event_Withdraw, Event,
  Object, object,
  Object, target,
  Item, item,
  Quantity, quantity)
#endif

#endif
