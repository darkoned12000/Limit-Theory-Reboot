#include "../Objects.h"

#include "Component/Orientation.h"
#include "Component/Pluggable.h"
#include "Component/Supertyped.h"
#include "Component/Tasks.h"

#include "Game/Player.h"
#include "Game/Item/AssemblyChip.h"

#include "LTE/Pool.h"

#include "Module/SoundEngine.h"
#include "LTE/FunctionBind.h"

using TechLabBaseT = ObjectWrapper
  < Component_Orientation
  < Component_Pluggable
  < Component_Supertyped
  < Component_Tasks
  < ObjectWrapperTail<ObjectType_TechLab>
  > > > > >;

AutoClassDerivedEmpty(TechLab, TechLabBaseT)
  Sound sound;

  DERIVED_TYPE_EX(TechLab)
  POOLED_TYPE
  
  void OnUpdate(UpdateState& state) override {
    BaseType::OnUpdate(state);

    if (!sound)
      sound = Sound_Play3D("techlab/loop.ogg",
        GetRoot().t, 0, 0,
        0.1f * GetRoot()->GetScale().GetMax(), true);
    sound->SetVolume(GetCurrentTask() == nullptr ? 0.0f : 0.1f);
  }
};

DERIVED_IMPLEMENT(TechLab)

Object Object_TechLab(Item const& type) {
  Reference<TechLab> self = new TechLab;
  self->SetSupertype(type);
  return self;
}
static Function const Object_TechLab_Registration = Function_Bind(
  "Object_TechLab",
  "None",
  &Object_TechLab,
  "type");


