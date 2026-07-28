#ifndef Attribute_Integrity_h__
#define Attribute_Integrity_h__

#include "Common.h"

template <class T>
struct Attribute_Integrity : public T {
  using SelfType = Attribute_Integrity;
  ATTRIBUTE_COMMON(integrity)
  Health integrity;

  Attribute_Integrity() :
    integrity(0)
    {}

  Health const& GetIntegrity() const override {
    return integrity;
  }

  bool HasIntegrity() const override {
    return true;
  }
};

#endif
