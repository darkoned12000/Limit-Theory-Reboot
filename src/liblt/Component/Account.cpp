// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Account.h"
#include "Game/Object.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static Function const Object_AddCredits_Registration = Function_Bind(
  "Object_AddCredits",
  "Transfer 'count' credits to 'object's bank account",
  [](Object const& object, Quantity const& count)
  {
  object->AddCredits(count);
  },
  "object", "count");
static int const Object_AddCredits_Alias = Function_Alias("Object_AddCredits", "AddCredits");

static Function const Object_GetCredits_Registration = Function_Bind(
  "Object_GetCredits",
  "Return the number of credits in 'object's bank account",
  [](Object const& object) -> Quantity
  {
  return object->GetCredits();
  },
  "object");
static int const Object_GetCredits_Alias = Function_Alias("Object_GetCredits", "GetCredits");

static Function const Object_RemoveCredits_Registration = Function_Bind(
  "Object_RemoveCredits",
  "Try to transfer 'count' credits from 'object's bank account; return success",
  [](Object const& object, Quantity const& count) -> bool
  {
  return object->RemoveCredits(count);
  },
  "object", "count");
static int const Object_RemoveCredits_Alias = Function_Alias("Object_RemoveCredits", "RemoveCredits");

static Function const Object_SetCredits_Registration = Function_Bind(
  "Object_SetCredits",
  "Set 'object's bank account to exactly 'count' credits",
  [](Object const& object, Quantity const& count)
  {
  object->SetCredits(count);
  },
  "object", "count");
static int const Object_SetCredits_Alias = Function_Alias("Object_SetCredits", "SetCredits");
