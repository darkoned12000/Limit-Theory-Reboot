#ifndef LTE_Mutable_h__
#define LTE_Mutable_h__

#include "Common.h"

namespace LTE {
  template <class T>
  struct GetMutableType {
    using Result = T;
  };

  template <class T>
  struct GetMutableType<const T> {
    using Result = T;
  };

  template <class T>
  T& Mutable(T& t) {
    return t;
  }

  template <class T>
  T& Mutable(T const& t) {
    return (T&)t;
  }

  template <class T>
  T* Mutable(T* t) {
    return t;
  }

  template <class T>
  T* Mutable(T const* t) {
    return (T*)t;
  }
}

#endif
