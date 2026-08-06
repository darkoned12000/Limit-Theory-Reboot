#include "../Tasks.h"

#include "Component/MotionControl.h"
#include "Component/Sockets.h"

#include "Game/Conditions.h"
#include "Game/Icons.h"
#include "Game/Items.h"
#include "Game/Messages.h"
#include "Game/Object.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/SDFs.h"
#include "LTE/StackFrame.h"

#include "LTE/Debug.h"
#include "LTE/FunctionBind.h"
const Distance kDestroyDistance = 10000;

namespace {
  AutoClass(TaskDestroyInstance,
    Object, target,
    float, t)

    TaskDestroyInstance() :
      t(0)
      {}
  };

  AutoClassDerived(TaskDestroy, TaskT,
    Task_Destroy_Args, args)
    DERIVED_TYPE_EX(TaskDestroy)
    POOLED_TYPE

    TaskDestroy() = default;

    float GetAlignment() const override {
      return -1;
    }

    /* TODO */
    float GetDuration() const override {
      return 1.0f;
    }

    Icon GetIcon() const override {
      return Icon_Crosshair();
    }

    String GetName() const override {
      return "Destroy";
    }

    String GetNoun() const override {
      return "Raider";
    }

    void GetOutputs(Vector<ItemDelta>& outputs) const override {
      outputs.push(ItemDelta(
        args.target->GetDataDamaged(),
        nullptr,
        (Quantity)args.target->GetHealth()));

      outputs.push(ItemDelta(args.target->GetDataDestroyed(), nullptr, 1));
    }

    Capability GetRateFactor() const override {
      return Capability_Attack(1);
    }

    Object GetTarget() const override {
      return args.target;
    }

    bool IsFinished(Object const& self, Data const& data) const override {
      return !args.target->IsAlive();
    }

    void OnBegin(Object const& self, Data& data) override {
      data = TaskDestroyInstance();
    }

    void OnUpdate(Object const& self, float dt, Data& data) override { AUTO_FRAME;
      TaskDestroyInstance& it = data.Convert<TaskDestroyInstance>();
      it.t += dt;

      if (!Condition_Nearby(self, args.target, kDestroyDistance)) {
        self->PushTask(Task_Goto(args.target, kDestroyDistance));
        return;
      }

      float radius = 1000 + args.target->GetRadius() + self->GetRadius();
      Position target = args.target->GetCenter();
      float angle = kTau * (it.t / 8.0);
      V3D targetPos = target + radius * V3D(Cos(angle), 0, Sin(angle));
      self->GetMotionControl()->elements.push(SDF_Sphere(targetPos, 1));

      if (self->GetOwner() && args.target->GetOwner())
        self->GetOwner()->ModOpinion(args.target->GetOwner(), -1);

      Pointer<ComponentSockets> sockets = self->GetSockets();
      for (size_t i = 0; i < sockets->instances.size(); ++i) {
        Object const& o = sockets->instances[i];
        if (!o || o->GetCapability().Attack <= 0)
          continue;

        Position center = o->GetCenter();
        Item const& weapon = o->GetSockets()->instances[0]->GetSupertype();
        float speed = weapon->GetSpeed();
        float range = weapon->GetRange();
        if (Length(center - target) > range)
          continue;

        Position hitPoint;
        float t = ComputeImpact(
          center,
          target,
          self->GetVelocity(),
          args.target->GetVelocity(),
          speed,
          &hitPoint);

        if (t >= 0) {
          o->Broadcast(MessageTargetPosition(hitPoint));
          o->Broadcast(MessageTargetObject(args.target));
          o->Broadcast(MessageFire());
        }
      }
    }
  };
}

Task Task_Destroy(Task_Destroy_Args const& args) {
  return new TaskDestroy(args);
}
static Function const Task_Destroy_Registration = Function_Bind(
  "Task_Destroy",
  "None",
  [](Object const& target) -> Task { return Task_Destroy(target); },
  "target");



