#ifndef Component_Storage_h__
#define Component_Storage_h__

#include "Common.h"
#include "Game/Player.h"
#include "LTE/Map.h"

using StorageMapT = Map<Object, Object>;

AutoClass(ComponentStorage,
  StorageMapT, entries)

  ComponentStorage() = default;

  LT_API Object Get(Object const& owner);
};

AutoComponent(Storage)
  Object GetStorageLocker(Object const& owner) override {
    return Storage.Get(owner);
  }
};

#endif
