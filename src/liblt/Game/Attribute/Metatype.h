#ifndef Attribute_Metatype_h__
#define Attribute_Metatype_h__

#include "Common.h"
#include "LTE/Data.h"

template <class T>
struct Attribute_Metatype : public T {
  using SelfType = Attribute_Metatype;
  ATTRIBUTE_COMMON(metatype)
  Data metatype;

  Attribute_Metatype() = default;

  Data const& GetMetatype() const override {
    return metatype;
  }

  bool HasMetatype() const override {
    return true;
  }
};

#endif
