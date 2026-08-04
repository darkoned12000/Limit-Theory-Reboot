#ifndef Component_History_h__
#define Component_History_h__

#include "Common.h"

#include "Game/Event.h"

#include "LTE/AutoClass.h"
#include "LTE/Vector.h"

AutoClass(ComponentHistory,
  Vector<Event>, elements)

  ComponentHistory() = default;

  LT_API void Run(ObjectT*, UpdateState&);
};

AutoComponent(History)
  void OnUpdate(UpdateState& s) override {
    History.Run(this, s);
    BaseT::OnUpdate(s);
  }
};

LT_API void Object_AddHistory(Object const& object, Event const& event);

#endif
