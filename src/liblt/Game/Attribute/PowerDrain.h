#ifndef Attribute_PowerDrain_h__
#define Attribute_PowerDrain_h__

#include "Common.h"

template <class T>
struct Attribute_PowerDrain : public T {
  using SelfType = Attribute_PowerDrain;
  ATTRIBUTE_COMMON(powerDrain)
  float powerDrain;

  float const& GetPowerDrain() const override {
    return powerDrain;
  }

  bool HasPowerDrain() const override {
    return true;
  }
};

#endif
