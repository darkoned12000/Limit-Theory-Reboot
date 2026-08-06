#include "../Objects.h"

#include "Component/Drawable.h"
#include "Component/Interior.h"
#include "Component/Motion.h"
#include "Component/Orientation.h"
#include "Component/Queryable.h"

#include "Game/Light.h"
#include "Game/Renderables.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"

#include "Module/SoundEngine.h"
#include "LTE/FunctionBind.h"

using PayloadBaseT = ObjectWrapper
  < Component_Drawable
  < Component_Motion
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_Payload>
  > > > >;

AutoClassDerived(Payload, PayloadBaseT,
  LightRef, light,
  Item, payload,
  Object, source,
  V3, thrust)

  DERIVED_TYPE_EX(Payload)
  POOLED_TYPE
  
  Payload() = default;

  Payload(
      Item const& payload,
      Object const& source,
      V3 const& thrust) :
    payload(payload),
    source(source),
    thrust(thrust)
    {}

  void OnUpdate(UpdateState& state) override {
    BaseType::OnUpdate(state);

    Motion.force += thrust;

    if (!light)  {
      light = Light_Create(this);
      light->radius = 10.0f;
      light->color = Color(2.8f, 1.3f, 1.0f);
    }

    /* CRITICAL. */
    WorldRay r = WorldRay::FromPoints(GetPos(), GetPos());
    float t;
    V3 normal;
    ObjectT* collider = GetContainer()
      ->QueryInterior(r, t, 1, &normal, true, RaycastCanCollideBidirectional, this);

    if (collider) {
      Sound_Play3D("impact/metal/1.wav", nullptr, GetPos());
      Object p = payload->Instantiate(collider);
      p->SetPos(r(t));
      p->SetLook(normal);
      GetContainer()->AddInterior(p);
      Delete();
    }
  }

  bool CanCollide(ObjectT const* o) const override {
    return o->GetRoot() != source;
  }
};

DERIVED_IMPLEMENT(Payload)

Object Object_Payload(Object const& source, Item const& payload, Position const& position, V3 const& thrust, V3 const& velocity) {
  Reference<Payload> self = new Payload(payload, source, thrust);
  self->Orientation.GetTransformW().pos = position;
  self->Orientation.GetTransformW().look = Normalize(thrust);
  self->Motion.velocity = velocity;
  return self;
}
static Function const Object_Payload_Registration = Function_Bind(
  "Object_Payload",
  "None",
  &Object_Payload,
  "source", "payload", "position", "thrust", "velocity");


