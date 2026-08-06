#include "Collidable.h"
#include "Motion.h"

#include "Game/Object.h"

#include "LTE/Debug.h"
#include "LTE/Math.h"
#include "LTE/StackFrame.h"
#include "LTE/FunctionBind.h"

void ComponentMotion::Run(ObjectT* self, UpdateState& state) { AUTO_FRAME;
  LTE_ASSERT(force.IsFinite());
  LTE_ASSERT(torque.IsFinite());

  /* Notify the collidable component that we're not a passive object, since
     we've got the ability to move. */
  ComponentCollidable* c = self->GetCollidable();
  if (c)
    c->passive = false;

  /* Apply drag. */
  force -= velocity * (mass * kLinearDrag);
  torque -= velocityA * (inertia * kAngularDrag);

  velocity += force * (state.dt / static_cast<float>(mass));

  /* Note that this should actually be inertial tensor, not mass. But
   * for now this will do as an approximation. */
  velocityA += torque * (state.dt / static_cast<float>(inertia));

  const float tolerance = 0;

  ComponentOrientation* ori = self->GetOrientation();
  if (Abs(velocity).GetMax() > tolerance)
    ori->GetTransformW().pos += (Position)velocity * state.dt;
  
  if (Abs(velocityA).GetMax() > tolerance)
    ori->Rotate(velocityA * state.dt);

  force = 0;
  torque = 0;
  speed = Length(velocity);

  LTE_ASSERT(velocity.IsFinite());
  LTE_ASSERT(velocityA.IsFinite());
}

static Function const Object_ApplyForce_Registration = Function_Bind(
  "Object_ApplyForce",
  "Apply 'force' to 'object'",
  [](Object const& object, V3 const& force)
  {
  return object->ApplyForce(force);
  },
  "object", "force");
static int const Object_ApplyForce_Alias = Function_Alias("Object_ApplyForce", "ApplyForce");

static Function const Object_GetMass_Registration = Function_Bind(
  "Object_GetMass",
  "Return the mass of 'object'",
  [](Object const& object) -> Mass
  {
  return object->GetMass();
  },
  "object");
static int const Object_GetMass_Alias = Function_Alias("Object_GetMass", "GetMass");

static Function const Object_GetSpeed_Registration = Function_Bind(
  "Object_GetSpeed",
  "Return the current speed of 'object'",
  [](Object const& object) -> float
  {
  return object->GetSpeed();
  },
  "object");
static int const Object_GetSpeed_Alias = Function_Alias("Object_GetSpeed", "GetSpeed");

static Function const Object_GetTopSpeed_Registration = Function_Bind(
  "Object_GetTopSpeed",
  "Return the top speed of 'object' under normal propulsion",
  [](Object const& object) -> float
  {
  return object->GetTopSpeed();
  },
  "object");
static int const Object_GetTopSpeed_Alias = Function_Alias("Object_GetTopSpeed", "GetTopSpeed");

static Function const Object_GetVelocity_Registration = Function_Bind(
  "Object_GetVelocity",
  "Return the current velocity of 'object'",
  [](Object const& object) -> V3
  {
  return object->GetVelocity();
  },
  "object");
static int const Object_GetVelocity_Alias = Function_Alias("Object_GetVelocity", "GetVelocity");

static Function const Object_GetVelocityAngular_Registration = Function_Bind(
  "Object_GetVelocityAngular",
  "Return the current angular velocity of 'object'",
  [](Object const& object) -> V3
  {
  return object->GetVelocityA();
  },
  "object");
static int const Object_GetVelocityAngular_Alias = Function_Alias("Object_GetVelocityAngular", "GetVelocityAngular");

static Function const Object_SetMass_Registration = Function_Bind(
  "Object_SetMass",
  "Set the mass of 'object' to 'mass'",
  [](Object const& object, Mass const& mass)
  {
  if (object->GetMotion())
    object->GetMotion()->mass = mass;
  },
  "object", "mass");
static int const Object_SetMass_Alias = Function_Alias("Object_SetMass", "SetMass");

AutoClass(Impact,
  float, t,
  Position, position)
  Impact() = default;
};

static Function const GetImpact_Registration = Function_Bind(
  "GetImpact",
  "Compute the time and location of impact for a munition with 'speed' going from 'source' to 'dest'",
  [](Position const& sourcePos, Position const& destPos, V3 const& sourceVelocity, V3 const& destVelocity, float const& speed) -> Impact
  {
  Position hitPoint = {};
  float t = ComputeImpact(sourcePos, destPos, sourceVelocity, destVelocity, speed, &hitPoint);
  return Impact(t, hitPoint);
  },
  "sourcePos", "destPos", "sourceVelocity", "destVelocity", "speed");
