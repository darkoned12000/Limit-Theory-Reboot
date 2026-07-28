#ifndef Attribute_Name_h__
#define Attribute_Name_h__

#include "Common.h"
#include "LTE/String.h"

template <class T>
struct Attribute_Name : public T {
  using SelfType = Attribute_Name;
  ATTRIBUTE_COMMON(name)
  String name;

  String const& GetName() const override {
    return name;
  }

  bool HasName() const override {
    return true;
  }

  String ToString() const override {
    return name;
  }
};

#endif
