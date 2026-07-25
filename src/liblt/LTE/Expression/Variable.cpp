#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/Script.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionVariable, ExpressionT,
    uint, index,
    uint, offset,
    Type, type,
    String, name)
    DERIVED_TYPE_EX(ExpressionVariable)
    POOLED_TYPE

    ExpressionVariable() = default;

    String Emit(Vector<String>& context) const override {
      return name;
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      type->Assign((char*)env.registers[env.base + index] + offset, returnValue);
    }

    void* GetLValue(Environment& env) const override {
      return (char*)env.registers[env.base + index] + offset;
    }

    Type GetType() const override {
      return type;
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return env.Get(name).constant;
    }

    bool IsLValue() const override {
      return true;
    }
  };
}

namespace LTE {
  Expression Expression_Variable(
    uint index,
    Type const& type,
    String const& name)
  {
    return new ExpressionVariable(index, 0, type, name);
  }

  Expression Expression_Variable(
    StringList const& list,
    CompileEnvironment& env)
  {
    String const& name = list->GetValue();
    if (env.Contains(name)) {
      Variable const& var = env.Get(name);
      if (var.reference)
        return nullptr;

      return var.offset < 0
        ? Expression_Variable(var.registerIndex, var.type, var.name)
        : new ExpressionVariable(var.registerIndex, var.offset, var.type, var.name);
    }

    /* Variable not found — try to suggest a similar name. */
    Vector<String> candidates;
    env.CollectVariableNames(candidates);
    if (env.script) {
      for (auto it = env.script->functions.begin(); it != env.script->functions.end(); ++it)
        candidates.push(it->first);
    }
    String suggestion = BestMatch(name, candidates);
    if (suggestion.size() > 0)
      env.ReportError(list, Stringize()
        | "unknown variable '" | name
        | "' (did you mean '" | suggestion | "'?)");
    else
      env.ReportError(list, Stringize()
        | "unknown variable '" | name | "'");
    return nullptr;
  }
}
