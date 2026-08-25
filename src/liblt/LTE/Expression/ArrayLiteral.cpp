// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// `[a, b, c]` array literals (Phase 0 QW5). The tokenizer maps bracket
// groups to `(__bracket ...)` lists; this file infers the element type from
// the first element, converts every element to it, and evaluates to a fresh
// engine array value (Type_Array) whose registers hold the container
// pointer — the same representation `(Array T)` locals use.

#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/StringList.h"
#include "LTE/Types.h"

namespace {
  AutoClassDerived(ExpressionArrayLiteral, ExpressionT,
    Type, arrayType,
    Type, elemType,
    Array<Expression>, elements)
    DERIVED_TYPE_EX(ExpressionArrayLiteral)
    POOLED_TYPE

    ExpressionArrayLiteral() = default;

    ExpressionArrayLiteral(
        Type const& arrayType,
        Type const& elemType,
        Vector<Expression> const& compiledElements) :
      arrayType(arrayType),
      elemType(elemType)
    {
      elements.resize(compiledElements.size());
      for (size_t i = 0; i < elements.size(); ++i)
        elements[i] = compiledElements[i];
    }

    String Emit(Vector<String>& context) const override {
      Stringize value = Stringize() | "[";
      for (size_t i = 0; i < elements.size(); ++i) {
        if (i) value | ", ";
        value | elements[i]->Emit(context);
      }
      value | "]";
      return value;
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      /* Build the container. Registers for engine array types hold the
         heap-allocated container pointer (see Type_ArrayAlloc), matching
         what type->Allocate() leaves in a declared `(Array T)` local. */
      void* arr = Type_ArrayAlloc(arrayType);

      for (size_t i = 0; i < elements.size(); ++i) {
        void* tmp = elemType->Allocate();
        elements[i]->Evaluate(tmp, env);
        Type_ArrayAppend(arr, tmp);
        elemType->Deallocate(tmp);
      }

      if (returnValue)
        *(void**)returnValue = arr;
      else
        /* Statement context — nothing consumes the value. */
        arrayType->Deallocate(arr);
    }

    Type GetType() const override {
      return arrayType;
    }

    bool IsConstant(CompileEnvironment&) const override {
      return false;
    }
  };
}

namespace LTE {
  Expression Expression_ArrayLiteral(
    StringList const& list,
    CompileEnvironment& env)
  {
    /* list = (__bracket e1 e2 ...). */
    if (list->GetSize() < 2) {
      env.ReportError(list, Stringize()
        | "array literal needs at least one element (to infer its type); "
        | "declare an empty array with 'var name (Array ElementType)'");
      return nullptr;
    }

    Vector<Expression> compiled;
    compiled.resize(list->GetSize() - 1);

    for (size_t i = 1; i < list->GetSize(); ++i) {
      Expression e = Expression_Compile(list->Get(i), env);
      if (!e)
        return nullptr;

      if (!compiled[0]) {
        /* First element fixes the element type. */
        compiled[i - 1] = e;
        continue;
      }

      e = Expression_Conversion(e, compiled[0]->GetType());
      if (!e)
        return nullptr;
      compiled[i - 1] = e;
    }

    Type elemType = compiled[0]->GetType();
    return new ExpressionArrayLiteral(
      Type_Array(elemType),
      elemType,
      compiled);
  }
}
