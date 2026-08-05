#include "../Tasks.h"

#include "Game/Events.h"
#include "Game/Icons.h"
#include "Game/Items.h"
#include "Game/Object.h"
#include "Game/Item/Blueprint.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/StackFrame.h"

#include "Module/SoundEngine.h"
#include "LTE/FunctionBind.h"

namespace {
  /* TODO : Output of research? */

  AutoClassDerived(TaskResearch, TaskT, Task_Research_Args, args)
    DERIVED_TYPE_EX(TaskResearch)
    POOLED_TYPE

    TaskResearch() = default;

    float GetDuration() const override {
      return args.blueprint->GetValue();
    }

    Icon GetIcon() const override {
      return Icon_Task_Research();
    }

    String GetName() const override {
      return "Research";
    }

    String GetNoun() const override {
      return "Research Lab";
    }

    Capability GetRateFactor() const override {
      return Capability_Research(1);
    }

    void OnUpdate(Object const& self, float dt, Data& data) override { AUTO_FRAME;
      if (10.0 * RandExp() < dt) {
        Item item = Item_Blueprint_Derived(args.blueprint.t);
        self->GetRoot()->AddItem(item, 1);
        Sound_Play3D("techlab/complete.ogg", self, 0, 1, 0.05f);

        Player const& owner = self->GetOwner();
        if (owner)
          owner->AddLogMessage("Discovered [" + item->GetName() + "]");
      }
    }
  };
}

Task Task_Research(Task_Research_Args const& args) {
  return new TaskResearch(args);
}
static Function const Task_Research_Registration = Function_Bind(
  "Task_Research",
  "None",
  [](Item const& blueprint) -> Task { return Task_Research(blueprint); },
  "blueprint");


