#ifndef Attribute_Seed_h__
#define Attribute_Seed_h__

#include "Common.h"

template <class T>
struct Attribute_Seed : public T {
  using SelfType = Attribute_Seed;
  ATTRIBUTE_COMMON(seed)
  uint seed;

  Attribute_Seed() :
    seed(0)
    {}

  uint const& GetSeed() const override {
    return seed;
  }

  bool HasSeed() const override {
    return true;
  }
};

#endif
