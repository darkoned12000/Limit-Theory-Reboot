#include "../Tasks.h"

#include "Component/MotionControl.h"

#include "Game/Object.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/SDFs.h"
#include "LTE/StackFrame.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(TaskPatrolInstance,
    Position, target)

    TaskPatrolInstance() = default;
  };

  AutoClassDerived(TaskPatrol, TaskT,
    Task_Patrol_Args, args)
    DERIVED_TYPE_EX(TaskPatrol)
    POOLED_TYPE

    TaskPatrol() = default;

    String GetName() const override {
      return "Patrol";
    }

    String GetNoun() const override {
      return "Patrol Unit";
    }

    Capability GetRateFactor() const override {
      return Capability_Attack(1);
    }

    void OnBegin(Object const& self, Data& data) override {
      TaskPatrolInstance it;
      it.target = self->GetPos();
      data = it;
    }

    void OnUpdate(Object const& self, float dt, Data& data) override { AUTO_FRAME;
      TaskPatrolInstance& it = data.Convert<TaskPatrolInstance>();

      if (Length(self->GetPos() - it.target) < 512.0f)
        it.target = args.zone->GetPos() + SampleSphere() * args.zone->GetScale();

      // self->Broadcast(MessageBoost());
      self->GetMotionControl()->elements.push(SDF_Sphere(it.target, 1.0f));
    }
  };
}

Task Task_Patrol(Task_Patrol_Args const& args) {
  return new TaskPatrol(args);
}
static Function const Task_Patrol_Registration = Function_Bind(
  "Task_Patrol",
  "None",
  [](Object const& zone) -> Task { return Task_Patrol(zone); },
  "zone");


