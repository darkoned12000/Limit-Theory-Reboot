#include "Static.h"
#include "Map.h"
#include "String.h"

using StaticMap = Map<String, Data>;

namespace {
  StaticMap& GetMap() {
    static StaticMap map;
    return map;
  }
}

Data& Static_Get(String const& name) {
  return GetMap()[name];
}
