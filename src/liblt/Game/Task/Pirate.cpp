#include "../Tasks.h"

#include "Component/Cargo.h"
#include "Component/Interior.h"
#include "Component/Resources.h"

#include "Game/Conditions.h"
#include "Game/Events.h"
#include "Game/Icons.h"
#include "Game/Object.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/StackFrame.h"

#include "LTE/Debug.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(TaskPirateInstance,
    Object, target)

    TaskPirateInstance() = default;
  };

  AutoClassDerived(TaskPirate, TaskT,
    Task_Pirate_Args, args)
    DERIVED_TYPE_EX(TaskPirate)
    POOLED_TYPE

    TaskPirate() = default;

    bool CanAttack(Object const& self, Object const& target) const {
      return
        target->GetCargo() &&
        //target->GetCargo()->currentMass > 0 &&
        target->IsAlive() &&
        self->GetOwner() != target->GetOwner() &&
        !(target->GetOwner() && target->GetOwner()->IsHuman()); // TEMPCHANGE
    }

    float GetAlignment() const override {
      return -1;
    }

    /* TODO */
    float GetDuration() const override {
      return 1;
    }

    Icon GetIcon() const override {
      return Icon_Crosshair();
    }

    String GetName() const override {
      return "Pirate";
    }

    String GetNoun() const override {
      return "Raider";
    }

    void GetOutput(Vector<ItemDelta>& outputs) const {
      Pointer<ComponentResources> resources = args.zone->GetResources();
      for (size_t i = 0; i < resources->elements.size(); ++i)
        outputs.push(ItemDelta(resources->elements.entries[i].key, nullptr, 1));
    }

    Capability GetRateFactor() const override {
      return Capability_Attack(1);
    }

    void OnBegin(Object const& self, Data& data) override {
      data = TaskPirateInstance();
    }

    void OnUpdate(Object const& self, float dt, Data& data) override { AUTO_FRAME;
      TaskPirateInstance& it = data.Convert<TaskPirateInstance>();

      if (it.target && CanAttack(self, it.target))
        self->PushTask(Task_Destroy(it.target));

      /* TODO : Better target selection. */
      else {
        if (!it.target) {
          ComponentInterior* interior = args.zone->GetContainer()->GetInterior();
          it.target = interior->objects[RandI(interior->objects.size())];
        }
      }
    }
  };
}

Task Task_Pirate(Task_Pirate_Args const& args) {
  return new TaskPirate(args);
}
static Function const Task_Pirate_Registration = Function_Bind(
  "Task_Pirate",
  "None",
  [](Object const& zone) -> Task { return Task_Pirate(zone); },
  "zone");


