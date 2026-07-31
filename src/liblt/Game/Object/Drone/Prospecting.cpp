#include "../../Objects.h"
#include "../../Items.h"

#include "Component/Drawable.h"
#include "Component/Mineable.h"
#include "Component/Orientation.h"

#include "Game/Player.h"
#include "Game/Settings.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Value.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"

using DroneProspectingBaseT = ObjectWrapper
  < Component_Drawable
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_Drone>
  > > >;

AutoClassDerivedEmpty(DroneProspecting, DroneProspectingBaseT)
  DERIVED_TYPE_EX(DroneProspecting)
  POOLED_TYPE
  
  DroneProspecting() = default;

  DroneProspecting(ObjectT* parent) {
    parent->AddChild(this);
    SetScale(2);
  }

  void OnUpdate(UpdateState& state) override {}
};

DERIVED_IMPLEMENT(DroneProspecting)

using DroneProspectingTypeBaseT = 
    Attribute_Name
  < Attribute_Value
  < ItemWrapper<ItemType_DroneType>
  > >;

AutoClassDerivedEmpty(DroneProspectingType, DroneProspectingTypeBaseT)
  DERIVED_TYPE_EX(DroneProspectingType)
  POOLED_TYPE

  Object Instantiate(ObjectT* parent) override {
    return new DroneProspecting(parent);
  }
};

DefineFunction(Item_DroneProspectingType) {
  return new DroneProspectingType;
}

DERIVED_IMPLEMENT(DroneProspectingType)
