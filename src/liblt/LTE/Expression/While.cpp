#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionWhile, ExpressionT,
    Expression, predicate,
    Expression, statement,
    Type, statementType)
    DERIVED_TYPE_EX(ExpressionWhile)
    POOLED_TYPE

    ExpressionWhile() = default;

    String Emit(Vector<String>& context) const override {
      /* Predicate. */ {
        Vector<String> predContext;
        String predCall = predicate->Emit(predContext);
        
        context.push("while (1) {");
        for (size_t i = 0; i < predContext.size(); ++i)
          context.push("  " + predContext[i]);
        context.push("  if (!" + predCall + ") break;");
      }

      /* Statement. */ {
        Vector<String> statementContext;
        statement->Emit(statementContext);
        for (size_t i = 0; i < statementContext.size(); ++i)
          context.push("  " + statementContext[i]);
        context.push("}");
      }
      return "";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      bool pred;
      while (true) {
        predicate->Evaluate(&pred, env);
        if (env.returnSignal)
          break;
        if (pred) {
          if (statementType->allocate) {
            void* lv = env.Allocate(statementType);
            statement->Evaluate(lv, env);
            env.Free(statementType, lv);
          } else {
            statement->Evaluate(0, env);
          }
          /* A `return` in the body sets returnSignal; exit the loop so the
             enclosing function unwinds instead of spinning forever on the
             same iteration (the increment/operation appended to a `for`
             body is skipped by the block once the signal is set). */
          if (env.returnSignal)
            break;
        } else
          break;
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
  Expression Expression_While(
    Expression const& predicate,
    Expression const& statement)
  {
    return new ExpressionWhile(predicate, statement, statement->GetType());
  }

  Expression Expression_While(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() < 3) {
      env.ReportError(list, "'while' expects at least 2 arguments (predicate, body)");
      return nullptr;
    }

    Expression predicate = Expression_Compile(list->Get(1), env);
    if (!predicate) {
      env.ReportError(list, "'while' -- predicate expression failed to compile");
      return nullptr;
    }

    predicate = Expression_Conversion(predicate, Type_Get<bool>());
    if (!predicate) {
      env.ReportError(list, "'while' -- predicate must be convertible to 'bool'");
      return nullptr;
    }

    Expression statement = Expression_Block(list, env, 2);
    if (!statement) {
      env.ReportError(list, "'while' -- body block failed to compile");
      return nullptr;
    }

    return Expression_While(predicate, statement);
  }
}
