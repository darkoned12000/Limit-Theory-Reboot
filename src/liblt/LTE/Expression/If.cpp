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
    Type, statementType,
    Expression, elseStatement,
    Type, elseStatementType)
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

      if (elseStatement) {
        localContext.clear();
        elseStatement->Emit(localContext);
        context.push("else {");
        for (size_t i = 0; i < localContext.size(); ++i)
          context.push("  " + localContext[i]);
        context.push("}");
      }

      return "";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      bool pred;
      predicate->Evaluate(&pred, env);
      Expression const& branch = pred ? statement : elseStatement;
      Type const& branchType = pred ? statementType : elseStatementType;
      if (branchType && branchType->allocate) {
        void* lv = env.Allocate(branchType);
        branch->Evaluate(lv, env);
        env.Free(branchType, lv);
      } else {
        branch->Evaluate(0, env);
      }
    }

    Type GetType() const override {
      return Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      if (!predicate->IsConstant(env) || !statement->IsConstant(env))
        return false;
      return !elseStatement || elseStatement->IsConstant(env);
    }
  };

  /* Compile a flat slice of `list` (elements [start, end)) as an `else`
     branch body. A leading `if` atom means the branch is itself an if/else
     chain (e.g. `else if b ... else ...`), which RewriteElse left flat
     inside the parent `if` list; wrap the whole slice back into a single
     `(if ...)` list so Expression_Compile re-enters Expression_If and the
     chain collapses recursively. Any other shape compiles as a plain block. */
  Expression CompileElseRegion(
    StringList const& list,
    CompileEnvironment& env,
    size_t start,
    size_t end)
  {
    if (start >= end)
      return Expression_Noop();

    StringList first = list->Get(start);
    if (first->IsAtom() && first->GetValue() == "if") {
      Vector<StringList> elements;
      for (size_t i = start; i < end; ++i)
        elements.push(list->Get(i));
      StringList nested = new StringListList(elements);
      return Expression_Compile(nested, env);
    }

    return Expression_Block(list, env, (uint)start, (uint)end);
  }
}

namespace LTE {
  Expression Expression_If(
    Expression const& predicate,
    Expression const& statement)
  {
    return Expression_If(predicate, statement, Expression_Noop());
  }

  Expression Expression_If(
    Expression const& predicate,
    Expression const& statement,
    Expression const& elseStatement)
  {
    return new ExpressionIf(
      predicate, statement, statement->GetType(),
      elseStatement,
      elseStatement ? elseStatement->GetType() : nullptr);
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

    /* Locate the optional `else` atom inserted by the RewriteElse pass when
       an `(else ...)` sibling followed this `(if ...)` list. */
    size_t elseIndex = list->GetSize();
    for (size_t i = 2; i < list->GetSize(); ++i) {
      StringList e = list->Get(i);
      if (e->IsAtom() && e->GetValue() == "else") {
        elseIndex = i;
        break;
      }
    }

    Expression statement = Expression_Block(list, env, 2, (uint)elseIndex);
    if (!statement) {
      env.ReportError(list, "'if' -- body block failed to compile");
      return nullptr;
    }

    Expression elseStatement = Expression_Noop();
    if (elseIndex + 1 < list->GetSize()) {
      elseStatement = CompileElseRegion(list, env, elseIndex + 1, list->GetSize());
      if (!elseStatement) {
        env.ReportError(list, "'if' -- else block failed to compile");
        return nullptr;
      }
    }

    return Expression_If(predicate, statement, elseStatement);
  }
}
