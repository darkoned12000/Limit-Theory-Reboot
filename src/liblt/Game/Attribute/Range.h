#ifndef Attribute_Range_h__
#define Attribute_Range_h__

#include "Common.h"

/* TODO : Is merging going to break down here? How to handle range? */

template <class T>
struct Attribute_Range : public T {
  using SelfType = Attribute_Range;
  ATTRIBUTE_COMMON(range)
  float range;

  Attribute_Range() :
    range(0)
    {}

  float const& GetRange() const override {
    return range;
  }

  bool HasRange() const override {
    return true;
  }
};

#endif
