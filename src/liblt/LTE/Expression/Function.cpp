// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "../Expressions.h"

#include "LTE/Environment.h"
#include "LTE/ProgramLog.h"
#include "LTE/Script.h"
#include "LTE/StringList.h"

namespace {
  void ScriptFunction_Call(void*, void**, void*) {
  }
}

namespace LTE {
  ScriptFunction ScriptFunction_ParseSignature(
    StringList const& list,
    CompileEnvironment& env,
    ScriptType const& methodOwner)
  {
    if (list->GetSize() < 4) {
      env.ReportError(list, Stringize()
        | "'function' expects at least 3 arguments (return-type, name, params), but got "
        | (list->GetSize() - 1));
      return nullptr;
    }

    ScriptFunction fn = new ScriptFunctionT;
    fn->name = list->Get(2)->GetValue();

    if (methodOwner)
      fn->parameters.push(Parameter("this", methodOwner->type));

    Type returnType = env.script->ResolveType(list->Get(1));
    /* The declared return type is advisory, not a contract: the original
       engine always derived returnType from the body's expression type and
       never validated the declaration (e.g. scripts declare 'Void', which is
       not a registered type). Resolve it when possible so forward-referenced
       calls get a sane transient type; otherwise leave it null — the body
       compile overwrites it either way. Do NOT error out here. */
    fn->returnType = returnType ? returnType : Type_Get<void>();

    StringList const& params = list->Get(3);
    for (size_t i = 0; i < params->GetSize(); i += 2) {
      StringList const& typeName = params->Get(i + 0);
      String varName = params->Get(i + 1)->GetValue();
      Type type = env.script->ResolveType(typeName);
      if (!type) {
        Vector<String> typeNames;
        if (env.script) {
          for (auto it = env.script->types.begin(); it != env.script->types.end(); ++it)
            typeNames.push(it->first);
        }
        String suggestion = BestMatch(typeName->GetString(), typeNames);
        if (suggestion.size() > 0)
          env.ReportError(list, Stringize()
            | "unknown type '" | typeName->GetString()
            | "' in parameter list of function '" | fn->name
            | "' (did you mean '" | suggestion | "'?)");
        else
          env.ReportError(list, Stringize()
            | "unknown type '" | typeName->GetString()
            | "' in parameter list of function '" | fn->name | "'");
        return nullptr;
      }
      fn->parameters.push(Parameter(varName, type));
    }

    return fn;
  }

  Expression Expression_Function(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() < 4) {
      env.ReportError(list, Stringize()
        | "'function' expects at least 3 arguments (return-type, name, params), but got "
        | (list->GetSize() - 1));
      return nullptr;
    }

    String const& name = list->Get(2)->GetValue();

    /* Reuse a signature pre-registered by the type compiler when one exists
       (Expression_Type registers every method signature up front so method
       bodies can call methods declared LATER in the same type). The
       placeholder object is mutated in place, so ExpressionCall nodes that
       were resolved against it during an earlier method's compilation see
       the body once it is compiled below. Otherwise create fresh, erroring
       on duplicates. */
    ScriptFunction fn;
    if (env.context.size() && env.context.back()->functions[name]) {
      fn = env.context.back()->functions[name];
    } else if (env.context.size()) {
      fn = ScriptFunction_ParseSignature(list, env, env.context.back());
    } else {
      fn = env.script->functions[name];
      if (fn) {
        env.ReportError(list, Stringize()
          | "function '" | name | "' is already defined");
        return nullptr;
      }

      fn = ScriptFunction_ParseSignature(list, env);
    }
    if (!fn)
      return nullptr;

    CompileEnvironment subEnv;
    subEnv.script = env.script;
    subEnv.context = env.context;

    /* Rebuild the full parameter list: implicit 'this' (per context) first,
       then declared parameters. For a reused placeholder this reproduces the
       pre-registered signature exactly; for a fresh function it is the sole
       construction. */
    fn->parameters.clear();
    for (size_t i = 0; i < subEnv.context.size(); ++i) {
      ScriptType context = subEnv.context[i];
      uint contextRegister = subEnv.Allocate("this", context->type, false, false);

      for (size_t j = 0; j < context->fields.size(); ++j) {
        Field const& field = context->fields[j];
        subEnv.variables[field.name].push(
          Variable(field.name, contextRegister, field.offset, field.type, false, false));
      }

      fn->parameters.push(Parameter("this", context->type));
    }

    /* Compile parameters. */
    StringList const& params = list->Get(3);
    for (size_t i = 0; i < params->GetSize(); i += 2) {
      StringList const& typeName = params->Get(i + 0);
      String varName = params->Get(i + 1)->GetValue();
      Type type = env.script->ResolveType(typeName);
      if (!type) {
        Vector<String> typeNames;
        if (env.script) {
          for (auto it = env.script->types.begin(); it != env.script->types.end(); ++it)
            typeNames.push(it->first);
        }
        String suggestion = BestMatch(typeName->GetString(), typeNames);
        if (suggestion.size() > 0)
          env.ReportError(list, Stringize()
            | "unknown type '" | typeName->GetString()
            | "' in parameter list of function '" | name
            | "' (did you mean '" | suggestion | "'?)");
        else
          env.ReportError(list, Stringize()
            | "unknown type '" | typeName->GetString()
            | "' in parameter list of function '" | name | "'");
        return nullptr;
      }

      fn->parameters.push(Parameter(varName, type));
      subEnv.Allocate(varName, type, false, false);
    }

    /* TODO : Support recursion */
    fn->expression = Expression_Block(list, subEnv, 4);
    if (!fn->expression)
      fn->expression = Expression_Noop();
    fn->returnType = fn->expression->GetType();

    /* Function bodies compile into their own sub-environment; surface any
       errors there in the outer environment so they reach ScriptT::Reload's
       PrintErrors. Without this, a compile error inside a method/function
       body silently no-ops the offending statement (Expression_Block drops
       failed expressions) and the app "works" with zero diagnostics — exactly
       the silent-failure class ltsl-hardening.md exists to eliminate. */
    for (size_t i = 0; i < subEnv.errors.size(); ++i)
      env.errors.push(subEnv.errors[i]);
    if (subEnv.hasErrors)
      env.hasErrors = true;

    if (env.context.size())
      env.context.back()->functions[name] = fn;
    else
      env.script->functions[name] = fn;

    /* Native function. */
#if 0
    Function native = Function_Create("LTSL_Function_" + name);
    native->name = name;
    native->description = "LTSL-Defined Function";
    native->call = ScriptFunction_Call;
    native->paramCount = fn->parameters.size();
    native->params = new Parameter[native->paramCount];
    for (size_t i = 0; i < native->paramCount; i++)
      Mutable(native->params[i]) = fn->parameters[i];
#endif

    return nullptr;
  }
}
