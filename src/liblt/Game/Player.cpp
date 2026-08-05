#include "Player.h"

#include "Component/Asset.h"
#include "Component/Pilotable.h"

#include "Game/Tasks.h"
#include "LTE/FunctionBind.h"

AutoClassDerived(PlayerImpl, PlayerT,
  bool, isHuman)
  DERIVED_TYPE_EX(PlayerImpl)

  PlayerImpl() = default;

  Health GetHealth() const override {
    return 100;
  }

  Icon GetIcon() const override {
    Icon icon;
    ScriptFunction_Load("Icons:Person")->Call(icon);
    return icon;
  }

  Health GetMaxHealth() const override {
    return 100;
  }

  bool IsHuman() const override {
    return isHuman;
  }

  void OnAttacked(Player const& attacker) override {
    ModOpinion(attacker, -1.0f);
  }

  void Pilot(Object const& object) override {
    Unpilot();
    Pointer<ComponentPilotable> p = object->GetPilotable();
    LTE_ASSERT(!p->pilot);
    p->pilot = this;
    piloting = object;
    container = object;
  }

  void Unpilot() override {
    if (piloting) {
      Pointer<ComponentPilotable> p = piloting->GetPilotable();
      LTE_ASSERT(p);
      container = nullptr;
      p->pilot = nullptr;
      piloting = nullptr;
    }
  }
};

DERIVED_IMPLEMENT(PlayerImpl)

Player Player_AI(Traits const& traits) {
  Reference<PlayerImpl> self = new PlayerImpl(false);
  self->traits = traits;
  return self;
}
static Function const Player_AI_Registration = Function_Bind(
  "Player_AI",
  "None",
  &Player_AI,
  "traits");



Player Player_Human() {
  return new PlayerImpl(true);
}
static Function const Player_Human_Registration = Function_Bind(
  "Player_Human",
  "None",
  &Player_Human);


