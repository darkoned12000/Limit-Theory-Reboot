#ifndef Attribute_Uses_h__
#define Attribute_Uses_h__

#include "Common.h"

template <class T>
struct Attribute_Uses : public T {
  using SelfType = Attribute_Uses;
  ATTRIBUTE_COMMON(uses)
  Quantity uses;

  Attribute_Uses() :
    uses(0)
    {}

  Quantity GetUses() const override {
    return uses;
  }

  bool HasUses() const {
    return true;
  }
};

#endif
