#include "../RenderPasses.h"

#include "Game/Camera.h"
#include "Game/Object.h"

#include "Component/BoundingBox.h"
#include "Component/Cullable.h"
#include "Component/Drawable.h"
#include "Component/Interior.h"

#include "LTE/DrawState.h"
#include "LTE/Iterator.h"
#include "LTE/Renderable.h"
#include "LTE/View.h"

#include <algorithm>

namespace {
  /* LOD thresholds — based on screen-space coverage (radius / distance).
     LOD 0 (close):    full detail   ≥5%  of FOV height
     LOD 1 (medium):   simplified    ≥1.5%
     LOD 2 (far):      billboard     ≥0.5%
     LOD 3 (very far): culled        <0.5%  */
  static float const kLOD0Threshold = 0.05f;
  static float const kLOD1Threshold = 0.015f;
  static float const kLOD2Threshold = 0.005f;

  /* Sort key: extract the Renderable pointer from an ObjectT*.
     Objects sharing the same renderable (model) share the same shader
     and mesh, so grouping them reduces shader switches and VBO binds. */
  RenderableT const* GetRenderableKey(void const* v) {
    ObjectT const* obj = (ObjectT const*)v;
    Renderable r = obj->GetRenderable();
    return r;
  }

  struct Visibility : public RenderPassT {
    DERIVED_TYPE_EX(Visibility)

    int ComputeLODLevel(ObjectT* self, DrawState* state) {
      ComponentCullable* c = self->GetCullable();
      if (!c)
        return 0;

      float radius = self->GetRadius();
      if (radius <= 0)
        return 0;

      Transform const& transform = self->GetTransform();
      float dist = (float)Length(transform.pos - state->view->transform.pos);
      if (dist < 1.0f)
        return 0;

      float screenSize = radius / dist;

      if (screenSize >= kLOD0Threshold)
        return 0;
      if (screenSize >= kLOD1Threshold)
        return 1;
      if (screenSize >= kLOD2Threshold)
        return 2;
      return 3;
    }

    void CheckVisibility(ObjectT* self, DrawState* state) {
      if (self->GetType() == ObjectType_Light)
        state->lights.push((void*)self);

      if (!IsVisible(self, state))
        return;

      int lod = 0;
      ComponentDrawable* d = self->GetDrawable();
      if (d) {
        lod = ComputeLODLevel(self, state);
        d->lodLevel = lod;
      }

      if (lod < 3)
        state->visible.push((void*)self);

      LIST_ITERATE(Object, self->children, nextSibling) {
        CheckVisibility(*it, state);
      }
    }

    char const* GetName() const override {
      return "Visibility Pass";
    }

    bool IsVisible(ObjectT* self, DrawState* state) {
      ComponentCullable* c = self->GetCullable();
      if (!c)
        return true;

      c->Recompute(self);

      /* First, perform distance culling. */
      Transform const& transform = self->GetTransform();
      Distance d2 = LengthSquared(transform.pos - state->view->transform.pos);
      if (d2 > c->cullDistanceSquared)
        return false;

      ComponentBoundingBox* bb = self->GetBoundingBox();
      if (!bb)
        return true;

      return state->view->CanSee(self->GetGlobalBound());
    }

    void OnRender(DrawState* state) override {
      ObjectT* container = Camera_Get()->GetContainer();
      state->lights.clear();
      state->visible.clear();
      state->visible.push((void*)container);

      for (ObjectType type = 0; type < ObjectType_SIZE; ++type)
        for (InteriorTypeIterator it = Object_GetInteriorObjects(container, type);
             it.HasMore(); it.Advance())
          CheckVisibility(it.Get(), state);

      /* Sort visible objects by renderable (shader+mesh) to minimize
         state changes during draw passes. Skip index 0 (container)
         since Particles pass uses visible[0] as the root object. */
      if (state->visible.size() > 2) {
        std::sort(
          state->visible.begin() + 1,
          state->visible.end(),
          [](void const* a, void const* b) {
            return GetRenderableKey(a) < GetRenderableKey(b);
          });
      }
    }
  };
}

DefineFunction(RenderPass_Visibility) {
  return new Visibility;
}
