// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Assets.h"
#include "Asset.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

ComponentAssets::~ComponentAssets() {
  for (size_t i = 0; i < elements.size(); ++i) {
    Object const& o = elements[i];
    if (o) {
      Pointer<ComponentAsset> as = o->GetAsset();
      if (as)
        as->owner = nullptr;
    }
  }
}

void ComponentAssets::Add(ObjectT* self, Object const& asset) {
  Pointer<ComponentAsset> as = asset->GetAsset();
  LTE_ASSERT(!as->owner);
  as->owner = self;
  elements.push(asset);
}

void ComponentAssets::Remove(ObjectT* self, Object const& asset) {
  Pointer<ComponentAsset> as = asset->GetAsset();
  LTE_ASSERT(as->owner.t == self);
  as->owner = nullptr;
  elements.remove(asset);
}

static Function const Object_AddAsset_Registration = Function_Bind(
  "Object_AddAsset",
  "Transfer ownership of 'asset' to 'object'",
  [](Object const& object, Object const& asset)
  {
  object->AddAsset(asset);
  },
  "object", "asset");
static int const Object_AddAsset_Alias = Function_Alias("Object_AddAsset", "AddAsset");

static Function const Object_ClearAssets_Registration = Function_Bind(
  "Object_ClearAssets",
  "Release ownership of all assets owned by 'object'",
  [](Object const& object)
  {
  Pointer<ComponentAssets> assets = object->GetAssets();
  if (!assets)
    return;
  for (size_t i = 0; i < assets->elements.size(); ++i) {
    Object const& o = assets->elements[i];
    if (o) {
      Pointer<ComponentAsset> as = o->GetAsset();
      if (as)
        as->owner = nullptr;
    }
  }
  assets->elements.clear();
  },
  "object");
static int const Object_ClearAssets_Alias = Function_Alias("Object_ClearAssets", "ClearAssets");

AutoClass(AssetsIterator,
  Object, object,
  uint, index)
  AssetsIterator() = default;
};

static Function const Object_GetAssets_Registration = Function_Bind(
  "Object_GetAssets",
  "Return an iterator to the assets owned by 'object'",
  [](Object const& object) -> AssetsIterator
  {
  return AssetsIterator(object, 0);
  },
  "object");
static int const Object_GetAssets_Alias = Function_Alias("Object_GetAssets", "GetAssets");

static Function const AssetsIterator_Access_Registration = Function_Bind(
  "AssetsIterator_Access",
  "Return the contents of 'iterator'",
  [](AssetsIterator const& iterator) -> Object
  {
  return iterator.object->GetAssets()->elements[iterator.index];
  },
  "iterator");
static int const AssetsIterator_Access_Alias = Function_Alias("AssetsIterator_Access", "Get");

static Function const AssetsIterator_Advance_Registration = Function_Bind(
  "AssetsIterator_Advance",
  "Advance 'iterator'",
  [](AssetsIterator const& iterator)
  {
  Mutable(iterator).index++;
  },
  "iterator");
static int const AssetsIterator_Advance_Alias = Function_Alias("AssetsIterator_Advance", "Advance");

static Function const AssetsIterator_HasMore_Registration = Function_Bind(
  "AssetsIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](AssetsIterator const& iterator) -> bool
  {
  return 
    iterator.object->GetAssets() &&
    iterator.index < iterator.object->GetAssets()->elements.size();
  },
  "iterator");
static int const AssetsIterator_HasMore_Alias = Function_Alias("AssetsIterator_HasMore", "HasMore");

static Function const AssetsIterator_Size_Registration = Function_Bind(
  "AssetsIterator_Size",
  "Return the total number of elements in 'iterator'",
  [](AssetsIterator const& iterator) -> int
  {
  return static_cast<int>(iterator.object->GetAssets()->elements.size());
  },
  "iterator");
static int const AssetsIterator_Size_Alias = Function_Alias("AssetsIterator_Size", "Size");
