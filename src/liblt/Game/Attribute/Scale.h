#ifndef Attribute_Scale_h__
#define Attribute_Scale_h__

#include "Common.h"

template <class T>
struct Attribute_Scale : public T {
  using SelfType = Attribute_Scale;
  ATTRIBUTE_COMMON(scale)
  float scale;

  Attribute_Scale() :
    scale(0)
    {}

  float const& GetScale() const override {
    return scale;
  }

  bool HasScale() const override {
    return true;
  }
};

#endif
