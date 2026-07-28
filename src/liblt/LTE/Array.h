#ifndef LTE_Array_h__
#define LTE_Array_h__

// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Type.h"

template <class T>
struct Array : public NullBase<Array<T> > {
  using ArgType = size_t;
  using BaseType = NullBase<Array<T> >;
  using ReturnType = T;

  T* buffer;
  size_t _size;

  struct Iterator {
    T* elem;

    Iterator(T* elem) :
      elem(elem)
      {}

    friend bool operator!=(Iterator const& a, Iterator const& b) {
      return a.elem != b.elem;
    }

    Iterator& operator++() {
      elem++;
      return *this;
    }

    Iterator operator++(int unused) {
      Iterator it = *this;
      elem++;
      return it;
    }

    T& operator*() {
      return *elem;
    }

    T* operator->() {
      return elem;
    }
  };

  using IteratorType = Iterator;

  Array() :
    buffer(0),
    _size(0)
    {}

  Array(size_t size) :
    _size(size)
  {
    buffer = new T[size];
  }

  Array(size_t size, T const& elem) :
    _size(size)
  {
    buffer = new T[size];
    for (size_t i = 0; i < size; ++i)
      buffer[i] = elem;
  }

  Array(Array const& other) :
    _size(other._size)
  {
    buffer = new T[_size];
    for (size_t i = 0; i < _size; ++i)
      buffer[i] = other.buffer[i];
  }

  Array(size_t size, T const* elems) :
    _size(size)
  {
    buffer = new T[_size];
    for (size_t i = 0; i < _size; ++i)
      buffer[i] = elems[i];
  }

  ~Array() {
    delete[] buffer;
  }

  T& operator[](size_t index) const {
#ifdef DEBUG_BOUNDS_CHECK
    LTE_ASSERT(index < _size);
#endif
    return buffer[index];
  }

  Array& operator=(Array const& other) {
    if (this == &other)
      return *this;
    delete[] buffer;
    buffer = 0;
    _size = other._size;
    if (_size) {
      buffer = new T[_size];
      for (size_t i = 0; i < _size; ++i)
        buffer[i] = other.buffer[i];
    }
    return *this;
  }

  friend bool operator==(const Array& one, const Array& two) {
    if (one.size() != two.size())
      return false;
    if (!one.data() || !two.data())
      return !one.data() && !two.data();
    return memcmp(one.data(), two.data(), one.size()) == 0;
  }

  Iterator begin() {
    return Iterator(buffer);
  }

  void clear() {
    delete[] buffer;
    buffer = 0;
    _size = 0;
  }

  T* data() const {
    return buffer;
  }

  Iterator end() {
    return Iterator(buffer + _size);
  }

  size_t memory() const {
    return _size * sizeof(T);
  }

  T& random() {
    LTE_ASSERT(size());
    return (*this)[rand() % size()];
  }

  T const& random() const {
    LTE_ASSERT(size());
    return (*this)[rand() % size()];
  }

  void release() {
    buffer = 0;
    _size = 0;
  }

  void replace(Array& other) {
    delete[] buffer;
    buffer = other.buffer;
    _size = other._size;
    other.release();
  }

  void resize(size_t newSize) {
    if (_size != newSize) {
      delete[] buffer;
      _size = newSize;
      buffer = new T[newSize];
    }
  }

  void resize(size_t newSize, T const& t) {
    if (_size != newSize) {
      delete[] buffer;
      _size = newSize;
      buffer = new T[newSize];
      for (size_t i = 0; i < newSize; ++i)
        buffer[i] = t;
    }
  }

  Array* set(T* newBuffer, size_t bufferSize) {
    delete[] buffer;
    buffer = newBuffer;
    _size = bufferSize;
    return this;
  }

  size_t size() const {
    return _size;
  }

  FIELDS {
    Array* self = (Array*)addr;
    size_t oldSize = self->size();
    size_t sz = oldSize;

    m(&sz, "size", Type_Get(sz), aux);

    if (sz != oldSize)
      self->resize(sz);
    
    for (size_t i = 0; i < sz; ++i)
      m(&(*self)[i], "data", Type_Get((*self)[i]), aux);
  }

  template <class StreamT>
  friend void _ToStream(StreamT& s, Array const& v) {
    s << '[';
    for (size_t i = 0; i < v.size(); ++i) {
      if (i)
        s << ", ";
      ToStream(s, v[i]);
    }
    s << ']';
  }

  AUTOMATIC_REFLECTION_PARAMETRIC1(Array, T)
};

#endif
