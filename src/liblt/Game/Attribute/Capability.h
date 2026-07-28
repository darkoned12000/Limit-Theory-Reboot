#ifndef Attribute_Capability_h__
#define Attribute_Capability_h__

#include "Common.h"
#include "Game/Capability.h"

template <class T>
struct Attribute_Capability : public T {
  using SelfType = Attribute_Capability;
  ATTRIBUTE_COMMON(capability)
  Capability capability;

  Capability const& GetCapability() const override {
    return capability;
  }

  bool HasCapability() const override {
    return true;
  }
};

#endif
