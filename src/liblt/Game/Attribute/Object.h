#ifndef Attribute_Object_h__
#define Attribute_Object_h__

#include "Common.h"

template <class T>
struct Attribute_Object : public T {
  using SelfType = Attribute_Object;
  ATTRIBUTE_COMMON(object)
  Object object;

  Attribute_Object() = default;

  Object const& GetObject() const override {
    return object;
  }

  bool HasObject() const override {
    return true;
  }
};

#endif
