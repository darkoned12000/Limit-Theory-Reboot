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
    typedef Map<ShaderInstance, Vector<Particle> > ParticleMapT;
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

DefineFunction(ParticleSystem_Add) {
  ((ParticleSystemImpl*)args.particleSystem.t)
    ->particles[args.particle].push(Particle(
    args.position,
    args.velocity,
    args.scale,
    args.life,
    args.attribute));
} FunctionAlias(ParticleSystem_Add, Add);

/* Overload accepting Position (V3D) for velocity/attribute. See AGENTS.md §8d #1
 * and ParticleSystem.h. Avoids a V3D -> V3F conversion that corrupts the global
 * conversion table. */
DefineFunction(ParticleSystem_Add_Position) {
  ((ParticleSystemImpl*)args.particleSystem.t)
    ->particles[args.particle].push(Particle(
    V3(args.position),
    V3(args.velocity),
    args.scale,
    args.life,
    V3(args.attribute)));
} FunctionAlias(ParticleSystem_Add_Position, Add);

DefineFunction(ParticleSystem_Create) {
  return new ParticleSystemImpl;
}

DefineFunction(ParticleSystem_Get) {
  return GetStack().size() ? GetStack().back() : nullptr;
}

DefineFunction(ParticleSystem_Pop) {
  GetStack().pop();
}

DefineFunction(ParticleSystem_Push) {
  GetStack().push(args.ps);
}
