#ifndef Component_Info_h__
#define Component_Info_h__

#include "Common.h"
#include "LTE/AutoClass.h"
#include "LTE/Map.h"

AutoClass(InfoEntry,
  InfoLevelT, level,
  Time, expiration)
  InfoEntry() = default;
};

using InfoMapT = Map<ObjectID, InfoEntry>;

AutoClass(ComponentInfo,
  InfoMapT, elements)

  ComponentInfo() = default;

  bool HasLevel(ObjectT* object, InfoLevelT level) const {
    return GetLevel(object) >= level;
  }

  LT_API void Add(ObjectT* object, InfoLevelT level);
  LT_API InfoLevelT GetLevel(ObjectT* object) const;
};

AutoComponent(Info)
  void OnUpdate(UpdateState& s) override {
    BaseT::OnUpdate(s);
  }
};

#endif
