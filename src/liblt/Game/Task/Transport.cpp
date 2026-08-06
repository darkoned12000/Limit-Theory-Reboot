#include "../Tasks.h"

#include "Game/Events.h"
#include "Game/Object.h"
#include "LTE/Pool.h"
#include "LTE/StackFrame.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(TaskTransport, TaskT, Task_Transport_Args, args)
    DERIVED_TYPE_EX(TaskTransport)
    POOLED_TYPE

    TaskTransport() = default;

    /* TODO : Estimate of travel time. */
    float GetDuration() const override {
      return Length(args.source->GetPos() - args.dest->GetPos());
    }

    String GetName() const override {
      return "Transport";
    }

    String GetNoun() const override {
      return "Transport";
    }

    void GetInputs(Vector<ItemDelta>& inputs) const override {
      inputs.push(ItemDelta(args.item, args.source, 1));
    }

    void GetOutputs(Vector<ItemDelta>& outputs) const override {
      outputs.push(ItemDelta(args.item, args.dest, 1));
    }

    Capability GetRateFactor() const override {
      return Capability_Motion(1);
    }

    Capability GetScaleFactor() const override {
      return Capability_Storage(1);
    }

    void OnUpdate(Object const& self, float dt, Data& data) override { AUTO_FRAME;
      if (self->GetContainer() == args.source) {
        // Quantity quantity =
        //  (Quantity)Floor(self->GetFreeCapacity() / args.item->GetMass());
        self->PushTask(Task_Dock(args.dest));
      }

      else if (self->GetContainer() == args.dest) {
        // Event_Deposit(self, args.dest, args.item, self->GetItemCount(args.item));
        self->PushTask(Task_Dock(args.source));
      }

      else
        self->PushTask(Task_Dock(args.source));
    }
  };
}

Task Task_Transport(Task_Transport_Args const& args) {
  return new TaskTransport(args);
}
static Function const Task_Transport_Registration = Function_Bind(
  "Task_Transport",
  "None",
  [](Object const& source, Object const& dest, Item const& item) -> Task { return Task_Transport(source, dest, item); },
  "source", "dest", "item");


