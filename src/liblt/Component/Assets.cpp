// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Assets.h"
#include "Asset.h"

#include "LTE/Function.h"

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

VoidFreeFunction(Object_AddAsset,
  "Transfer ownership of 'asset' to 'object'",
  Object, object,
  Object, asset)
{
  object->AddAsset(asset);
} FunctionAlias(Object_AddAsset, AddAsset);

VoidFreeFunction(Object_ClearAssets,
  "Release ownership of all assets owned by 'object'",
  Object, object)
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
} FunctionAlias(Object_ClearAssets, ClearAssets);

AutoClass(AssetsIterator,
  Object, object,
  uint, index)
  AssetsIterator() = default;
};

FreeFunction(AssetsIterator, Object_GetAssets,
  "Return an iterator to the assets owned by 'object'",
  Object, object)
{
  return AssetsIterator(object, 0);
} FunctionAlias(Object_GetAssets, GetAssets);

FreeFunction(Object, AssetsIterator_Access,
  "Return the contents of 'iterator'",
  AssetsIterator, iterator)
{
  return iterator.object->GetAssets()->elements[iterator.index];
} FunctionAlias(AssetsIterator_Access, Get);

VoidFreeFunction(AssetsIterator_Advance,
  "Advance 'iterator'",
  AssetsIterator, iterator)
{
  Mutable(iterator).index++;
} FunctionAlias(AssetsIterator_Advance, Advance);

FreeFunction(bool, AssetsIterator_HasMore,
  "Return whether 'iterator' has more elements",
  AssetsIterator, iterator)
{
  return 
    iterator.object->GetAssets() &&
    iterator.index < iterator.object->GetAssets()->elements.size();
} FunctionAlias(AssetsIterator_HasMore, HasMore);

FreeFunction(int, AssetsIterator_Size,
  "Return the total number of elements in 'iterator'",
  AssetsIterator, iterator)
{
  return static_cast<int>(iterator.object->GetAssets()->elements.size());
} FunctionAlias(AssetsIterator_Size, Size);
