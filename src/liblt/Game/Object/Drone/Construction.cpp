#include "../../Objects.h"
#include "../../Items.h"

#include "Component/Drawable.h"
#include "Component/Orientation.h"

#include "Game/Player.h"
#include "Game/Renderables.h"
#include "Game/Settings.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Value.h"

#include "LTE/Pool.h"
#include "LTE/FunctionBind.h"

using DroneConstructionBaseT = ObjectWrapper
  < Component_Drawable
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_Drone>
  > > >;

AutoClassDerivedEmpty(DroneConstruction, DroneConstructionBaseT)
  DERIVED_TYPE_EX(DroneConstruction)
  POOLED_TYPE
  
  DroneConstruction() {
    Drawable.renderable = Renderable_Ice(5);
    SetScale(10);
  }

  void OnUpdate(UpdateState& state) override {}
};

DERIVED_IMPLEMENT(DroneConstruction)

using DroneConstructionTypeBaseT = 
    Attribute_Name
  < Attribute_Value
  < ItemWrapper<ItemType_DroneType>
  > >;

AutoClassDerivedEmpty(DroneConstructionType, DroneConstructionTypeBaseT)
  DERIVED_TYPE_EX(DroneConstructionType)
  POOLED_TYPE

  Object Instantiate(ObjectT* parent) override {
    return new DroneConstruction;
  }
};

Item Item_DroneConstructionType(Item_DroneConstructionType_Args const& args) {
  return new DroneConstructionType;
}
static Function const Item_DroneConstructionType_Registration = Function_Bind(
  "Item_DroneConstructionType",
  "None",
  [](double const& value, uint const& seed) -> Item { return Item_DroneConstructionType(value, seed); },
  "value", "seed");



DERIVED_IMPLEMENT(DroneConstructionType)
