#include "../Items.h"

#include "Game/Constants.h"
#include "Game/Icons.h"
#include "Game/Objects.h"
#include "Game/Attribute/Capability.h"
#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Mass.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/PowerDrain.h"
#include "Game/Attribute/Scale.h"
#include "Game/Attribute/Value.h"

#include "UI/Glyphs.h"
#include "LTE/FunctionBind.h"

using TechLabTypeBase = 
    Attribute_Capability
  < Attribute_Icon
  < Attribute_Mass
  < Attribute_Name
  < Attribute_PowerDrain
  < Attribute_Scale
  < Attribute_Value
  < ItemWrapper<ItemType_TechLabType>
  > > > > > > >;

AutoClassDerivedEmpty(TechLabType, TechLabTypeBase)
  DERIVED_TYPE_EX(TechLabType)

  SocketType GetSocketType() const override {
    return SocketType_Interior;
  }

  Object Instantiate(ObjectT* parent) override {
    return Object_TechLab(this);
  }
};

DERIVED_IMPLEMENT(TechLabType)

Item Item_TechLabType(Item_TechLabType_Args const& args) {
  Mass mass = Constant_ValueToMass(args.value, args.compactness);
  float powerDrain = Constant_ValueToPowerDrain(args.value, args.efficiency);
  float rate = Constant_ValueToOutput(args.value, args.rate);

  Reference<TechLabType> self = new TechLabType;
  self->capability = Capability_Research(rate);
  self->icon = Icon_Task_Research();
  self->mass = mass;
  self->name = "Tech Lab";
  self->powerDrain = powerDrain;
  self->scale = Constant_MassToScale(mass);
  self->value = args.value;
  return self;
}
static Function const Item_TechLabType_Registration = Function_Bind(
  "Item_TechLabType",
  "None",
  [](double const& value, uint const& seed, float const& compactness, float const& efficiency, float const& rate) -> Item { return Item_TechLabType(value, seed, compactness, efficiency, rate); },
  "value", "seed", "compactness", "efficiency", "rate");


