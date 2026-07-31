#ifndef Game_Items_h__
#define Game_Items_h__

#include "ItemWrapper.h"
#include "Capability.h"
#include "Task.h"

#include "LTE/DeclareFunction.h"

#include "UI/Icon.h"

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

DeclareFunctionArgBind(Item_AssemblyChip, Item,
  Item, blueprint,
  Item, source)

LT_API Item Item_Blueprint(DataRef const& properties);

LT_API Item Item_Blueprint_Derived(Reference<Blueprint> const& source);

DeclareFunction(Item_ColonyType, Item,
  String, name,
  Icon, icon,
  Task, task,
  Traits, traits)

LT_API Item Item_Commodity(int id);

LT_API Item Item_Data_Damaged(Object const& object);

LT_API Item Item_Data_Destroyed(Object const& object);

DeclareFunctionArgBind(Item_DroneBayType, Item,
  double, value,
  uint, seed,

  float, compactness,
  float, speed)

inline Item Item_DroneBayType(double value, uint seed) {
  return Item_DroneBayType(value, seed, 1, 1);
}

using Meta_DroneBayType = Item_DroneBayType_Args;

DeclareFunctionArgBind(Item_DroneConstructionType, Item,
  double, value,
  uint, seed)

DeclareFunctionArgBind(Item_DroneProspectingType, Item,
  double, value,
  uint, seed)

DeclareFunction(Item_PlanetType,
  Item,
  uint, seed)

DeclareFunctionArgBind(Item_ProductionLabType, Item,
  double, value,
  uint, seed,

  float, compactness,
  float, efficiency,
  float, rate)

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

DeclareFunctionArgBind(Item_OreType, Item,
  double, value,
  uint, seed)

using Meta_OreType = Item_OreType_Args;

DeclareFunctionArgBind(Item_PowerGeneratorType, Item,
  double, value,
  uint, seed)

using Meta_PowerGeneratorType = Item_PowerGeneratorType_Args;

DeclareFunctionArgBind(Item_ScannerType, Item,
  double, value,
  uint, seed,

  float, range)

inline Item Item_ScannerType(double value, uint seed) {
  return Item_ScannerType(value, seed, 1);
}

using Meta_ScannerType = Item_ScannerType_Args;

DeclareFunctionArgBind(Item_ShieldType, Item,
  double, value,
  uint, seed,

  float, compactness,
  float, efficiency,
  float, integrity,
  float, rate)

inline Item Item_ShieldType(double value, uint seed) {
  return Item_ShieldType(value, seed, 1, 1, 1, 1);
}

using Meta_ShieldType = Item_ShieldType_Args;

DeclareFunctionArgBind(Item_ShipType, Item,
  double, value,
  uint, seed,

  float, capacity,
  float, compactness,
  float, integrity,
  float, propulsion,
  float, systems,
  float, turrets)

inline Item Item_ShipType(double value, uint seed) {
  return Item_ShipType(value, seed, 1, 1, 1, 1, 1, 1);
}

using Meta_ShipType = Item_ShipType_Args;

DeclareFunctionArgBind(Item_StationType, Item,
  double, value,
  uint, seed,

  float, capacity,
  float, integrity,
  float, systems,
  float, turrets)

inline Item Item_StationType(double value, uint seed) {
  return Item_StationType(value, seed, 1, 1, 1, 1);
}

DeclareFunctionArgBind(Item_TechLabType, Item,
  double, value,
  uint, seed,

  float, compactness,
  float, efficiency,
  float, rate)

inline Item Item_TechLabType(double value, uint seed) {
  return Item_TechLabType(value, seed, 1, 1, 1);
}

using Meta_TechLabType = Item_TechLabType_Args;

DeclareFunctionArgBind(Item_ThrusterType, Item,
  double, value,
  uint, seed,

  float, compactness,
  float, efficiency,
  float, integrity,
  float, rate)

inline Item Item_ThrusterType(double value, uint seed) {
  return Item_ThrusterType(value, seed, 1, 1, 1, 1);
}

using Meta_ThrusterType = Item_ThrusterType_Args;

DeclareFunctionArgBind(Item_TransferUnitType, Item,
  double, value,
  uint, seed,

  float, compactness,
  float, efficiency,
  float, range,
  float, rate)

using Meta_TransferUnitType = Item_TransferUnitType_Args;

DeclareFunctionArgBind(Item_TurretType, Item,
  uint, sockets,
  float, trackingSpeed)

using Meta_TurretType = Item_TurretType_Args;

DeclareFunction(Item_WeaponType, Item, int, id)

DeclareFunctionArgBind(Item_Worker_Engineer, Item,
  uint, level,
  Item, nextLevel)

DeclareFunctionArgBind(Item_Worker_Miner, Item,
  uint, level,
  Item, nextLevel)

DeclareFunctionArgBind(Item_Worker_Pilot, Item,
  uint, level,
  Item, nextLevel)

#endif
