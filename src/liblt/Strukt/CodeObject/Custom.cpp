#include "../CodeObjects.h"

#include "LTE/Pool.h"
#include "LTE/Script.h"

#include "UI/Widget.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(CodeObjectCustom, CodeObjectT,
    Data, instance,
    ScriptFunction, getName,
    ScriptFunction, getType,
    ScriptFunction, getWidget)
    DERIVED_TYPE_EX(CodeObjectCustom)
    POOLED_TYPE

    CodeObjectCustom() = default;

    String GetName() const override {
      return "Custom Code Object";
    }

    String GetType() const override {
      return "unknown";
    }

    Widget GetWidget() const override {
      return nullptr;
    }
  };
}

CodeObject CodeObject_Custom(Data const& data) {
  Reference<CodeObjectCustom> self = new CodeObjectCustom;
  return self;
}
static Function const CodeObject_Custom_Registration = Function_Bind(
  "CodeObject_Custom",
  "None",
  &CodeObject_Custom,
  "data");


