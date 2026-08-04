// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
// Substantial modification: added Object_SetLook_Position overload (Position/Vec3d).

#include "Orientation.h"
#include "Game/Object.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Math.h"
#include "LTE/Matrix.h"

void ComponentOrientation::Rotate(V3 const& rotation) {
  RotateBasis(transform.right, transform.up, transform.look, rotation);
  version++;
}

static Function const Object_GetLook_Registration = Function_Bind(
  "Object_GetLook",
  "Return the look direction of 'object'",
  [](Object const& object) -> V3
  {
  return object->GetLook();
  },
  "object");
static int const Object_GetLook_Alias = Function_Alias("Object_GetLook", "GetLook");

static Function const Object_GetPos_Registration = Function_Bind(
  "Object_GetPos",
  "Return the position of 'object'",
  [](Object const& object) -> Position
  {
  return object->GetPos();
  },
  "object");
static int const Object_GetPos_Alias = Function_Alias("Object_GetPos", "GetPos");

static Function const Object_GetRight_Registration = Function_Bind(
  "Object_GetRight",
  "Return the right direction of 'object'",
  [](Object const& object) -> V3
  {
  return object->GetRight();
  },
  "object");
static int const Object_GetRight_Alias = Function_Alias("Object_GetRight", "GetRight");

static Function const Object_GetScale_Registration = Function_Bind(
  "Object_GetScale",
  "Return the axial scaling of 'object'",
  [](Object const& object) -> V3
  {
  return object->GetScale();
  },
  "object");
static int const Object_GetScale_Alias = Function_Alias("Object_GetScale", "GetScale");

static Function const Object_GetTransform_Registration = Function_Bind(
  "Object_GetTransform",
  "Return the global transform of 'object'",
  [](Object const& object) -> Transform
  {
  return object->GetTransform();
  },
  "object");
static int const Object_GetTransform_Alias = Function_Alias("Object_GetTransform", "GetTransform");

static Function const Object_GetUp_Registration = Function_Bind(
  "Object_GetUp",
  "Return the up direction of 'object'",
  [](Object const& object) -> V3
  {
  return object->GetUp();
  },
  "object");
static int const Object_GetUp_Alias = Function_Alias("Object_GetUp", "GetUp");

static Function const Object_SetLook_Registration = Function_Bind(
  "Object_SetLook",
  "Orient 'object' to face towards 'look' direction",
  [](Object const& object, V3 const& look)
  {
  object->SetLook(look);
  },
  "object", "look");
static int const Object_SetLook_Alias = Function_Alias("Object_SetLook", "SetLook");

/* Overload of SetLook for a double-precision Position direction (see
 * AGENTS.md §8d #1). Added as a function overload rather than a V3D -> V3F
 * conversion because the latter corrupts the engine's global conversion table. */
static Function const Object_SetLook_Position_Registration = Function_Bind(
  "Object_SetLook_Position",
  "Orient 'object' to face towards 'look' direction (Position)",
  [](Object const& object, V3D const& look)
  {
  object->SetLook(V3(look));
  },
  "object", "look");
static int const Object_SetLook_Position_Alias = Function_Alias("Object_SetLook_Position", "SetLook");

static Function const Object_SetPos_Registration = Function_Bind(
  "Object_SetPos",
  "Move 'object' to 'position'",
  [](Object const& object, Position const& position)
  {
  object->SetPos(position);
  },
  "object", "position");
static int const Object_SetPos_Alias = Function_Alias("Object_SetPos", "SetPos");

static Function const Object_SetScale_Registration = Function_Bind(
  "Object_SetScale",
  "Set the axial scaling of 'object' to 'scale'",
  [](Object const& object, V3 const& scale)
  {
  object->SetScale(scale);
  },
  "object", "scale");
static int const Object_SetScale_Alias = Function_Alias("Object_SetScale", "SetScale");
