#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/Script.h"
#include "LTE/StackFrame.h"
#include "LTE/StringList.h"
#include "LTE/Vector.h"

#include <algorithm>

namespace {
  AutoClass(ArgData,
    Expression, expression,
    Type, type,
    bool, isLValue)
    ArgData() = default;

    ArgData(Expression const& expression) :
      expression(expression),
      type(expression->GetType()),
      isLValue(expression->IsLValue())
      {}
  };

  AutoClassDerived(ExpressionFunctionCall, ExpressionT,
    Function, function,
    Array<ArgData>, args)
    Array<void*> argStack;

    DERIVED_TYPE_EX(ExpressionFunctionCall)
    POOLED_TYPE


    ExpressionFunctionCall() = default;

    ExpressionFunctionCall(
        Function const& function,
        Vector<Expression> const& arguments) :
      function(function)
    {
      args.resize(arguments.size());
      argStack.resize(arguments.size(), nullptr);
      for (size_t i = 0; i < arguments.size(); ++i)
        args[i] = ArgData(arguments[i]);
    }

    String Emit(Vector<String>& scope) const override {
      Stringize call = Stringize() | function->name | "(";
      for (size_t i = 0; i < args.size(); ++i) {
        if (i) call | ", ";
        call | args[i].expression->Emit(scope);
      }

      call | ")";
      return call;
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      // SFRAME(function->name.data());

      for (size_t i = 0; i < args.size(); ++i) {
        ArgData const& arg = args[i];
        if (!arg.isLValue) {
          argStack[i] = env.Allocate(arg.type);
          arg.expression->Evaluate(argStack[i], env);
        } else {
          argStack[i] = arg.expression->GetLValue(env);
        }
      }

      function->call(argStack.data(), returnValue);

      for (size_t i = 0; i < args.size(); ++i) {
        size_t index = args.size() - i - 1;
        ArgData const& arg = args[index];
        if (!arg.isLValue)
          env.Free(arg.type, argStack[index]);
      }
    }

    Type GetType() const override {
      return function->returnType;
    }

    bool IsConstant(CompileEnvironment& env) const override {
      /* TODO */
      return false;

      /* NOTE : Assumes pure functions! Dangerous! */
      for (size_t i = 0; i < args.size(); ++i)
        if (!args[i].expression->IsConstant(env))
          return false;
      return true;
    }
  };

  AutoClass(FunctionMatch,
    Function, fn,
    uint, order)
    
    FunctionMatch() = default;

    friend bool operator<(FunctionMatch const& a, FunctionMatch const& b) {
      return a.order < b.order;
    }
  };

  Function OverloadResolution(
    String const& name,
    Vector<Function> const& candidates,
    Vector<Expression>& expressions,
    CompileEnvironment& env,
    StringList const& list)
  {
    Vector<Type> types;
    for (size_t i = 0; i < expressions.size(); ++i)
      types.push(expressions[i]->GetType());

    Vector<FunctionMatch> matches;
    for (size_t i = 0; i < candidates.size(); ++i) {
      Function const& fn = candidates[i];
      if (fn->paramCount != expressions.size())
        continue;

      bool match = true;
      uint order = 0;

      for (size_t j = 0; j < expressions.size(); ++j) {
        if (types[j] != fn->params[j].type) {
          order++;
          if (!Expression_Conversion(expressions[j], fn->params[j].type))
            match = false;
        }
      }

      if (match)
        matches.push(FunctionMatch(fn, order));
    }

    if (matches.size() == 1) {
      Function const& fn = matches[0].fn;
      /* Create implicit conversion. */
      for (size_t i = 0; i < expressions.size(); ++i)
        expressions[i] = Expression_Conversion(expressions[i], fn->params[i].type);
      return fn;
    }

    if (matches.isEmpty()) {
      String sig = "(";
      for (size_t i = 0; i < types.size(); ++i) {
        if (i) sig += ", ";
        sig += types[i]->GetAliasName();
      }
      sig += ")";

      env.ReportError(list, Stringize()
        | "no overload of '" | name | sig | "' is compatible with these argument types");

      if (candidates.size() > 0) {
        String avail = "  candidates: ";
        for (size_t i = 0; i < candidates.size(); ++i) {
          if (i) avail += ", ";
          avail += candidates[i]->GetSignature();
        }
        env.ReportError(list, avail);
      }

      return nullptr;
    }

    else {
      std::sort(matches.begin(), matches.end());
      if (matches[1].order > matches[0].order)
        return matches[0].fn;

      env.ReportError(list, Stringize()
        | "ambiguous call to '" | name | "' — multiple overloads match");

      for (size_t i = 0; i < matches.size(); ++i) {
        FunctionMatch const& match = matches[i];
        env.ReportError(list, Stringize()
          | "  candidate " | (i + 1) | " (" | match.order
          | " implicit conversion(s)): " | match.fn->GetSignature());
      }

      return nullptr;
    }
  }
}

namespace LTE {
  Expression Expression_FunctionCall(
    Function const& function,
    Vector<Expression> const& arguments)
  {
    return new ExpressionFunctionCall(function, arguments);
  }

  Expression Expression_FunctionCall(
    StringList const& original,
    CompileEnvironment& env)
  {
    StringList list = original;

    if (!list->IsAtom() &&
        !list->Get(0)->IsAtom() &&
         list->Get(0)->GetSize() == 2)
    {
      list = original->Clone();
      ((StringListList*)list.t)->elements[0] = original->Get(0)->Get(0);
      ((StringListList*)list.t)->elements.insert(1, original->Get(0)->Get(1));
    }

    String const& name = list->IsAtom()
      ? list->GetValue()
      : list->Get(0)->GetValue();

    Vector<Function> candidates = Function_Find(name);
    if (!candidates.size()) {
      /* Unknown function — try to suggest a similar name. */
      Vector<String> allFunctions;
      if (env.script) {
        for (auto it = env.script->functions.begin(); it != env.script->functions.end(); ++it)
          allFunctions.push(it->first);
      }
      String suggestion = BestMatch(name, allFunctions);
      if (suggestion.size() > 0)
        env.ReportError(list, Stringize()
          | "no native function named '" | name
          | "' (did you mean '" | suggestion | "'?)");
      else
        env.ReportError(list, Stringize()
          | "no native function named '" | name | "'");
      return nullptr;
    }

    Vector<Expression> args;
    for (size_t i = 1; i < list->GetSize(); ++i) {
      Expression e = Expression_Compile(list->Get(i), env); 
      if (!e) {
        env.ReportError(list, Stringize()
          | "argument " | i | " to function '" | name | "' failed to compile");
        return nullptr;
      }
      args.push(e);
    }

    Function fn = OverloadResolution(name, candidates, args, env, list);
    if (!fn)
      return nullptr;

    return Expression_FunctionCall(fn, args);
  }
}
