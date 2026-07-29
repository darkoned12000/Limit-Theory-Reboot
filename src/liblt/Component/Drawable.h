#ifndef Drawable_h__
#define Drawable_h__

#include "Common.h"
#include "Game/Item.h"
#include "LTE/AutoClass.h"
#include "LTE/Generic.h"
#include "LTE/Renderable.h"

/* CRITICAL : Serialization of renderable. */
AutoClassEmpty(ComponentDrawable)
  Generic<Renderable> renderable;
  int lodLevel = 0;

  LT_API void Draw(ObjectT* self, DrawState* state);
};

AutoComponent(Drawable)
  void OnDraw(DrawState* state) override {
    Drawable.Draw(this, state);
    BaseT::OnDraw(state);
  }

  Renderable GetRenderable() const override {
    return Drawable.renderable ? Drawable.renderable() : nullptr;
  }

  void SetSupertype(Item const& type) override {
    Renderable const& renderable = type->GetRenderable();
    if (renderable)
      Drawable.renderable = renderable;

    BaseT::SetSupertype(type);
  }
};

#endif
