// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionReturn, ExpressionT,
    Expression, expression)
    DERIVED_TYPE_EX(ExpressionReturn)
    POOLED_TYPE

    ExpressionReturn() = default;

    String Emit(Vector<String>& context) const override {
      return "return " + expression->Emit(context);
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      /* Write the result to the enclosing function's canonical return slot
         (if one is bound) rather than the slot passed to this expression,
         so that early returns nested inside if/switch/blocks still land in
         the function's output rather than a transient buffer. */
      void* target = env.returnValue ? env.returnValue : returnValue;
      expression->Evaluate(target, env);
      env.returnSignal = true;
    }

    Type GetType() const override {
      return expression ? expression->GetType() : Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return expression ? expression->IsConstant(env) : true;
    }
  };
}

namespace LTE {
  Expression Expression_Return(Expression const& expression) {
    return new ExpressionReturn(expression);
  }

  Expression Expression_Return(StringList const& list, CompileEnvironment& env) {
    if (list->GetSize() < 2)
      return Expression_Return(Expression_Noop());

    Expression e = Expression_Compile(list->Get(1), env);
    if (!e) {
      env.ReportError(list, "'return' -- expression failed to compile");
      return nullptr;
    }

    return Expression_Return(e);
  }
}
