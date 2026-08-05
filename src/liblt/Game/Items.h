#ifndef Game_Items_h__
#define Game_Items_h__

#include "ItemWrapper.h"
#include "Capability.h"
#include "Task.h"


#include "UI/Icon.h"
#include "LTE/AutoClass.h"

/* Attributes -
 * value - Overall, how 'good' is the item?
 * seed - Number for randomizing other generated parameters.
 *
 * capacity - How much internal mass can the item hold?
 * compactness - What's the mass of the item?
 * damage - How much damage does the item inflict?
 * efficiency - How much power does the item draw?
 * integrity - How much health does the item have?
 * rate - How quickly does the item perform it's task?
 * speed - How fast is the item? */

AutoClass(Item_AssemblyChip_Args,
  Item, blueprint,
  Item, source)
  Item_AssemblyChip_Args() {}
};

LT_API Item Item_AssemblyChip(Item_AssemblyChip_Args const& args);
inline Item Item_AssemblyChip(
  Item const& blueprint, Item const& source) {
  return Item_AssemblyChip(Item_AssemblyChip_Args(blueprint, source));
}

LT_API Item Item_Blueprint(DataRef const& properties);

LT_API Item Item_Blueprint_Derived(Reference<Blueprint> const& source);

LT_API Item Item_ColonyType(
  String const& name, Icon const& icon, Task const& task, Traits const& traits);

LT_API Item Item_Commodity(int id);

LT_API Item Item_Data_Damaged(Object const& object);

LT_API Item Item_Data_Destroyed(Object const& object);

AutoClass(Item_DroneBayType_Args,
  double, value,
  uint, seed,
  float, compactness,
  float, speed)
  Item_DroneBayType_Args() {}
};

LT_API Item Item_DroneBayType(Item_DroneBayType_Args const& args);
inline Item Item_DroneBayType(
  double const& value, uint const& seed, float const& compactness, float const& speed) {
  return Item_DroneBayType(Item_DroneBayType_Args(value, seed, compactness, speed));
}

inline Item Item_DroneBayType(double value, uint seed) {
  return Item_DroneBayType(value, seed, 1, 1);
}

using Meta_DroneBayType = Item_DroneBayType_Args;

AutoClass(Item_DroneConstructionType_Args,
  double, value,
  uint, seed)
  Item_DroneConstructionType_Args() {}
};

LT_API Item Item_DroneConstructionType(Item_DroneConstructionType_Args const& args);
inline Item Item_DroneConstructionType(
  double const& value, uint const& seed) {
  return Item_DroneConstructionType(Item_DroneConstructionType_Args(value, seed));
}

AutoClass(Item_DroneProspectingType_Args,
  double, value,
  uint, seed)
  Item_DroneProspectingType_Args() {}
};

LT_API Item Item_DroneProspectingType(Item_DroneProspectingType_Args const& args);
inline Item Item_DroneProspectingType(
  double const& value, uint const& seed) {
  return Item_DroneProspectingType(Item_DroneProspectingType_Args(value, seed));
}

LT_API Item Item_PlanetType(
  uint const& seed);

AutoClass(Item_ProductionLabType_Args,
  double, value,
  uint, seed,
  float, compactness,
  float, efficiency,
  float, rate)
  Item_ProductionLabType_Args() {}
};

LT_API Item Item_ProductionLabType(Item_ProductionLabType_Args const& args);
inline Item Item_ProductionLabType(
  double const& value, uint const& seed, float const& compactness, float const& efficiency,
  float const& rate) {
  return Item_ProductionLabType(Item_ProductionLabType_Args(value, seed, compactness, efficiency, rate));
}

inline Item Item_ProductionLabType(double value, uint seed) {
  return Item_ProductionLabType(value, seed, 1, 1, 1);
}

using Meta_ProductionLabType = Item_ProductionLabType_Args;

#if 0
DeclareFunction(Item_PulseType, Item,
  double, value,
  uint, seed,

  float, compactness,
  float, damage,
  float, efficiency,
  float, integrity,
  float, rate,
  float, speed)

using Meta_PulseType = Item_PulseType_Args;
#endif

AutoClass(Item_OreType_Args,
  double, value,
  uint, seed)
  Item_OreType_Args() {}
};

LT_API Item Item_OreType(Item_OreType_Args const& args);
inline Item Item_OreType(
  double const& value, uint const& seed) {
  return Item_OreType(Item_OreType_Args(value, seed));
}

using Meta_OreType = Item_OreType_Args;

AutoClass(Item_PowerGeneratorType_Args,
  double, value,
  uint, seed)
  Item_PowerGeneratorType_Args() {}
};

LT_API Item Item_PowerGeneratorType(Item_PowerGeneratorType_Args const& args);
inline Item Item_PowerGeneratorType(
  double const& value, uint const& seed) {
  return Item_PowerGeneratorType(Item_PowerGeneratorType_Args(value, seed));
}

using Meta_PowerGeneratorType = Item_PowerGeneratorType_Args;

AutoClass(Item_ScannerType_Args,
  double, value,
  uint, seed,
  float, range)
  Item_ScannerType_Args() {}
};

LT_API Item Item_ScannerType(Item_ScannerType_Args const& args);
inline Item Item_ScannerType(
  double const& value, uint const& seed, float const& range) {
  return Item_ScannerType(Item_ScannerType_Args(value, seed, range));
}

inline Item Item_ScannerType(double value, uint seed) {
  return Item_ScannerType(value, seed, 1);
}

using Meta_ScannerType = Item_ScannerType_Args;

AutoClass(Item_ShieldType_Args,
  double, value,
  uint, seed,
  float, compactness,
  float, efficiency,
  float, integrity,
  float, rate)
  Item_ShieldType_Args() {}
};

LT_API Item Item_ShieldType(Item_ShieldType_Args const& args);
inline Item Item_ShieldType(
  double const& value, uint const& seed, float const& compactness, float const& efficiency,
  float const& integrity, float const& rate) {
  return Item_ShieldType(Item_ShieldType_Args(value, seed, compactness, efficiency, integrity, rate));
}

inline Item Item_ShieldType(double value, uint seed) {
  return Item_ShieldType(value, seed, 1, 1, 1, 1);
}

using Meta_ShieldType = Item_ShieldType_Args;

AutoClass(Item_ShipType_Args,
  double, value,
  uint, seed,
  float, capacity,
  float, compactness,
  float, integrity,
  float, propulsion,
  float, systems,
  float, turrets)
  Item_ShipType_Args() {}
};

LT_API Item Item_ShipType(Item_ShipType_Args const& args);
inline Item Item_ShipType(
  double const& value, uint const& seed, float const& capacity, float const& compactness,
  float const& integrity, float const& propulsion, float const& systems,
  float const& turrets) {
  return Item_ShipType(Item_ShipType_Args(value, seed, capacity, compactness, integrity, propulsion, systems, turrets));
}

inline Item Item_ShipType(double value, uint seed) {
  return Item_ShipType(value, seed, 1, 1, 1, 1, 1, 1);
}

using Meta_ShipType = Item_ShipType_Args;

AutoClass(Item_StationType_Args,
  double, value,
  uint, seed,
  float, capacity,
  float, integrity,
  float, systems,
  float, turrets)
  Item_StationType_Args() {}
};

LT_API Item Item_StationType(Item_StationType_Args const& args);
inline Item Item_StationType(
  double const& value, uint const& seed, float const& capacity, float const& integrity,
  float const& systems, float const& turrets) {
  return Item_StationType(Item_StationType_Args(value, seed, capacity, integrity, systems, turrets));
}

inline Item Item_StationType(double value, uint seed) {
  return Item_StationType(value, seed, 1, 1, 1, 1);
}

AutoClass(Item_TechLabType_Args,
  double, value,
  uint, seed,
  float, compactness,
  float, efficiency,
  float, rate)
  Item_TechLabType_Args() {}
};

LT_API Item Item_TechLabType(Item_TechLabType_Args const& args);
inline Item Item_TechLabType(
  double const& value, uint const& seed, float const& compactness, float const& efficiency,
  float const& rate) {
  return Item_TechLabType(Item_TechLabType_Args(value, seed, compactness, efficiency, rate));
}

inline Item Item_TechLabType(double value, uint seed) {
  return Item_TechLabType(value, seed, 1, 1, 1);
}

using Meta_TechLabType = Item_TechLabType_Args;

AutoClass(Item_ThrusterType_Args,
  double, value,
  uint, seed,
  float, compactness,
  float, efficiency,
  float, integrity,
  float, rate)
  Item_ThrusterType_Args() {}
};

LT_API Item Item_ThrusterType(Item_ThrusterType_Args const& args);
inline Item Item_ThrusterType(
  double const& value, uint const& seed, float const& compactness, float const& efficiency,
  float const& integrity, float const& rate) {
  return Item_ThrusterType(Item_ThrusterType_Args(value, seed, compactness, efficiency, integrity, rate));
}

inline Item Item_ThrusterType(double value, uint seed) {
  return Item_ThrusterType(value, seed, 1, 1, 1, 1);
}

using Meta_ThrusterType = Item_ThrusterType_Args;

AutoClass(Item_TransferUnitType_Args,
  double, value,
  uint, seed,
  float, compactness,
  float, efficiency,
  float, range,
  float, rate)
  Item_TransferUnitType_Args() {}
};

LT_API Item Item_TransferUnitType(Item_TransferUnitType_Args const& args);
inline Item Item_TransferUnitType(
  double const& value, uint const& seed, float const& compactness, float const& efficiency,
  float const& range, float const& rate) {
  return Item_TransferUnitType(Item_TransferUnitType_Args(value, seed, compactness, efficiency, range, rate));
}

using Meta_TransferUnitType = Item_TransferUnitType_Args;

AutoClass(Item_TurretType_Args,
  uint, sockets,
  float, trackingSpeed)
  Item_TurretType_Args() {}
};

LT_API Item Item_TurretType(Item_TurretType_Args const& args);
inline Item Item_TurretType(
  uint const& sockets, float const& trackingSpeed) {
  return Item_TurretType(Item_TurretType_Args(sockets, trackingSpeed));
}

using Meta_TurretType = Item_TurretType_Args;

LT_API Item Item_WeaponType(
  int const& id);

AutoClass(Item_Worker_Engineer_Args,
  uint, level,
  Item, nextLevel)
  Item_Worker_Engineer_Args() {}
};

LT_API Item Item_Worker_Engineer(Item_Worker_Engineer_Args const& args);
inline Item Item_Worker_Engineer(
  uint const& level, Item const& nextLevel) {
  return Item_Worker_Engineer(Item_Worker_Engineer_Args(level, nextLevel));
}

AutoClass(Item_Worker_Miner_Args,
  uint, level,
  Item, nextLevel)
  Item_Worker_Miner_Args() {}
};

LT_API Item Item_Worker_Miner(Item_Worker_Miner_Args const& args);
inline Item Item_Worker_Miner(
  uint const& level, Item const& nextLevel) {
  return Item_Worker_Miner(Item_Worker_Miner_Args(level, nextLevel));
}

AutoClass(Item_Worker_Pilot_Args,
  uint, level,
  Item, nextLevel)
  Item_Worker_Pilot_Args() {}
};

LT_API Item Item_Worker_Pilot(Item_Worker_Pilot_Args const& args);
inline Item Item_Worker_Pilot(
  uint const& level, Item const& nextLevel) {
  return Item_Worker_Pilot(Item_Worker_Pilot_Args(level, nextLevel));
}

#endif
