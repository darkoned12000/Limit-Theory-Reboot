#include "Order.h"

#include "Item.h"
#include "Object.h"
#include "LTE/FunctionBind.h"

Order Order_Create(Object const& owner, Item const& item, Quantity const& volume, Quantity const& price) {
  Order self = new OrderT;
  self->owner = owner;
  self->item = item;
  self->volume = volume;
  self->price = price;
  return self;
}
static Function const Order_Create_Registration = Function_Bind(
  "Order_Create",
  "None",
  &Order_Create,
  "owner", "item", "volume", "price");


