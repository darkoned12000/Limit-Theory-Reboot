#include "../Objects.h"

#include "Component/BoundingBox.h"
#include "Component/Collidable.h"
#include "Component/Cullable.h"
#include "Component/Detectable.h"
#include "Component/Drawable.h"
#include "Component/Mineable.h"
#include "Component/Orientation.h"
#include "Component/Seeded.h"

#include "Game/Renderables.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/RNG.h"
#include "LTE/ShaderInstance.h"
#include "LTE/FunctionBind.h"

using AsteroidBaseT = ObjectWrapper
  < Component_BoundingBox
  < Component_Collidable
  < Component_Cullable
  < Component_Drawable
  < Component_Orientation
  < Component_Seeded
  < ObjectWrapperTail<ObjectType_Asteroid>
  > > > > > > >;

AutoClassDerivedEmpty(Asteroid, AsteroidBaseT)
  DERIVED_TYPE_EX(Asteroid)
  POOLED_TYPE

  void Initialize() {
    Drawable.renderable =
      // Renderable_Ice(Seeded.seed);
      Renderable_Asteroid(Seeded.seed);
  }
};

DERIVED_IMPLEMENT(Asteroid)

using AsteroidRichBaseT = ObjectWrapper
  < Component_BoundingBox
  < Component_Collidable
  < Component_Cullable
  < Component_Detectable
  < Component_Drawable
  < Component_Mineable
  < Component_Orientation
  < Component_Seeded
  < ObjectWrapperTail<ObjectType_AsteroidRich>
  > > > > > > > > >;

AutoClassDerivedEmpty(AsteroidRich, AsteroidRichBaseT)
  DERIVED_TYPE_EX(AsteroidRich)
  POOLED_TYPE

  void Initialize() {
    Drawable.renderable =
      //Renderable_Ice(Seeded.seed);
      Renderable_Asteroid(Seeded.seed);
  }

  Signature GetSignature() const override {
    return Signature(1.0f, 6.0f, 0.5f, 2.0f);
  }
};

DERIVED_IMPLEMENT(AsteroidRich)

Object Object_Asteroid(uint const& seed) {
  Reference<Asteroid> self = new Asteroid;
  self->Seeded.seed = seed;
  self->Initialize();
  return self;
}
static Function const Object_Asteroid_Registration = Function_Bind(
  "Object_Asteroid",
  "None",
  &Object_Asteroid,
  "seed");



Object Object_AsteroidRich(uint const& seed, Item const& resource, Quantity const& quantity) {
  Reference<AsteroidRich> self = new AsteroidRich;

  /* TODO : Factor generating algorithm. */
  RNG rg = RNG_MTG(seed);
  self->Mineable.item = resource;
  self->Mineable.quantity = quantity;
  self->Mineable.phase = rg->GetDirection();
  self->Seeded.seed = seed;
  self->Initialize();
  return self;
}
static Function const Object_AsteroidRich_Registration = Function_Bind(
  "Object_AsteroidRich",
  "None",
  &Object_AsteroidRich,
  "seed", "resource", "quantity");


