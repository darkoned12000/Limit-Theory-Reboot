#include "../Tasks.h"

#include "Component/Resources.h"

#include "Game/Object.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/StackFrame.h"
#include "LTE/Smooth.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(TaskDrill, TaskT,
    Task_Drill_Args, args)
    DERIVED_TYPE_EX(TaskDrill)
    POOLED_TYPE

    TaskDrill() = default;

    float GetDuration() const override {
      return 1.0f;
    }

    String GetName() const override {
      return "Drill";
    }

    String GetNoun() const override {
      return "Drilling Rig";
    }

    void GetOutputs(Vector<ItemDelta>& outputs) const override {
      Pointer<ComponentResources> resources = args.target->GetResources();
      for (size_t i = 0; i < resources->elements.size(); ++i)
        outputs.push(ItemDelta(resources->elements.entries[i].key, nullptr, 1));
    }

    Capability GetRateFactor() const override {
      return Capability_Transference(1);
    }

    void OnUpdate(Object const& self, float dt, Data& data) override { AUTO_FRAME;
      if (RandExp() < dt)
        self->AddItem(args.target->GetResources()->elements.sample(Rand()), 1);
    }
  };
}

Task Task_Drill(Task_Drill_Args const& args) {
  return new TaskDrill(args);
}
static Function const Task_Drill_Registration = Function_Bind(
  "Task_Drill",
  "None",
  [](Object const& target) -> Task { return Task_Drill(target); },
  "target");


