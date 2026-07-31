#ifndef Attribute_Renderable_h__
#define Attribute_Renderable_h__

#include "Common.h"
#include "LTE/Renderable.h"

template <class T>
struct Attribute_Renderable : public T {
  using SelfType = Attribute_Renderable;
  ATTRIBUTE_COMMON(renderable)
  Renderable renderable;

  Renderable const& GetRenderable() const override {
    return renderable;
  }

  bool HasRenderable() const override {
    return true;
  }
};

#endif
