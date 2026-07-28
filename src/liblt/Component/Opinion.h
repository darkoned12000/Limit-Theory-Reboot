#ifndef Component_Opinions_h__
#define Component_Opinions_h__

#include "Common.h"
#include "LTE/AutoClass.h"
#include "LTE/Map.h"

/* TODO : Memory issues, opinions are currently permanent. */

using OpinionMapT = Map<ObjectID, float>;

AutoClass(ComponentOpinions,
  OpinionMapT, elements,
  float, defaultOpinion)

  ComponentOpinions() :
    defaultOpinion(0)
    {}

  float Get(ObjectID id) const {
    float const* value = elements.get(id);
    return value ? *value : defaultOpinion;
  }

  void Mod(ObjectID id, float amount) {
    float& opinion = elements[id];
    opinion = Clamp(opinion + amount, -1.0f, 1.0f);
  }
};

AutoComponent(Opinions)
  float GetOpinion(Object const& o) const override {
    return Opinions.Get(o->GetID());
  }

  void ModOpinion(Object const& o, float amount) override {
    Opinions.Mod(o->GetID(), amount);
  }
};

#endif
