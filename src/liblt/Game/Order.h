#ifndef Game_Order_h__
#define Game_Order_h__

#include "Common.h"

#include "LTE/AutoClass.h"
#include "LTE/Pool.h"
#include "LTE/Reference.h"

AutoClassDerived(OrderT, RefCounted,
  Object, owner,
  Item, item,
  Quantity, volume,
  Quantity, price,
  Quantity, filledVolume,
  Quantity, filledTotal,
  Object, node)
  POOLED_TYPE

  OrderT() :
    volume(0),
    price(0),
    filledVolume(0),
    filledTotal(0)
    {}
};

LT_API Order Order_Create(
  Object const& owner, Item const& item, Quantity const& volume, Quantity const& price);

#endif
