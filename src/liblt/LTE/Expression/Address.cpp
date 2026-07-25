
#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/StringList.h"
#include "LTE/Types.h"

namespace {
  AutoClassDerived(ExpressionAddress, ExpressionT,
    Expression, location,
    Type, type)
    DERIVED_TYPE_EX(ExpressionAddress)
    POOLED_TYPE

    ExpressionAddress() = default;

    ExpressionAddress(Expression const& location) :
      location(location),
      type(Type_Pointer(location->GetType()))
      {}

    String Emit(Vector<String>& scope) const override {
      String label = location->Emit(scope);
      return "(&" + label + ")";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      *(void**)returnValue = location->GetLValue(env);
    }

    Type GetType() const override {
      return type;
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return false;
    }
  };
}

namespace LTE {
  Expression Expression_Address(Expression const& location) {
    return new ExpressionAddress(location);
  }

  Expression Expression_Address(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() != 2) {
      env.ReportError(list, "'address' expects 1 argument (l-value)");
      return nullptr;
    }

    Expression location = Expression_Compile(list->Get(1), env);
    if (!location) {
      env.ReportError(list, "'address' -- expression failed to compile");
      return nullptr;
    }

    if (!location->IsLValue()) {
      env.ReportError(list, "'address' target must be an l-value (assignable location)");
      return nullptr;
    }

    return Expression_Address(location);
  }
}
