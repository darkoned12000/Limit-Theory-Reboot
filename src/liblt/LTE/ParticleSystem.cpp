// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
// Substantial modification: added ParticleSystem_Add_Position overload (Position/Vec3d).

#include "ParticleSystem.h"

#include "DrawState.h"
#include "Map.h"
#include "Meshes.h"
#include "Renderer.h"
#include "RenderStyle.h"
#include "ShaderInstance.h"
#include "Vector.h"
#include "View.h"

#include <algorithm>
#include "Debug.h"
#include "LTE/FunctionBind.h"

const size_t kMaxParticles = 1024 * 1024;
const float kLodFactor = Squared(512);

namespace {
  Vector<ParticleSystem>& GetStack() {
    static Vector<ParticleSystem> stack;
    return stack;
  }

  Mesh gParticleMesh = Mesh_Billboard(-1, 1, -1, 1);

  struct Particle {
    V3D p;
    V3 v;
    V3 attrib;
    float size;
    float maxLife;
    float life;

    Particle(
        V3D const& p,
        V3 const& v,
        float size,
        float maxLife,
        V3 const& attrib) :
      p(p),
      v(v),
      attrib(attrib),
      size(size),
      maxLife(maxLife),
      life(maxLife)
      {}

    friend bool operator<(Particle const& a, Particle const& b) {
      return b.life < a.life;
    }
  };

  AutoClassDerivedEmpty(ParticleSystemImpl, ParticleSystemT)
    using ParticleMapT = Map<ShaderInstance, Vector<Particle> >;
    ParticleMapT particles;
    DERIVED_TYPE_EX(ParticleSystemImpl)

    ParticleSystemImpl() = default;

    void Draw(DrawState* state) const override {
      RenderStyle const& style = RenderStyle_Get();

      for (ParticleMapT::const_iterator it = particles.begin();
           it != particles.end();
           ++it)
      {
        ShaderInstance const& shader = it->first;
        Vector<Particle> const& particles = it->second;

        style->SetTransform(Transform_Identity());
        style->SetShader(shader);

        if (!style->WillRender())
          continue;

        static Vector<ParticleInstanceData> instances;
        instances.clear();
        instances.reserve(particles.size());

        V3D camPos = state->view->transform.pos;

        for (size_t i = 0; i < particles.size(); ++i) {
          Particle const& particle = particles[i];
          if (!state->view->CanSee(particle.p))
            continue;

          V3D position = particle.p - camPos;
          float d2 = LengthSquared(position);
          float r2 = Squared(particle.size);
          if (d2 >= kLodFactor * r2)
            continue;

          float age = (particle.maxLife - particle.life) / particle.maxLife;

          ParticleInstanceData inst;
          inst.position = V3(position);
          inst.size = particle.size;
          inst.age = age;
          inst.attrib = particle.attrib;
          instances.push(inst);
        }

        if (instances.empty())
          continue;

        DrawState_Link(shader);
        shader->Begin();
        state->primary->Bind(0);

        Renderer_DrawParticlesInstanced(
          gParticleMesh,
          instances.data(),
          (int)instances.size());

        state->primary->Unbind();
        shader->End();
      }
    }

    void Run(float dt) override {
      for (ParticleMapT::iterator it = particles.begin();
           it != particles.end();
           ++it)
      {
        Vector<Particle>& particles = it->second;

        /* Particle update. */
        for (int i = 0; i < (int)particles.size(); ++i) {
          Particle& particle = particles[i];
          particle.p += dt * particle.v;
          particle.life -= dt;
          if (particle.life <= 0.0f) {
            particles.removeIndex(i);
            i--;
            continue;
          }
        }

        /* Enforce max particle count. */
        if (particles.size() > kMaxParticles) {
          // std::sort(particles.v.begin(), particles.v.end());
          while (particles.size() > kMaxParticles)
            particles.pop();
        }
      }
    }
  };
}

void ParticleSystem_Add(ParticleSystem const& particleSystem, ShaderInstance const& particle, V3D const& position, V3 const& velocity, float const& scale, float const& life, V3 const& attribute) {
  ((ParticleSystemImpl*)particleSystem.t)
    ->particles[particle].push(Particle(
    position,
    velocity,
    scale,
    life,
    attribute));
}
static Function const ParticleSystem_Add_Registration = Function_Bind(
  "ParticleSystem_Add",
  "None",
  &ParticleSystem_Add,
  "particleSystem", "particle", "position", "velocity", "scale", "life", "attribute");
static int const ParticleSystem_Add_Alias = Function_Alias("ParticleSystem_Add", "Add");



/* Overload accepting Position (V3D) for velocity/attribute. See AGENTS.md §8d #1
 * and ParticleSystem.h. Avoids a V3D -> V3F conversion that corrupts the global
 * conversion table. */
void ParticleSystem_Add_Position(ParticleSystem const& particleSystem, ShaderInstance const& particle, V3D const& position, V3D const& velocity, float const& scale, float const& life, V3D const& attribute) {
  ((ParticleSystemImpl*)particleSystem.t)
    ->particles[particle].push(Particle(
    V3(position),
    V3(velocity),
    scale,
    life,
    V3(attribute)));
}
static Function const ParticleSystem_Add_Position_Registration = Function_Bind(
  "ParticleSystem_Add_Position",
  "None",
  &ParticleSystem_Add_Position,
  "particleSystem", "particle", "position", "velocity", "scale", "life", "attribute");
static int const ParticleSystem_Add_Position_Alias = Function_Alias("ParticleSystem_Add_Position", "Add");



ParticleSystem ParticleSystem_Create() {
  return new ParticleSystemImpl;
}
static Function const ParticleSystem_Create_Registration = Function_Bind(
  "ParticleSystem_Create",
  "None",
  &ParticleSystem_Create);



ParticleSystem ParticleSystem_Get() {
  return GetStack().size() ? GetStack().back() : nullptr;
}
static Function const ParticleSystem_Get_Registration = Function_Bind(
  "ParticleSystem_Get",
  "None",
  &ParticleSystem_Get);



void ParticleSystem_Pop() {
  GetStack().pop();
}
static Function const ParticleSystem_Pop_Registration = Function_Bind(
  "ParticleSystem_Pop",
  "None",
  &ParticleSystem_Pop);



void ParticleSystem_Push(ParticleSystem const& ps) {
  GetStack().push(ps);
}
static Function const ParticleSystem_Push_Registration = Function_Bind(
  "ParticleSystem_Push",
  "None",
  &ParticleSystem_Push,
  "ps");


