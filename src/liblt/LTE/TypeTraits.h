#ifndef LTE_TypeTraits_h__
#define LTE_TypeTraits_h__

#include "Common.h"

namespace LTE {
  template <class T>
  struct GetDereferenceType {
    using Result = void;
  };

  template <class T>
  struct GetDereferenceType<T*> {
    using Result = T;
  };

  template <class T>
  struct GetDereferenceType<const T> : public GetDereferenceType<T> {};

  template <class T>
  struct GetDereferenceType<AutoPtr<T> > : public GetDereferenceType<T*> {};

  template <class T>
  struct GetDereferenceType<Pointer<T> > : public GetDereferenceType<T*> {};

  template <class T>
  struct GetDereferenceType<Reference<T> > : public GetDereferenceType<T*> {};

  template <class T>
  struct GetReturnType { 
    using Result = typename T::ReturnType;
  };

  template <class T>
  struct GetReturnType<const T> : public GetReturnType<T> {};

  template <class T>
  struct GetReturnType<AutoPtr<T> > {
    using Result = T;
  };

  template <class T>
  struct GetReturnType<Pointer<T> > {
    using Result = T;
  };

  template <class T>
  struct GetReturnType<Reference<T> > {
    using Result = T;
  };

  template <class T>
  struct GetReturnType<T(*)()> {
    using Result = T;
  };

  template <class T, class A1>
  struct GetReturnType<T(*)(A1)> {
    using Result = T;
  };

  template <class T, class A1, class A2>
  struct GetReturnType<T(*)(A1, A2)> {
    using Result = T;
  };

  template <class T, class A1, class A2, class A3>
  struct GetReturnType<T(*)(A1, A2, A3)> {
    using Result = T;
  };

  template <class T, class A1, class A2, class A3, class A4>
  struct GetReturnType<T(*)(A1, A2, A3, A4)> {
    using Result = T;
  };

  template <class T, class S>
  struct GetReturnType<T(S::*)()> {
    using Result = T;
  };

  template <class T, class S, class A1>
  struct GetReturnType<T(S::*)(A1)> {
    using Result = T;
  };

  template <class T, class S, class A1, class A2>
  struct GetReturnType<T(S::*)(A1, A2)> {
    using Result = T;
  };

  template <class T, class S, class A1, class A2, class A3>
  struct GetReturnType<T(S::*)(A1, A2, A3)> {
    using Result = T;
  };

  template <class T, class S, class A1, class A2, class A3, class A4>
  struct GetReturnType<T(S::*)(A1, A2, A3, A4)> {
    using Result = T;
  };

  template <class T, class S>
  struct GetReturnType<T(S::*)() const> {
    using Result = T;
  };

  template <class T, class S, class A1>
  struct GetReturnType<T(S::*)(A1) const> {
    using Result = T;
  };

  template <class T, class S, class A1, class A2>
  struct GetReturnType<T(S::*)(A1, A2) const> {
    using Result = T;
  };

  template <class T, class S, class A1, class A2, class A3>
  struct GetReturnType<T(S::*)(A1, A2, A3) const> {
    using Result = T;
  };

  template <class T, class S, class A1, class A2, class A3, class A4>
  struct GetReturnType<T(S::*)(A1, A2, A3, A4) const> {
    using Result = T;
  };

  template <class T>
  struct GetReturnType<T()> {
    using Result = T;
  };

  template <class T, class A1>
  struct GetReturnType<T(A1)> {
    using Result = T;
  };

  template <class T, class A1, class A2>
  struct GetReturnType<T(A1, A2)> {
    using Result = T;
  };

  template <class T, class A1, class A2, class A3>
  struct GetReturnType<T(A1, A2, A3)> {
    using Result = T;
  };

  template <class T, class A1, class A2, class A3, class A4>
  struct GetReturnType<T(A1, A2, A3, A4)> {
    using Result = T;
  };

  template <class T>
  struct RemoveRef {
    using Result = T;
  };

  template <class T> struct RemoveRef<T&> : public RemoveRef<T> {};
  template <class T> struct RemoveRef<T const&> : public RemoveRef<T> {};
}

#endif
