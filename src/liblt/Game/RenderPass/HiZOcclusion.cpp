#include "../RenderPasses.h"

#include "Component/Drawable.h"

#include "Game/Object.h"

#include "LTE/DrawState.h"
#include "LTE/Map.h"
#include "LTE/Renderer.h"
#include "LTE/Texture2D.h"
#include "LTE/Vector.h"
#include "LTE/View.h"

#include <cstdlib>
#include <climits>

namespace {
  struct HiZOcclusion : public RenderPassT {
    float* depthReadback = nullptr;
    size_t depthReadbackSize = 0;
    Map<void*, int> fadeFrames;
    DERIVED_TYPE_EX(HiZOcclusion)

    ~HiZOcclusion() {
      free(depthReadback);
    }

    char const* GetName() const override {
      return "Hi-Z Occlusion";
    }

    void OnRender(DrawState* state) override {
      if (!state->depth)
        return;

      int w = (int)state->depth->GetWidth();
      int h = (int)state->depth->GetHeight();

      size_t needed = (size_t)w * h;
      if (depthReadbackSize < needed) {
        free(depthReadback);
        depthReadback = (float*)malloc(needed * sizeof(float));
        depthReadbackSize = needed;
      }

      state->depth->GetData(depthReadback);

      Matrix const& viewMatrix = Renderer_GetViewMatrix();
      Matrix const& projMatrix = Renderer_GetProjMatrix();
      Matrix viewProj = projMatrix * viewMatrix;

      Vector<void*>& visible = state->visible;
      if (visible.size() <= 1)
        return;

      size_t writeIdx = 1;

      for (size_t i = 1; i < visible.size(); ++i) {
        ObjectT* obj = (ObjectT*)visible[i];

        if (!obj->HasComponent(ComponentType_BoundingBox)) {
          visible[writeIdx++] = visible[i];
          continue;
        }

        Bound3D db = obj->GetGlobalBound();
        Bound3 b(db);

        float minZ = FLT_MAX;
        float minX = FLT_MAX, maxX = -FLT_MAX;
        float minY = FLT_MAX, maxY = -FLT_MAX;
        bool safe = true;

        for (size_t j = 0; j < 8 && safe; ++j) {
          float wc;
          V3 clip = viewProj.TransformV3(b[j], 1.0f, wc);

          if (wc <= 0.001f) {
            safe = false;
            break;
          }

          float invW = 1.0f / wc;
          float nx = clip.x * invW;
          float ny = clip.y * invW;
          minX = Min(minX, nx);
          maxX = Max(maxX, nx);
          minY = Min(minY, ny);
          maxY = Max(maxY, ny);
          minZ = Min(minZ, clip.z);
        }

        if (!safe) {
          visible[writeIdx++] = visible[i];
          continue;
        }

        if (maxX < -1.0f || minX > 1.0f ||
            maxY < -1.0f || minY > 1.0f)
        {
          visible[writeIdx++] = visible[i];
          continue;
        }

        if (minX < -1.0f) minX = -1.0f;
        if (maxX >  1.0f) maxX =  1.0f;
        if (minY < -1.0f) minY = -1.0f;
        if (maxY >  1.0f) maxY =  1.0f;

        float qx = (maxX - minX) * 0.25f;
        float qy = (maxY - minY) * 0.25f;
        float cx = (minX + maxX) * 0.5f;
        float cy = (minY + maxY) * 0.5f;

        float sampleNDC[5][2] = {
          { cx, cy },
          { minX + qx, maxY - qy },
          { maxX - qx, maxY - qy },
          { minX + qx, minY + qy },
          { maxX - qx, minY + qy }
        };

        bool occluded = true;
        for (int s = 0; s < 5 && occluded; ++s) {
          int sx = (int)((sampleNDC[s][0] * 0.5f + 0.5f) * w);
          int sy = (int)((sampleNDC[s][1] * 0.5f + 0.5f) * h);
          if (sx < 0) sx = 0;
          if (sx >= w) sx = w - 1;
          if (sy < 0) sy = 0;
          if (sy >= h) sy = h - 1;

          if (minZ <= depthReadback[sy * w + sx])
            occluded = false;
        }

        if (occluded)
          continue;

        visible[writeIdx++] = visible[i];
      }

      visible.resize(writeIdx);

      /* Phase 2: Multi-step fade-in ramp.
         Objects that were absent from the Hi-Z survivors list last frame
         enter at LOD 2 (coarsest), graduate to LOD 1 (medium) after
         kFadeFramesLOD2 frames, then to their proper LOD after
         kFadeFramesLOD1 more.  Spreading the transitions over ~16 frames
         (~267ms @ 60fps) makes pop-in and LOD-shift far less noticeable. */

      static int const kFadeLOD2 = 8;
      static int const kFadeLOD1 = 8;
      static int const kFadeTotal = kFadeLOD2 + kFadeLOD1;

      Map<void*, bool> survived;
      for (size_t i = 1; i < writeIdx; ++i)
        survived[visible[i]] = true;

      Vector<void*> stale;
      for (auto it = fadeFrames.begin(); it != fadeFrames.end(); ++it)
        if (!survived.contains(it->first))
          stale.push(it->first);
      for (size_t i = 0; i < stale.size(); ++i)
        fadeFrames.erase(stale[i]);

      for (size_t i = 1; i < writeIdx; ++i) {
        int& frames = fadeFrames[visible[i]];
        frames++;

        ObjectT* obj = (ObjectT*)visible[i];
        ComponentDrawable* d = obj->GetDrawable();
        if (!d)
          continue;

        if (frames <= kFadeLOD2) {
          if (d->lodLevel > 2)
            d->lodLevel = 2;
        } else if (frames <= kFadeTotal) {
          if (d->lodLevel > 1)
            d->lodLevel = 1;
        }
      }
    }
  };
}

DefineFunction(RenderPass_HiZOcclusion) {
  return new HiZOcclusion;
}
