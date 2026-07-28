#ifndef Attribute_Hash_h__
#define Attribute_Hash_h__

#include "Common.h"

template <class T>
struct Attribute_Hash : public T {
  using SelfType = Attribute_Hash;
  ATTRIBUTE_COMMON(hash)
  HashT hash;

  Attribute_Hash() :
    hash(0)
    {}

  HashT const& GetHash() const override {
    return hash;
  }

  bool HasHash() const override {
    return true;
  }
};

#endif
