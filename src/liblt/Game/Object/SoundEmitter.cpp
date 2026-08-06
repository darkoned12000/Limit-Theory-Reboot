#include "../Objects.h"

#include "Component/Drawable.h"
#include "Component/Motion.h"
#include "Component/Orientation.h"

#include "LTE/Pool.h"

#include "Module/SoundEngine.h"
#include "LTE/FunctionBind.h"

using SoundEmitterBaseT = ObjectWrapper
  < Component_Drawable
  < Component_Motion
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_SoundEmitter>
  > > > >;

AutoClassDerived(SoundEmitter, SoundEmitterBaseT,
  float, life)
  Sound sound;

  DERIVED_TYPE_EX(SoundEmitter)
  POOLED_TYPE

  SoundEmitter() :
    life(0)
    {}

  void OnUpdate(UpdateState& state) override {
    BaseType::OnUpdate(state);

    life -= state.dt;
    if (life <= 0) {
      Delete();
      return;
    }
  }
};

DERIVED_IMPLEMENT(SoundEmitter)

Object Object_SoundEmitter(String const& filename, Position const& position, float const& volume, float const& distanceDiv) {
  Reference<SoundEmitter> self = new SoundEmitter;
  self->SetPos(position);
  self->sound = Sound_Play3D(filename, self, 0, volume, distanceDiv, false);
  self->life = self->sound->GetDuration() / 1000.0f;
  return self;
}
static Function const Object_SoundEmitter_Registration = Function_Bind(
  "Object_SoundEmitter",
  "None",
  &Object_SoundEmitter,
  "filename", "position", "volume", "distanceDiv");


