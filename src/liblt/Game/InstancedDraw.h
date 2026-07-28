#ifndef InstancedDraw_h__
#define InstancedDraw_h__

#include "Game/Object.h"

#include "LTE/DrawState.h"
#include "LTE/Matrix.h"
#include "LTE/Renderer.h"
#include "LTE/Renderable.h"
#include "LTE/RenderStyle.h"
#include "LTE/Transform.h"

#include <vector>

namespace {
  /* Maximum batch size for GPU instancing. Objects beyond this in a same-
     renderable run fall back to individual draw calls. */
  const int kMaxInstanceBatch = 256;

  /* Draw all visible objects using instanced batching where possible.
     Objects in state->visible are already sorted by renderable pointer
     (Phase 1.4), so consecutive same-renderable objects form natural batches.
     Index 0 is always the container object — it must be drawn first via
     OnDraw because ComponentInterior::Draw triggers OnDrawInterior (particles,
     stars, dust) only when visible[0] == self. */
  void DrawVisibleInstanced(DrawState* state) {
    /* Always draw the container first — this triggers interior rendering
       (particles, starfield, nebula) via ComponentInterior::OnDraw. */
    ((ObjectT*)state->visible[0])->OnDraw(state);

    static std::vector<Matrix> matrices;
    matrices.clear();

    size_t i = 1;
    while (i < state->visible.size()) {
      ObjectT* obj = (ObjectT*)state->visible[i];
      Renderable renderable = obj->GetRenderable();

      if (!renderable) {
        obj->OnDraw(state);
        ++i;
        continue;
      }

      /* Count consecutive objects sharing the same renderable. */
      size_t batchStart = i;
      while (i < state->visible.size()) {
        ObjectT* candidate = (ObjectT*)state->visible[i];
        if (candidate->GetRenderable() != renderable)
          break;
        ++i;
      }

      size_t batchCount = i - batchStart;

      if (batchCount == 1 || (int)batchCount > kMaxInstanceBatch) {
        /* Single object or oversized batch — use normal path. */
        obj->OnDraw(state);
        /* For oversized batches, draw the rest individually. */
        for (size_t j = batchStart + 1; j < i; ++j)
          ((ObjectT*)state->visible[j])->OnDraw(state);
      } else {
        /* Collect interleaved world + worldIT matrices for the batch.
         The SSBO struct is { mat4 world; mat4 worldIT; } per instance,
         so we push two matrices per object. */
        matrices.clear();
        matrices.reserve(batchCount * 2);
        for (size_t j = batchStart; j < i; ++j) {
          ObjectT* batchObj = (ObjectT*)state->visible[j];
          Matrix world = batchObj->GetTransform().GetMatrix();
          Matrix worldIT = world.Inverse().Transpose();
          matrices.push_back(world);
          matrices.push_back(worldIT);
        }

        /* Set the world transform to the first object so shader setup
           (InjectMatrices) has a valid WORLD uniform. The vertex shader
           ignores this when uInstanced > 0, using the SSBO instead. */
        RenderStyle_Get()->SetTransform(obj->GetTransform());

        Renderer_BeginInstancedDraw(matrices.data(), (int)batchCount);
        renderable->RenderInstanced(state, (int)batchCount);
        Renderer_EndInstancedDraw();
      }
    }
  }
}

#endif
