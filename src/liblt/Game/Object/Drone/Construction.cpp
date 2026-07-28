#include "../../Objects.h"
#include "../../Items.h"

#include "Component/Drawable.h"
#include "Component/Orientation.h"

#include "Game/Player.h"
#include "Game/Renderables.h"
#include "Game/Settings.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Value.h"

using DroneConstructionBaseT = ObjectWrapper
  < Component_Drawable
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_Drone>
  > > >;

AutoClassDerivedEmpty(DroneConstruction, DroneConstructionBaseT)
  DERIVED_TYPE_EX(DroneConstruction)
  
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

  Object Instantiate(ObjectT* parent) override {
    return new DroneConstruction;
  }
};

DefineFunction(Item_DroneConstructionType) {
  return new DroneConstructionType;
}

DERIVED_IMPLEMENT(DroneConstructionType)
