#ifndef Attribute_Sound_h__
#define Attribute_Sound_h__

#include "Common.h"

template <class T>
struct Attribute_Sound : public T {
  using SelfType = Attribute_Sound;
  ATTRIBUTE_COMMON(sound)
  String sound;

  String GetSound() const override {
    return sound;
  }

  bool HasSound() const {
    return true;
  }
};

#endif
