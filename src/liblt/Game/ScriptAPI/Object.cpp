// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
// Substantial modification: replaced static-init TypeAlias(Position, Position)
// with lazy dll-side Position_RegisterConstructor() binding "Position" -> V3D.

#include "Game/Common.h"
#include "Game/Object.h"
#include "Game/Player.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Renderable.h"
#include "LTE/Script.h"
#include "LTE/Transform.h"

#include "UI/Widget.h"

TypeAlias(Reference<ObjectT>, Object);
TypeAlias(Reference<PlayerT>, Player);
TypeAlias(HashT, HashT);

/* Position === V3D === V3T<double> (see Game/Common.h). We bind the script name
 * "Position" to the (fully operator-populated) Vec3d type.
 *
 * IMPORTANT (AGENTS.md §8d #1): binding Position via the static-init TypeAlias
 * macro (which calls Type_Get<Position>()) triggers the static-initialization-
 * order fiasco that corrupts the global registry (every numeric literal like
 * '0.5' starts failing with "variable name '0.5' not found"). The fix is lazy /
 * late registration. We use a dll-side __attribute__((constructor)) so this runs
 * at dll load, AFTER all dll static-init (so V3D is fully populated) and BEFORE
 * any script is compiled -- avoiding both the SIOF and re-entrancy with the
 * compiler. We reach the type via Type_Get<V3D>() (safe; V3.cpp resolves it
 * cleanly) and never call Type_Get<Position>(). */
static void Position_RegisterScriptAPI() {
  Type_AddAlias(Type_Get<V3D>(), "Position");
}

__attribute__((constructor))
static void Position_RegisterConstructor() {
  Position_RegisterScriptAPI();
}

static void object_to_player_Impl(Object const& src, Player& dest) {
  dest = (Player)src;
}
static int const object_to_player_Registration = Conversion_Bind<&object_to_player_Impl>();

static void player_to_object_Impl(Player const& src, Object& dest) {
  dest = (Object)src;
}
static int const player_to_object_Registration = Conversion_Bind<&player_to_object_Impl>();

/* Reference null-checks (IsNull / IsNotNull). These exist solely so an
   `Object` reference's null state is decidable from script without
   dispatching to the `Data`-typed overloads (Data.cpp binds Data_IsNull/
   Data_IsNotNull): with only the Data overloads available, an Object
   receiver (an 8-byte Reference block) matched the 8-byte Data param by
   layout and the binding read garbage. */
static Function const Object_IsNotNull_Registration = Function_Bind(
  "Object_IsNotNull",
  "Return whether 'object' refers to a valid instance",
  [](Object const& object) -> bool
  {
  return object.t != nullptr;
  },
  "object");
static int const Object_IsNotNull_Alias = Function_Alias("Object_IsNotNull", "IsNotNull");

static Function const Object_IsNull_Registration = Function_Bind(
  "Object_IsNull",
  "Return whether 'object' does not refer to an instance",
  [](Object const& object) -> bool
  {
  return object.t == nullptr;
  },
  "object");
static int const Object_IsNull_Alias = Function_Alias("Object_IsNull", "IsNull");

static Function const Object_AddChild_Registration = Function_Bind(
  "Object_AddChild",
  "Add 'child' to 'object'",
  [](Object const& object, Object const& child)
  {
  object->AddChild(child);
  },
  "object", "child");
static int const Object_AddChild_Alias = Function_Alias("Object_AddChild", "AddChild");

static Function const Object_Attach_Registration = Function_Bind(
  "Object_Attach",
  "Attach 'child' to 'object' with 'localTransform'",
  [](Object const& object, Object const& child, Transform const& localTransform)
  {
  object->Attach(child, localTransform);
  },
  "object", "child", "localTransform");
static int const Object_Attach_Alias = Function_Alias("Object_Attach", "Attach");

static Function const Object_Broadcast_Registration = Function_Bind(
  "Object_Broadcast",
  "Broadcast 'message' to 'object'",
  [](Object const& object, Data const& message)
  {
  object->Broadcast(Mutable(message));
  },
  "object", "message");
static int const Object_Broadcast_Alias = Function_Alias("Object_Broadcast", "Broadcast");

static Function const Object_Delete_Registration = Function_Bind(
  "Object_Delete",
  "Delete 'object' from the game world",
  [](Object const& object)
  {
  object->Delete();
  },
  "object");
static int const Object_Delete_Alias = Function_Alias("Object_Delete", "Delete");

static Function const Object_Equal_Registration = Function_Bind(
  "Object_Equal",
  "Return a == b",
  [](Object const& a, Object const& b) -> bool
  {
  return a == b;
  },
  "a", "b");
static int const Object_Equal_Alias = Function_Alias("Object_Equal", "==");

static Function const Object_GetContainer_Registration = Function_Bind(
  "Object_GetContainer",
  "Return the container within which 'object' exists",
  [](Object const& object) -> Object
  {
  return object->GetContainer().t;
  },
  "object");
static int const Object_GetContainer_Alias = Function_Alias("Object_GetContainer", "GetContainer");

static Function const Object_GetDistance_Registration = Function_Bind(
  "Object_GetDistance",
  "Return the distance from 'a' to 'b'",
  [](Object const& a, Object const& b) -> DistanceT
  {
  return Length(a->GetPos() - b->GetPos());
  },
  "a", "b");
static int const Object_GetDistance_Alias = Function_Alias("Object_GetDistance", "GetDistance");

static Function const Object_GetHash_Registration = Function_Bind(
  "Object_GetHash",
  "Return a hash for 'object'",
  [](Object const& object) -> HashT
  {
  return object->GetHash();
  },
  "object");
static int const Object_GetHash_Alias = Function_Alias("Object_GetHash", "GetHash");

static Function const Object_GetIcon_Registration = Function_Bind(
  "Object_GetIcon",
  "Return the icon for 'object'",
  [](Object const& object) -> Icon
  {
  return object->GetIcon();
  },
  "object");
static int const Object_GetIcon_Alias = Function_Alias("Object_GetIcon", "GetIcon");

static Function const Object_GetID_Registration = Function_Bind(
  "Object_GetID",
  "Return the globally-unique ID of 'object'",
  [](Object const& object) -> ObjectID
  {
  return object->GetID();
  },
  "object");
static int const Object_GetID_Alias = Function_Alias("Object_GetID", "GetID");

static Function const Object_GetRadius_Registration = Function_Bind(
  "Object_GetRadius",
  "Return the world-space radius of 'object'",
  [](Object const& object) -> float
  {
  return object->GetRadius();
  },
  "object");
static int const Object_GetRadius_Alias = Function_Alias("Object_GetRadius", "GetRadius");

static Function const Object_GetRenderable_Registration = Function_Bind(
  "Object_GetRenderable",
  "Return the renderable for 'object'",
  [](Object const& object) -> Renderable
  {
  return object->GetRenderable();
  },
  "object");
static int const Object_GetRenderable_Alias = Function_Alias("Object_GetRenderable", "GetRenderable");

static Function const Object_GetRoot_Registration = Function_Bind(
  "Object_GetRoot",
  "Return the root object of 'object'",
  [](Object const& object) -> Object
  {
  return object->GetRoot().t;
  },
  "object");
static int const Object_GetRoot_Alias = Function_Alias("Object_GetRoot", "GetRoot");

static Function const Object_GetSeed_Registration = Function_Bind(
  "Object_GetSeed",
  "Return the seed value of 'object'",
  [](Object const& object) -> uint
  {
  return object->GetSeed();
  },
  "object");
static int const Object_GetSeed_Alias = Function_Alias("Object_GetSeed", "GetSeed");

static Function const Object_GetSupertype_Registration = Function_Bind(
  "Object_GetSupertype",
  "Return the item supertype of 'object'",
  [](Object const& object) -> Item
  {
  return object->GetSupertype();
  },
  "object");
static int const Object_GetSupertype_Alias = Function_Alias("Object_GetSupertype", "GetSupertype");

static Function const Object_GetSystem_Registration = Function_Bind(
  "Object_GetSystem",
  "Return the system within which 'object' exists",
  [](Object const& object) -> Object
  {
  return (ObjectT*)object->GetSystem().t;
  },
  "object");
static int const Object_GetSystem_Alias = Function_Alias("Object_GetSystem", "GetSystem");

static Function const Object_GetTraits_Registration = Function_Bind(
  "Object_GetTraits",
  "Return the personality traits of 'object'",
  [](Object const& object) -> Traits
  {
  return object->GetTraits();
  },
  "object");
static int const Object_GetTraits_Alias = Function_Alias("Object_GetTraits", "GetTraits");

static Function const Object_GetType_Registration = Function_Bind(
  "Object_GetType",
  "Return the type of 'object'",
  [](Object const& object) -> String
  {
  return object->GetTypeString();
  },
  "object");
static int const Object_GetType_Alias = Function_Alias("Object_GetType", "GetType");

static Function const Object_GetWidget_Registration = Function_Bind(
  "Object_GetWidget",
  "Return the object-specific widget for 'object' from 'player's point-of-view",
  [](Object const& object, Player const& player) -> Widget
  {
  return object->GetWidget(player);
  },
  "object", "player");
static int const Object_GetWidget_Alias = Function_Alias("Object_GetWidget", "GetWidget");

static Function const Object_NotEqual_Registration = Function_Bind(
  "Object_NotEqual",
  "Return a != b",
  [](Object const& a, Object const& b) -> bool
  {
  return a != b;
  },
  "a", "b");
static int const Object_NotEqual_Alias = Function_Alias("Object_NotEqual", "!=");

static Function const Object_RemoveChild_Registration = Function_Bind(
  "Object_RemoveChild",
  "Remove 'child' from 'object'",
  [](Object const& object, Object const& child)
  {
  object->RemoveChild(child);
  },
  "object", "child");
static int const Object_RemoveChild_Alias = Function_Alias("Object_RemoveChild", "RemoveChild");

static Function const Object_Update_Registration = Function_Bind(
  "Object_Update",
  "Run one iteration of 'object's update logic, with time step 'dt'",
  [](Object const& object, float const& dt)
  {
  UpdateState state(dt, true);
  object->Update(state);
  },
  "object", "dt");
static int const Object_Update_Alias = Function_Alias("Object_Update", "Update");

static Function const Object_Send_Registration = Function_Bind(
  "Object_Send",
  "Send 'message' to 'object'",
  [](Object const& object, Data const& message)
  {
  object->Send(Mutable(message));
  },
  "object", "message");
static int const Object_Send_Alias = Function_Alias("Object_Send", "Send");

namespace Parent {
  AutoClass(ObjectChildIterator,
    Object, object,
    Object, child)
    ObjectChildIterator() = default;
  };

  static Function const Object_GetChildren_Registration = Function_Bind(
  "Object_GetChildren",
  "Return an iterator to the children of 'object'",
  [](Object const& object) -> ObjectChildIterator
  {
    return ObjectChildIterator(object, object->children);
  
  },
  "object");
static int const Object_GetChildren_Alias = Function_Alias("Object_GetChildren", "GetChildren");

  static Function const ObjectChildIterator_Advance_Registration = Function_Bind(
  "ObjectChildIterator_Advance",
  "Advance 'iterator'",
  [](ObjectChildIterator const& iterator)
  {
    Mutable(iterator).child = iterator.child->nextSibling;
  
  },
  "iterator");
static int const ObjectChildIterator_Advance_Alias = Function_Alias("ObjectChildIterator_Advance", "Advance");

  static Function const ObjectChildIterator_Get_Registration = Function_Bind(
  "ObjectChildIterator_Get",
  "Return the contents of 'iterator'",
  [](ObjectChildIterator const& iterator) -> Object
  {
    return iterator.child;
  
  },
  "iterator");
static int const ObjectChildIterator_Get_Alias = Function_Alias("ObjectChildIterator_Get", "Get");

  static Function const ObjectChildIterator_HasMore_Registration = Function_Bind(
  "ObjectChildIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](ObjectChildIterator const& iterator) -> bool
  {
    return iterator.child != nullptr;
  
  },
  "iterator");
static int const ObjectChildIterator_HasMore_Alias = Function_Alias("ObjectChildIterator_HasMore", "HasMore");
}

#define Z(x, y, z)                                                             \
  static Function const Object_Is##y##_Registration =                          \
    Function_Bind(                                                             \
      "Object_Is" #y,                                                          \
      "Return whether 'object' is a " z,                                       \
      [](Object const& object) -> bool                                         \
      { return object->GetType() == ObjectType_##y; },                         \
      "object");                                                               \
  static int const Object_Is##y##_Alias = Function_Alias(                      \
    "Object_Is" #y, "Is" #y);
OBJECT_X
#undef Z
