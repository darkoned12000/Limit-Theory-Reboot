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
  AutoClassDerived(ExpressionBreak, ExpressionT,
    Expression, unused)
    DERIVED_TYPE_EX(ExpressionBreak)
    POOLED_TYPE

    ExpressionBreak() = default;

    String Emit(Vector<String>& context) const override {
      return "break";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      /* Signal the innermost enclosing loop (`while` / `for`) to exit.
         ExpressionWhile::Evaluate consumes (clears) the signal so nested
         loops do not propagate it outward; blocks stop evaluating further
         statements once it is set. */
      env.breakSignal = true;
    }

    Type GetType() const override {
      return Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return true;
    }
  };
}

namespace LTE {
  Expression Expression_Break() {
    return new ExpressionBreak(Expression_Noop());
  }
}
