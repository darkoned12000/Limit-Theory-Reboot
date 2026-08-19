#include "../Objects.h"

#include "Component/Drawable.h"
#include "Component/Orientation.h"

#include "Game/Light.h"
#include "Game/Messages.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/RNG.h"
#include "LTE/FunctionBind.h"

using StarBaseT = ObjectWrapper
  < Component_Drawable
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_Star>
  > > >;

AutoClassDerived(Star, StarBaseT,
  LightRef, light,
  Color, color,
  float, lightBrightness,
  float, lightRadius,
  float, baseBrightness,
  float, pulseSpeed,
  float, pulseAmplitude,
  float, age)

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

    age += state.dt;
    lightBrightness = baseBrightness + baseBrightness * pulseAmplitude * Sin(age * pulseSpeed);

    if (!light)
      light = Light_Create(this);
    light->color = lightBrightness * color;
    light->radius = lightRadius;
  }
};

DERIVED_IMPLEMENT(Star)

Object Object_Star(Color const& color, float const& brightness,
                   float const& radius, float const& pulseSpeed,
                   float const& pulseAmplitude) {
  Reference<Star> self = new Star;
  self->color = color;
  self->lightBrightness = brightness;
  self->lightRadius = radius;
  self->baseBrightness = brightness;
  self->pulseSpeed = pulseSpeed;
  self->pulseAmplitude = pulseAmplitude;
  self->age = 0.0f;
  return self;
}
static Function const Object_Star_Registration = Function_Bind(
  "Object_Star",
  "None",
  &Object_Star,
  "color", "brightness", "radius", "pulseSpeed", "pulseAmplitude");


