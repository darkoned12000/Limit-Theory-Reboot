#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionReference, ExpressionT,
    uint, index,
    Type, type,
    String, name)
    DERIVED_TYPE_EX(ExpressionReference)
    POOLED_TYPE

    ExpressionReference() = default;

    String Emit(Vector<String>& context) const override {
      return name;
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      type->Assign(env.registers[env.base + index], returnValue);
    }

    void* GetLValue(Environment& env) const override {
      return env.registers[env.base + index];
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
  Expression Expression_Reference(
    uint index,
    Type const& type,
    String const& name)
  {
    return new ExpressionReference(index, type, name);
  }

  Expression Expression_Reference(
    StringList const& list,
    CompileEnvironment& env)
  {
    String const& name = list->GetValue();
    if (env.Contains(name)) {
      Variable const& var = env.Get(name);
      if (!var.reference)
        return nullptr;
      return Expression_Reference(var.registerIndex, var.type, var.name);
    }

    /* Reference not found — try to suggest a similar name. */
    Vector<String> candidates;
    env.CollectVariableNames(candidates);
    String suggestion = BestMatch(name, candidates);
    if (suggestion.size() > 0)
      env.ReportError(list, Stringize()
        | "unknown reference '" | name
        | "' (did you mean '" | suggestion | "'?)");
    else
      env.ReportError(list, Stringize()
        | "unknown reference '" | name | "'");
    return nullptr;
  }
}
