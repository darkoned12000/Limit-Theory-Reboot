#include "../Items.h"

#include "Game/Constants.h"
#include "Game/Icons.h"
#include "Game/Objects.h"
#include "Game/Attribute/Capability.h"
#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Mass.h"
#include "Game/Attribute/Metatype.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/PowerDrain.h"
#include "Game/Attribute/Scale.h"
#include "Game/Attribute/Value.h"
#include "LTE/FunctionBind.h"

using ProductionLabTypeBase = 
    Attribute_Capability
  < Attribute_Icon
  < Attribute_Mass
  < Attribute_Metatype
  < Attribute_Name
  < Attribute_PowerDrain
  < Attribute_Scale
  < Attribute_Value
  < ItemWrapper<ItemType_ProductionLabType>
  > > > > > > > >;

AutoClassDerivedEmpty(ProductionLabType, ProductionLabTypeBase)
  DERIVED_TYPE_EX(ProductionLabType)

  SocketType GetSocketType() const override {
    return SocketType_Interior;
  }

  Object Instantiate(ObjectT* parent) override {
    return Object_ProductionLab(this);
  }
};

DERIVED_IMPLEMENT(ProductionLabType)

Item Item_ProductionLabType(Item_ProductionLabType_Args const& args) {
  Mass mass = Constant_ValueToMass(args.value, args.compactness);
  float powerDrain = Constant_ValueToPowerDrain(args.value, args.efficiency);
  float rate = Constant_ValueToOutput(args.value, args.rate);

  Reference<ProductionLabType> self = new ProductionLabType;
  self->capability = Capability_Production(rate);
  self->icon = Icon_Task_Produce();
  self->mass = mass;
  self->metatype = Item_ProductionLabType_Args(args);
  self->name = "Production Lab";
  self->powerDrain = powerDrain;
  self->scale = Constant_MassToScale(mass);
  self->value = args.value;
  return self;
}
static Function const Item_ProductionLabType_Registration = Function_Bind(
  "Item_ProductionLabType",
  "None",
  [](double const& value, uint const& seed, float const& compactness, float const& efficiency, float const& rate) -> Item { return Item_ProductionLabType(value, seed, compactness, efficiency, rate); },
  "value", "seed", "compactness", "efficiency", "rate");


