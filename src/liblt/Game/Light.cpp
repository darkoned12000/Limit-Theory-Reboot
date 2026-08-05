#include "Light.h"
#include "Objects.h"

#include "LTE/Pool.h"
#include "LTE/FunctionBind.h"

AutoClassDerivedEmpty(LightImpl, Light)
  DERIVED_TYPE_EX(LightImpl)
  POOLED_TYPE
};

DERIVED_IMPLEMENT(LightImpl)

LightRef Light_Create(ObjectT* parent) {
  LightRef self = new LightImpl;
  parent->AddChild(self);
  return self;
}

Object Object_Light(Color const& color, float const& radius, bool const& lensFlare) {
  LightRef self = new LightImpl;
  self->color = color;
  self->radius = radius;
  self->flare = lensFlare;
  return self;
}
static Function const Object_Light_Registration = Function_Bind(
  "Object_Light",
  "None",
  &Object_Light,
  "color", "radius", "lensFlare");


