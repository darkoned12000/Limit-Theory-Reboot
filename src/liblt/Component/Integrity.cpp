#include "Integrity.h"

#include "Game/Items.h"
#include "Game/Object.h"

#include "LTE/Mutable.h"
#include "LTE/FunctionBind.h"

Damage ComponentIntegrity::ApplyDamage(ObjectT* self, Damage damage) {
  Health thisDamage = Min(damage, health);
  health -= thisDamage;
  if (health <= 0)
    self->OnDeath();
  return damage - thisDamage;
}

ItemT* ComponentIntegrity::GetDataDamaged(ObjectT const* self) const {
  if (!dataDamaged)
    Mutable(this)->dataDamaged = Item_Data_Damaged(Mutable(self));
  return dataDamaged;
}

ItemT* ComponentIntegrity::GetDataDestroyed(ObjectT const* self) const {
  if (!dataDestroyed)
    Mutable(this)->dataDestroyed = Item_Data_Destroyed(Mutable(self));
  return dataDestroyed;
}

static Function const Object_GetHealth_Registration = Function_Bind(
  "Object_GetHealth",
  "Return 'object's current (non-normalized) health",
  [](Object const& object) -> Health
  {
  return object->GetHealth();
  },
  "object");
static int const Object_GetHealth_Alias = Function_Alias("Object_GetHealth", "GetHealth");

static Function const Object_GetMaxHealth_Registration = Function_Bind(
  "Object_GetMaxHealth",
  "Return 'object's maximal health value",
  [](Object const& object) -> Health
  {
  return object->GetMaxHealth();
  },
  "object");
static int const Object_GetMaxHealth_Alias = Function_Alias("Object_GetMaxHealth", "GetMaxHealth");

static Function const Object_GetHealthNormalized_Registration = Function_Bind(
  "Object_GetHealthNormalized",
  "Return 'object's current health on a scale from 0.0 (dead) to 1.0 (max health)",
  [](Object const& object) -> float
  {
  return object->GetHealthNormalized();
  },
  "object");
static int const Object_GetHealthNormalized_Alias = Function_Alias("Object_GetHealthNormalized", "GetHealthNormalized");

static Function const Object_SetHealth_Registration = Function_Bind(
  "Object_SetHealth",
  "Set 'object's current health to 'health'",
  [](Object const& object, Health const& health)
  {
  if (object->GetIntegrity())
    object->GetIntegrity()->health = health;
  },
  "object", "health");
static int const Object_SetHealth_Alias = Function_Alias("Object_SetHealth", "SetHealth");
