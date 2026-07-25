#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionArray, ExpressionT,
    Type, type)
    DERIVED_TYPE_EX(ExpressionArray)
    POOLED_TYPE

    ExpressionArray() = default;

    void Evaluate(void* returnValue, Environment& env) const override {
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
  Expression Expression_Array(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() != 2) {
      env.ReportError(list, "'array' expects 1 argument (element type)");
      return nullptr;
    }

    Type subType;
    Type subTypePtr;
    return nullptr;
  }
}
