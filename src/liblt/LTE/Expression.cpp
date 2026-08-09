// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Expression.h"
#include "Environment.h"
#include "Expressions.h"
#include "Map.h"
#include "ProgramLog.h"
#include "Script.h"
#include "Stack.h"
#include "StackFrame.h"
#include "StringList.h"

#include <cctype>
#include <iostream>
#include "Debug.h"

namespace LTE {
  /* True when `value` can only be interpreted as a literal (number, string, or
     bool) rather than an identifier. Mirrors the recognition rules in
     Expression_Constant so probe skipping never changes which atom resolves. */
  bool IsLiteralAtom(String const& value) {
    if (value == "true" || value == "false")
      return true;

    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\'')))
      return true;

    bool alpha = false;
    bool digit = false;
    for (size_t i = 0; i < value.size(); ++i) {
      char c = value[i];
      if (isalpha(c))
        alpha = true;
      if (isdigit(c) || c == '-' || c == '.')
        digit = true;
    }
    return !alpha && digit;
  }

  void ExpressionT::Evaluate(void* returnValue) const {
    AUTO_FRAME;
    Environment env;
    env.returnValue = returnValue;
    Evaluate(returnValue, env);
  }

  /* The probe-chain core. Expression_Compile tries each candidate
     interpretation (Variable, Reference, FunctionCall, Access, Dereference,
     Constructor, ...) in order; the wrappers below roll back diagnostics
     that probes leave behind when a LATER probe succeeds. */
  Expression CompileExpressionCore(
    StringList const& list,
    CompileEnvironment& env,
    Vector<String>* locals)
  {
    SFRAME("Compile Expression");

    if (list->IsAtom()) {
      /* Return with no value. */ {
        if (list->GetValue() == "return")
          return Expression_Return(Expression_Noop());
      }

      /* Comment line. */ {
        if (list->GetValue() == "#")
          return nullptr;
      }

      /* Literal atoms (numbers, strings, bools) never resolve as variables,
         references, calls, or constructors — only the Constant factory can
         accept them. Skip the earlier probes so their spurious "unknown
         variable '1'" errors don't pollute the real diagnostics
         (ltsl-hardening.md §5.1). */ {
        if (IsLiteralAtom(list->GetValue()))
          return Expression_Constant(list, env);
      }

      /* Variable. */ {
        Expression e = Expression_Variable(list, env);
        if (e) return e;
      }

      /* Reference. */ {
        Expression e = Expression_Reference(list, env);
        if (e) return e;
      }

      /* Function call. */ {
        Expression e = Expression_FunctionCall(list, env);
        if (e) return e;
      }

      /* Expression call. */ {
        Expression e = Expression_ExpressionCall(list, env);
        if (e) return e;
      }

      /* Constructor. */ {
        Expression e = Expression_Constructor(list, env);
        if (e) return e;
      }

      /* Constant. */ {
        Expression e = Expression_Constant(list, env);
        if (e) return e;
      }

      /* If we get here, nothing recognized the atom — report with suggestions. */
      String const& name = list->GetValue();
      Vector<String> candidates;
      env.CollectVariableNames(candidates);
      if (env.script) {
        for (auto it = env.script->functions.begin(); it != env.script->functions.end(); ++it)
          candidates.push(it->first);
        for (auto it = env.script->types.begin(); it != env.script->types.end(); ++it)
          candidates.push(it->first);
      }
      String suggestion = BestMatch(name, candidates);
      if (suggestion.size() > 0)
        env.ReportError(list, Stringize()
          | "unresolved name '" | name | "' (did you mean '" | suggestion | "'?)");
      else
        env.ReportError(list, Stringize()
          | "unresolved name '" | name | "'");
      return nullptr;
    }

    /* Empty lists are never used, but check for them to prevent crash. */
    if (list->GetSize() == 0) {
      env.ReportError(list, "empty expression (missing operand?)");
      return nullptr;
    }

    if (list->Get(0)->IsAtom()) {
      String const& value = list->Get(0)->GetValue();
      if (value == "#")
        return nullptr;
      if (value == "@")
        return Expression_Print(list, env);
      if (value == "address")
        return Expression_Address(list, env);
      if (value == "block")
        return Expression_Block(list, env, 1);
      if (value == "call")
        return Expression_DynamicDispatch(list, env);
      if (value == "cast")
        return Expression_Cast(list, env);
      if (value == "desc")
        return Expression_Block(list, env, 2);
      if (value == "deref")
        return Expression_DereferencePointer(list, env);
      if (value == "for")
        return Expression_For(list, env);
      if (value == "function")
        return Expression_Function(list, env);
      if (value == "if")
        return Expression_If(list, env);
      if (value == "list")
        return Expression_List(list, env);
      if (value == "ref")
        return Expression_DeclareReference(list, env, locals);
      if (value == "return")
        return Expression_Return(list, env);
      if (value == "set" || value == "=")
        return Expression_Assign(list, env);
      if (value == "static")
        return Expression_DeclareStatic(list, env, locals);
      if (value == "switch" || value == "?")
        return Expression_Switch(list, env);
      if (value == "type")
        return Expression_Type(list, env);
      if (value == "var")
        return Expression_DeclareLocal(list, env, locals);
      if (value == "while")
        return Expression_While(list, env);
    }

    /* Function call. */ {
      Expression e = Expression_FunctionCall(list, env);
      if (e) return e;
    }

    /* Expression call. */ {
      Expression e = Expression_ExpressionCall(list, env);
      if (e) return e;
    }

    /* Field access. */ {
      Expression e = Expression_Access(list, env);
      if (e) return e;
    }

    /* Pointer field access. */ {
      Expression e = Expression_Dereference(list, env);
      if (e) return e;
    }

    /* Constructor. */ {
      Expression e = Expression_Constructor(list, env);
      if (e) return e;
    }

    /* Implicit conversion. */ {
      Expression e = Expression_Conversion(list, env);
      if (e) return e;
    }
    
    if (list->GetSize() == 1)
      return Expression_Compile(list->Get(0), env);

    /* Nothing recognized this expression — report with context. */
    if (!env.hasErrors) {
      env.hasErrors = true;
      env.PrintErrors(env.script->name);
    }

    return nullptr;
  }

  Expression Expression_Compile(
    StringList const& list,
    CompileEnvironment& env,
    Vector<String>* locals)
  {
    /* Probe-chain noise rollback. Each candidate interpretation probe
       (Variable, Reference, FunctionCall, Access, Dereference, Constructor,
       Conversion) reports errors for the constructs it does NOT handle — e.g.
       `self.pos` fails the FunctionCall probe ("no native function named
       'pos'") before the Access probe resolves it. When a later probe
       succeeds, those earlier diagnostics are spurious and must not stick:
       function bodies now surface their own compile errors (Expression_Function
       propagates the body sub-environment), so any surviving noise would
       flood every app load with false errors. Roll the error state back to
       the entry snapshot on success; only genuinely-failed compiles keep
       their diagnostics. */
    size_t errorStart = env.errors.size();
    bool errorState = env.hasErrors;

    Expression e = CompileExpressionCore(list, env, locals);
    if (e) {
      env.errors.resize(errorStart);
      env.hasErrors = errorState;
    }
    return e;
  }

  Expression Expression_Compile(StringList const& list) {
    CompileEnvironment env;
    return Expression_Compile(list, env, nullptr);
  }
}
