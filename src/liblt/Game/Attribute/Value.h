#ifndef Attribute_Value_h__
#define Attribute_Value_h__

#include "Common.h"

template <class T>
struct Attribute_Value : public T {
  using SelfType = Attribute_Value;
  ATTRIBUTE_COMMON(value)
  Quantity value;

  Attribute_Value() :
    value(0)
    {}

  Quantity const& GetValue() const override {
    return value;
  }

  bool HasValue() const override {
    return true;
  }
};

#endif
