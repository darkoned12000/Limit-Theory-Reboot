#include "../Tasks.h"

#include "Game/Icons.h"
#include "Game/Object.h"

#include "LTE/Math.h"
#include "LTE/Pool.h"
#include "LTE/Script.h"
#include "LTE/StackFrame.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(TaskCustom, TaskT,
    Data, instance,
    ScriptFunction, begin,
    ScriptFunction, end,
    ScriptFunction, getIcon,
    ScriptFunction, getInputs,
    ScriptFunction, getName,
    ScriptFunction, getOutputs,
    ScriptFunction, update)
    DERIVED_TYPE_EX(TaskCustom)
    POOLED_TYPE

    TaskCustom() = default;

    Icon GetIcon() const override {
      if (!getIcon)
        return TaskT::GetIcon();

      Icon icon;
      getIcon->VoidCall(&icon, instance);
      return icon;
    }

    String GetName() const override {
      if (!getName)
        return "Custom Task";

      String name;
      getName->VoidCall(&name, instance);
      return name;
    }

    void OnBegin(Object const& self, Data& data) override {
      if (begin)
        begin->VoidCall(0, instance, self, (DataRef)data);
    }

    void OnEnd(Object const& self, Data& data) override {
      if (end)
        end->VoidCall(0, instance, self, (DataRef)data);
    }

    void OnUpdate(Object const& self, float dt, Data& data) override {
      if (update)
        update->VoidCall(0, instance, self, (DataRef)data);
    }
  };
}

Task Task_Custom(Data const& data) {
  Reference<TaskCustom> self = new TaskCustom;
  ScriptType type = data.type->GetAux().Convert<ScriptType>();
  self->instance = data;
  self->begin = type->GetFunction("Begin");
  self->end = type->GetFunction("End");
  self->getIcon = type->GetFunction("GetIcon");
  self->getName = type->GetFunction("GetName");
  self->update = type->GetFunction("Update");
  return self;
}
static Function const Task_Custom_Registration = Function_Bind(
  "Task_Custom",
  "None",
  &Task_Custom,
  "data");


