#include "Mission.h"

#include "Game/Object.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(MissionConstraintRange, MissionConstraintT,
    ItemProperty, property,
    Data, lower,
    Data, upper)
    DERIVED_TYPE_EX(MissionConstraintRange)
    MissionConstraintRange() = default;

    double Evaluate(Item const& data) const override {
      return 1.0;
    }

    String GetDescription() const override {
      return "";
    }
  };

  AutoClassDerived(MissionConstraintEquality, MissionConstraintT,
    ItemProperty, property,
    Data, value)
    DERIVED_TYPE_EX(MissionConstraintRange)
    MissionConstraintEquality() = default;

    double Evaluate(Item const& data) const override {
      return 1.0;
    }

    String GetDescription() const override {
      return Stringize() | property->GetName() | " is " | (*(Object*)value.data)->GetName();
    }
  };
}

MissionConstraint MissionConstraint_Equality(
  ItemProperty const& property,
  Data const& value)
{
  return new MissionConstraintEquality(property, value);
}

Mission Mission_Create(Object const& owner) {
  Mission self = new MissionT;
  self->owner = owner;
  return self;
}
static Function const Mission_Create_Registration = Function_Bind(
  "Mission_Create",
  "None",
  &Mission_Create,
  "owner");
static int const Mission_Create_Alias = Function_Alias("Mission_Create", "Mission");


