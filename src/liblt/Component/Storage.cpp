#include "Storage.h"

#include "Component/Cargo.h"

#include "Game/Objects.h"
#include "LTE/FunctionBind.h"

Object ComponentStorage::Get(Object const& owner) {
  Object& locker = entries[owner];
  if (!locker) {
    locker = Object_Pod(100000);
    locker->SetName(owner->GetName() + "'s Locker");
  }
  return locker;
}

static Function const Object_GetStorageLocker_Registration = Function_Bind(
  "Object_GetStorageLocker",
  "Return 'owner's storage locker at 'object'",
  [](Object const& object, Object const& owner) -> Object
  {
  return object->GetStorage()
    ? object->GetStorageLocker(owner)
    : nullptr;
  },
  "object", "owner");
static int const Object_GetStorageLocker_Alias = Function_Alias("Object_GetStorageLocker", "GetStorageLocker");
