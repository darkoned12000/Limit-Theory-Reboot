#include "../Items.h"

#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Value.h"
#include "LTE/FunctionBind.h"

/* Engineer - Production
 * Miner - Mining
 * Pilot - Transportation
 * Researcher - Research
 */

using WorkerBase = 
    Attribute_Icon
  < Attribute_Name
  < Attribute_Value
  < ItemWrapper<ItemType_Worker>
  > > >;

AutoClassDerived(WorkerEngineer, WorkerBase,
  uint, level,
  Item, nextLevel)
  DERIVED_TYPE_EX(WorkerEngineer)

  WorkerEngineer() = default;

  uint GetSkillEngineering() const override {
    return level;
  }
};

AutoClassDerived(WorkerMiner, WorkerBase,
  uint, level,
  Item, nextLevel)
  DERIVED_TYPE_EX(WorkerMiner)

  WorkerMiner() = default;

  uint GetSkillMiner() const {
    return level;
  }
};

AutoClassDerived(WorkerPilot, WorkerBase,
  uint, level,
  Item, nextLevel)
  DERIVED_TYPE_EX(WorkerPilot)

  WorkerPilot() = default;

  uint GetSkillPiloting() const override {
    return level;
  }
};

Item Item_Worker_Engineer(Item_Worker_Engineer_Args const& args) {
  return new WorkerEngineer(args.level, args.nextLevel);
}
static Function const Item_Worker_Engineer_Registration = Function_Bind(
  "Item_Worker_Engineer",
  "None",
  [](uint const& level, Item const& nextLevel) -> Item { return Item_Worker_Engineer(level, nextLevel); },
  "level", "nextLevel");



Item Item_Worker_Miner(Item_Worker_Miner_Args const& args) {
  return new WorkerMiner(args.level, args.nextLevel);
}
static Function const Item_Worker_Miner_Registration = Function_Bind(
  "Item_Worker_Miner",
  "None",
  [](uint const& level, Item const& nextLevel) -> Item { return Item_Worker_Miner(level, nextLevel); },
  "level", "nextLevel");



Item Item_Worker_Pilot(Item_Worker_Pilot_Args const& args) {
  return new WorkerPilot(args.level, args.nextLevel);
}
static Function const Item_Worker_Pilot_Registration = Function_Bind(
  "Item_Worker_Pilot",
  "None",
  [](uint const& level, Item const& nextLevel) -> Item { return Item_Worker_Pilot(level, nextLevel); },
  "level", "nextLevel");


