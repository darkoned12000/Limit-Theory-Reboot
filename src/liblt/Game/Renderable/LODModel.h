// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef LTE_LODModel_h__
#define LTE_LODModel_h__

#include "LTE/AutoClass.h"
#include "LTE/DrawState.h"
#include "LTE/Mesh.h"
#include "LTE/Renderable.h"

namespace LTE {
  AutoClassDerived(LODModel, RenderableT,
    Renderable, lod0,
    Renderable, lod1,
    Renderable, lod2)
    DERIVED_TYPE_EX(LODModel)

    LODModel() {}

    Bound3 GetBound() const override {
      return lod0 ? lod0->GetBound() : Bound3();
    }

    Mesh GetCollisionMesh() const override {
      return lod0 ? lod0->GetCollisionMesh() : nullptr;
    }

    size_t GetHash() const override {
      return lod0 ? lod0->GetHash() : 0;
    }

    short GetVersion() const override {
      return lod0 ? lod0->GetVersion() : 0;
    }

    V3 Sample() const override {
      return lod0 ? lod0->Sample() : V3();
    }

    void Render(DrawState* state) const override {
      int lod = state->lodLevel;
      Renderable target;
      if (lod <= 0)
        target = lod0;
      else if (lod == 1)
        target = lod1;
      else
        target = lod2;
      if (target)
        target->Render(state);
    }
  };

  inline Renderable LODModel_Create(
    Renderable const& r0,
    Renderable const& r1,
    Renderable const& r2)
  {
    return new LODModel(r0, r1, r2);
  }
}

#endif
