#ifndef Game_Objects_h__
#define Game_Objects_h__

#include "Object.h"
#include "ObjectWrapper.h"
#include "Item.h"

#include "LTE/Color.h"
#include "LTE/Distribution.h"
#include "LTE/SDF.h"
#include "LTE/AutoClass.h"

LT_API Object Object_Asteroid(
  uint const& seed);

LT_API Object Object_AsteroidRich(
  uint const& seed, Item const& resource, Quantity const& quantity);

LT_API Object Object_Colony(
  uint const& seed, Item const& type, Object const& planet, Quantity const& population);

LT_API Object Object_Custom(
  Data const& data);

LT_API Object Object_DustFlecks();

LT_API Object Object_Dynamic(Generic<Renderable, void> const& renderable);

LT_API Object Object_Explosion(
  ExplosionType const& type, float const& age, float const& duration);

LT_API Object Object_Light(
  Color const& color, float const& radius, bool const& lensFlare);

AutoClass(Object_Missile_Args,
  V3, thrust,
  V3, velocity,
  Object, target,
  V3, targetOffset)
  Object_Missile_Args() {}
};

LT_API Object Object_Missile(Object_Missile_Args const& args);
inline Object Object_Missile(
  V3 const& thrust, V3 const& velocity, Object const& target, V3 const& targetOffset) {
  return Object_Missile(Object_Missile_Args(thrust, velocity, target, targetOffset));
}

LT_API Object Object_Pod(
  Mass const& capacity);

LT_API Object Object_Payload(
  Object const& source, Item const& payload, Position const& position, V3 const& thrust,
  V3 const& velocity);

LT_API Object Object_Planet(
  Item const& type);

LT_API Object Object_PowerGenerator(
  Item const& type);

LT_API Object Object_ProductionLab(
  Item const& type);

LT_API Object Object_Pulse(
  V3 const& velocity,
  V3 const& drift,
  float width);

LT_API Object Object_Rail(
  Position const& origin,
  V3 const& direction,
  V3 const& velocity);

AutoClass(Object_Region_Args,
  int, level,
  Position, pos,
  float, radius,
  Distribution<Item>, resources,
  uint, seed)
  Object_Region_Args() {}
};

LT_API Object Object_Region(Object_Region_Args const& args);
inline Object Object_Region(
  int const& level, Position const& pos, float const& radius,
  Distribution<Item> const& resources, uint const& seed) {
  return Object_Region(Object_Region_Args(level, pos, radius, resources, seed));
}

using RegionType = Object_Region_Args;

LT_API Object Object_Scanner(
  Item const& type);

LT_API Object Object_Shield(
  Item const& type);

LT_API Object Object_Ship(
  Item const& type);

LT_API Object Object_SoundEmitter(
  String const& filename, Position const& position, float const& volume,
  float const& distanceDiv);

LT_API Object Object_Star(
  Color const& color,
  float const& brightness,
  float const& radius,
  float const& pulseSpeed,
  float const& pulseAmplitude);

LT_API Object Object_Station(
  Item const& type);

LT_API Object Object_Static(Generic<Renderable, void> const& renderable);

AutoClass(Object_System_Args,
  Position, position,
  uint, seed)
  Object_System_Args() {}
};

LT_API Object Object_System(Object_System_Args const& args);
inline Object Object_System(
  Position const& position, uint const& seed) {
  return Object_System(Object_System_Args(position, seed));
}

using SystemType = Object_System_Args;

LT_API Object Object_TechLab(
  Item const& type);

LT_API Object Object_Thruster(Item const& type, ObjectT* parent);

LT_API Object Object_Trail(
  Object const& parent,
  int length,
  Color const& color,
  float size);

LT_API Object Object_TransferUnit(
  Item const& type);

LT_API Object Object_Turret(
  Item const& type);

LT_API Object Object_Universe(
  uint const& seed, uint const& depth);

LT_API Object Object_WarpNode();

LT_API Object Object_WarpRail(
  Object const& node1, Object const& node2);

LT_API Object Object_Weapon(
  Item const& type);

LT_API Object Object_Wormhole();

LT_API void Object_Wormholes(ObjectT* a, ObjectT* b);

LT_API Object Object_Zone(
  Object const& parent, uint const& seed, Position const& position, V3 const& scale,
  SDF const& shape, uint const& statics, float const& asteroids, float const& gas,
  float const& ice, float const& planet);

#endif
