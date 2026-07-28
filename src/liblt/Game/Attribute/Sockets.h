#ifndef Attribute_Sockets_h__
#define Attribute_Sockets_h__

#include "Common.h"
#include "LTE/Array.h"
#include "Game/Socket.h"

template <class T>
struct Attribute_Sockets : public T {
  using SelfType = Attribute_Sockets;
  Array<Socket> sockets;

  Array<Socket> const* GetSockets() const override {
    return &sockets;
  }

  bool HasSockets() const {
    return true;
  }
};

#endif
