#ifndef Attribute_Speed_h__
#define Attribute_Speed_h__

#include "Common.h"

template <class T>
struct Attribute_Speed : public T {
  using SelfType = Attribute_Speed;
  ATTRIBUTE_COMMON(speed)
  float speed;

  Attribute_Speed() :
    speed(0)
    {}

  float const& GetSpeed() const override {
    return speed;
  }

  bool HasSpeed() const override {
    return true;
  }
};

#endif
