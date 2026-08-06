#include "Sockets.h"
#include "Attachable.h"
#include "Motion.h"
#include "Orientation.h"
#include "Pluggable.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

bool ComponentSockets::Plug(ObjectT* self, Item const& type) {
  for (uint i = 0; i < sockets.size(); ++i)
    if (!instances[i] && Plug(self, type, i))
      return true;
  return false;
}

bool ComponentSockets::Plug(ObjectT* self, Object const& object) {
  for (uint i = 0; i < sockets.size(); ++i)
    if (!instances[i] && Plug(self, object, i))
      return true;
  return false;
}

bool ComponentSockets::Plug(ObjectT* self, Item const& type, uint index) {
  if (index >= sockets.size())
    return false;

  Socket const& thisSocket = sockets[index];
  if (thisSocket.type != type->GetSocketType())
    return false;

  Unplug(self, index);
  Object newChild = type->Instantiate(self);
  LTE_ASSERT(newChild);
  Plug(self, newChild, index);
  return true;
}

bool ComponentSockets::Plug(ObjectT* self, Object const& object, uint index) {
  Socket const& thisSocket = sockets[index];
  instances[index] = object;
  ComponentAttachable* at = object->GetAttachable();
  if (at)
    at->transform = thisSocket.transform * at->transform;
  
  self->AddChild(object);

  ComponentPluggable* plug = object->GetPluggable();
  if (plug)
    plug->index = index;

  return true;
}

void ComponentSockets::Unplug(ObjectT* self, uint index) {
  LTE_ASSERT(index < instances.size());
  if (instances[index]) {
    self->RemoveChild(instances[index]);
    instances[index]->Delete();
    instances[index] = nullptr;
  }
}

AutoClass(SocketsIterator,
  Object, object,
  uint, index)
  SocketsIterator() = default;
};

static Function const Object_Plug_Registration = Function_Bind(
  "Object_Plug",
  "Attempt to plug 'item' into a free socket of 'object', return success",
  [](Object const& object, Item const& item) -> bool
  {
  return object->Plug(item);
  },
  "object", "item");
static int const Object_Plug_Alias = Function_Alias("Object_Plug", "Plug");

static Function const Object_GetSockets_Registration = Function_Bind(
  "Object_GetSockets",
  "Return an iterator to the sockets of 'object'",
  [](Object const& object) -> SocketsIterator
  {
  return SocketsIterator(object, 0);
  },
  "object");
static int const Object_GetSockets_Alias = Function_Alias("Object_GetSockets", "GetSockets");

static Function const SocketsIterator_Access_Registration = Function_Bind(
  "SocketsIterator_Access",
  "Return the contents of 'iterator'",
  [](SocketsIterator const& iterator) -> Object
  {
  return iterator.object->GetSockets()->instances[iterator.index];
  },
  "iterator");
static int const SocketsIterator_Access_Alias = Function_Alias("SocketsIterator_Access", "Get");

static Function const SocketsIterator_Advance_Registration = Function_Bind(
  "SocketsIterator_Advance",
  "Advance 'iterator'",
  [](SocketsIterator const& iterator)
  {
  Mutable(iterator).index++;
  ComponentSockets* sockets = iterator.object->GetSockets();
  while (
      iterator.index < sockets->instances.size() &&
      !sockets->instances[iterator.index])
    Mutable(iterator).index++;
  },
  "iterator");
static int const SocketsIterator_Advance_Alias = Function_Alias("SocketsIterator_Advance", "Advance");

static Function const SocketsIterator_HasMore_Registration = Function_Bind(
  "SocketsIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](SocketsIterator const& iterator) -> bool
  {
  return
    iterator.object->GetSockets() &&
    iterator.index < iterator.object->GetSockets()->instances.size();
  },
  "iterator");
static int const SocketsIterator_HasMore_Alias = Function_Alias("SocketsIterator_HasMore", "HasMore");
