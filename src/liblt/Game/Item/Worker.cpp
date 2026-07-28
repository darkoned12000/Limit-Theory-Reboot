#include "../Items.h"

#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Value.h"

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

DefineFunction(Item_Worker_Engineer) {
  return new WorkerEngineer(args.level, args.nextLevel);
}

DefineFunction(Item_Worker_Miner) {
  return new WorkerMiner(args.level, args.nextLevel);
}

DefineFunction(Item_Worker_Pilot) {
  return new WorkerPilot(args.level, args.nextLevel);
}
