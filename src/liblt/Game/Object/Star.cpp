#include "../Objects.h"

#include "Component/Drawable.h"
#include "Component/Orientation.h"

#include "Game/Light.h"
#include "Game/Messages.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/RNG.h"

using StarBaseT = ObjectWrapper
  < Component_Drawable
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_Star>
  > > >;

AutoClassDerived(Star, StarBaseT,
  LightRef, light,
  Color, color)

  DERIVED_TYPE_EX(Star)
  POOLED_TYPE

  Star() = default;

  Signature GetSignature() const override {
    return Signature(1e5f, Mix(12, 16, Saturate(color.z)), 0.25f, 1);
  }

  void OnMessage(Data& m) override {
    BaseType::OnMessage(m);
    if (m.type == Type_Get<MessageGetColor>())
      m.Convert<MessageGetColor>().color = color;
  }

  void OnUpdate(UpdateState& state) override {
    BaseType::OnUpdate(state);

    if (!light)
      light = Light_Create(this);
    light->color = 10.0f * color;
    light->radius = 3000000;
  }
};

DERIVED_IMPLEMENT(Star)

DefineFunction(Object_Star) {
  Reference<Star> self = new Star;
  self->color = args.color;
  return self;
}
