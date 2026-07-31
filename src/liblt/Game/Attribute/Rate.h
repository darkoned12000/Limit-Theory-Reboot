#ifndef Attribute_Rate_h__
#define Attribute_Rate_h__

#include "Common.h"

template <class T>
struct Attribute_Rate : public T {
  using SelfType = Attribute_Rate;
  ATTRIBUTE_COMMON(rate)
  float rate;

  Attribute_Rate() :
    rate(0)
    {}

  float const& GetRate() const override {
    return rate;
  }

  bool HasRate() const override {
    return true;
  }
};

#endif
