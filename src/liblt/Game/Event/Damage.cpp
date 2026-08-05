#include "../Events.h"

#include "Game/Items.h"
#include "Game/Player.h"
#include "LTE/Pool.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(EventDamage, EventT, Event_Damage_Args, args)
    DERIVED_TYPE_EX(EventDamage)
    POOLED_TYPE

    EventDamage() = default;

    String ToString() const override {
      return Stringize()
        | args.source->GetName() | " dealt  "
        | args.damage | " damage to "
        | args.dest->GetName();
    }
  };
}

Event Event_Damage(Event_Damage_Args const& args) {
  if (!args.dest->IsAlive())
    return nullptr;

  args.dest->ApplyDamage(args.damage);

  Pointer<ObjectT> sourceRoot = args.source->GetRoot();

  /* If the object has an owner, notify the owner that their asset is under
   * attack. */
  Player const& sourceOwner = sourceRoot->GetOwner();
  Player const& destOwner = args.dest->GetOwner();
  if (sourceOwner && destOwner)
    destOwner->OnAttacked(sourceOwner);

  /* Record a log of this damage. */
  sourceRoot->AddItem(Item_Data_Damaged(args.dest), args.damage);

  return new EventDamage(args);
}
static Function const Event_Damage_Registration = Function_Bind(
  "Event_Damage",
  "None",
  [](Object const& source, Object const& dest, Damage const& damage) -> Event { return Event_Damage(source, dest, damage); },
  "source", "dest", "damage");


