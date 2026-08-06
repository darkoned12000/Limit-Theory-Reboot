// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
// Substantial modification: added ParticleSystem_Add_Position declaration (Position/Vec3d).

#ifndef LTE_ParticleSystem_h__
#define LTE_ParticleSystem_h__

#include "BaseType.h"
#include "Reference.h"
#include "V3.h"

struct ParticleSystemT : public RefCounted {
  BASE_TYPE(ParticleSystemT)

  virtual void Draw(DrawState* state) const = 0;
  virtual void Run(float dt) = 0;
};

LT_API void ParticleSystem_Add(
  ParticleSystem const& particleSystem, ShaderInstance const& particle,
  V3D const& position, V3 const& velocity, float const& scale, float const& life,
  V3 const& attribute);

/* Overload accepting Position (V3D) for velocity/attribute so scripts that work
 * in double-precision Position space (e.g. WarpRail) don't need a V3D -> V3F
 * conversion (which corrupts the engine's global conversion table; see
 * AGENTS.md §8d #1). */
LT_API void ParticleSystem_Add_Position(
  ParticleSystem const& particleSystem, ShaderInstance const& particle,
  V3D const& position, V3D const& velocity, float const& scale, float const& life,
  V3D const& attribute);

LT_API ParticleSystem ParticleSystem_Create();

LT_API ParticleSystem ParticleSystem_Get();

LT_API void ParticleSystem_Pop();

LT_API void ParticleSystem_Push(
  ParticleSystem const& ps);

#endif
