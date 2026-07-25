#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionIf, ExpressionT,
    Expression, predicate,
    Expression, statement,
    Type, statementType)
    DERIVED_TYPE_EX(ExpressionIf)
    POOLED_TYPE

    ExpressionIf() = default;

    String Emit(Vector<String>& context) const override {
      String predValue = predicate->Emit(context);
      context.push(Stringize() | "if (" | predValue | ") {");

      Vector<String> localContext;
      statement->Emit(localContext);
      for (size_t i = 0; i < localContext.size(); ++i)
        context.push("  " + localContext[i]);
      context.push("}");

      return "";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      bool pred;
      predicate->Evaluate(&pred, env);
      if (pred) {
        if (statementType->allocate) {
          void* lv = env.Allocate(statementType);
          statement->Evaluate(lv, env);
          env.Free(statementType, lv);
        } else {
          statement->Evaluate(0, env);
        }
      }
    }

    Type GetType() const override {
      return Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return predicate->IsConstant(env) && statement->IsConstant(env);
    }
  };
}

namespace LTE {
  Expression Expression_If(
    Expression const& predicate,
    Expression const& statement)
  {
    return new ExpressionIf(predicate, statement, statement->GetType());
  }

  Expression Expression_If(StringList const& list, CompileEnvironment& env) {
    if (list->GetSize() < 3) {
      env.ReportError(list, "'if' expects at least 2 arguments (predicate, body)");
      return nullptr;
    }

    Expression predicate = Expression_Compile(list->Get(1), env);
    if (!predicate) {
      env.ReportError(list, "'if' -- predicate expression failed to compile");
      return nullptr;
    }

    predicate = Expression_Conversion(predicate, Type_Get<bool>());
    if (!predicate) {
      env.ReportError(list, "'if' -- predicate must be convertible to 'bool'");
      return nullptr;
    }

    Expression statement = Expression_Block(list, env, 2);
    if (!statement) {
      env.ReportError(list, "'if' -- body block failed to compile");
      return nullptr;
    }

    return Expression_If(predicate, statement);
  }
}
