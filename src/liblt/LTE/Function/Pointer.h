#ifndef LTE_Function_Pointer_h__
#define LTE_Function_Pointer_h__

#include "Wrapper.h"

namespace LTE {
  template <class T>
  struct ConstantPointerT {
    using ReturnType = T;

    T const* t;

    ConstantPointerT(T const* t) :
      t(t)
      {}

    T operator()() {
      return *t;
    }
  };

  template <class T>
  struct DereferenceT {
    using ReturnType = typename GetDereferenceType<T>::Result;

    T t;

    DereferenceT(T const& t) :
      t(t)
      {}

    ReturnType& operator()() {
      return *t;
    }
  };

  template <class T>
  WrapperT<ConstantPointerT<T>, typename ConstantPointerT<T>::ReturnType>
  ConstantPointer(T const* t) {
    return Wrapper(ConstantPointerT<T>(t));
  }

  template <class T>
  WrapperT<DereferenceT<T>, typename DereferenceT<T>::ReturnType>
  Dereference(T const& t) {
    return Wrapper(DereferenceT<T>(t));
  }
}

#endif
