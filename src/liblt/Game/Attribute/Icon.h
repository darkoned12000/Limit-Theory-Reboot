#ifndef Attribute_Icon_h__
#define Attribute_Icon_h__

#include "Common.h"
#include "UI/Icon.h"

template <class T>
struct Attribute_Icon : public T {
  using SelfType = Attribute_Icon;
  ATTRIBUTE_COMMON(icon)
  Icon icon;

  Icon const& GetIcon() const override {
    return icon;
  }

  bool HasIcon() const override {
    return true;
  }
};

#endif
