#include "../Objects.h"

#include "Component/Orientation.h"
#include "Component/Pluggable.h"
#include "Component/Supertyped.h"
#include "Component/Tasks.h"

#include "LTE/Pool.h"

#include "Module/SoundEngine.h"
#include "LTE/FunctionBind.h"

using ProductionLabBaseT = ObjectWrapper
  < Component_Orientation
  < Component_Pluggable
  < Component_Supertyped
  < Component_Tasks
  < ObjectWrapperTail<ObjectType_ProductionLab>
  > > > > >;

AutoClassDerivedEmpty(ProductionLab, ProductionLabBaseT)
  Sound sound;

  DERIVED_TYPE_EX(ProductionLab)
  POOLED_TYPE

  void OnUpdate(UpdateState& state) override {
    BaseType::OnUpdate(state);

    if (!sound)
      sound = Sound_Play3D("productionlab/loop.ogg", GetRoot().t, 0, 0, 1, true);
    sound->SetVolume(GetCurrentTask() == nullptr ? 0.0f : 0.1f);
  }
};

DERIVED_IMPLEMENT(ProductionLab)

Object Object_ProductionLab(Item const& type) {
  Reference<ProductionLab> self = new ProductionLab;
  self->SetSupertype(type);
  return self;
}
static Function const Object_ProductionLab_Registration = Function_Bind(
  "Object_ProductionLab",
  "None",
  &Object_ProductionLab,
  "type");


