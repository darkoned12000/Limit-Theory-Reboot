#include "../Tasks.h"

#include "Game/Object.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/StackFrame.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(TaskSpawn, TaskT, Task_Spawn_Args, args)
    DERIVED_TYPE_EX(TaskSpawn)
    POOLED_TYPE

    TaskSpawn() = default;

    float GetDuration() const override {
      return args.rate;
    }

    String GetName() const override {
      return "Spawn";
    }

    String GetNoun() const override {
      return "Spawner";
    }

    void GetOutput(Vector<ItemDelta>& outputs) const {
      outputs.push(ItemDelta(args.item, nullptr, 1));
    }

    void OnUpdate(Object const& self, float dt, Data& data) override { AUTO_FRAME;
      /* TODO */
#if 0
      while (args.rate * RandExp() < dt)
        self->GetRoot()->AddItem(args.item, 1);
#endif
    }
  };
}

Task Task_Spawn(Task_Spawn_Args const& args) {
  return new TaskSpawn(args);
}
static Function const Task_Spawn_Registration = Function_Bind(
  "Task_Spawn",
  "None",
  [](Item const& item, float const& rate) -> Task { return Task_Spawn(item, rate); },
  "item", "rate");


