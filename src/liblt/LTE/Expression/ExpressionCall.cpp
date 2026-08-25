#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/Script.h"
#include "LTE/StackFrame.h"
#include "LTE/StringList.h"
#include "LTE/Vector.h"

namespace {
  AutoClassDerived(ExpressionExpressionCall, ExpressionT,
    ScriptFunction, function,
    Array<Expression>, args)
    DERIVED_TYPE_EX(ExpressionExpressionCall)
    POOLED_TYPE

    ExpressionExpressionCall() = default;

    ExpressionExpressionCall(
        ScriptFunction const& function,
        Vector<Expression> arguments) :
      function(function)
    {
      args.resize(arguments.size());
      for (size_t i = 0; i < args.size(); ++i)
        args[i] = arguments[i];
    }

    String Emit(Vector<String>& scope) const override {
      Stringize call = Stringize() | function->name | "(";
      for (size_t i = 0; i < args.size(); ++i) {
        if (i) call | ", ";
        call | args[i]->Emit(scope);
      }

      call | ")";
      return call;
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      // SFRAME(&function->name.front());
      uint prevBase = env.base;
      uint newBase = env.registers.size();

      for (size_t i = 0; i < args.size(); ++i) {
        void* lv = args[i]->GetLValue(env);
        if (lv)
          env.registers.push(lv);
        else {
          void* lv = env.Allocate(function->parameters[i].type);
          args[i]->Evaluate(lv, env);
          env.registers.push(lv);
        }
      }

      env.base = newBase;
      env.returnValue = returnValue;
      function->expression->Evaluate(returnValue, env);
      /* A `return` inside the body set this flag to stop the body early;
         clear it so the caller (which reuses this Environment) is not
         affected. A `return` only ever applies to the innermost function.
         Same for `break`: a top-level `break` (outside any loop) in the
         body must not break an enclosing loop of the caller. */
      env.returnSignal = false;
      env.breakSignal = false;

      for (size_t i = 0; i < args.size(); ++i) {
        size_t index = args.size() - i - 1;
        if (!args[index]->IsLValue())
          env.Free(function->parameters[index].type, env.registers[env.base + index]);
        env.registers.pop();
      }

      env.base = prevBase;
    }

    Type GetType() const override {
      return function->returnType;
    }

    bool IsConstant(CompileEnvironment& env) const override {
      /* TODO. */
      return false;
      // return function->expression->IsConstant(env);
    }
  };

  ScriptFunction MemberLookup(
    Expression const& member,
    String const& name,
    CompileEnvironment& env)
  {
    return env.script->GetFunction(name);
  }
}

namespace LTE {
  Expression Expression_ExpressionCall(
    ScriptFunction const& function,
    Vector<Expression> const& arguments)
  {
    return new ExpressionExpressionCall(function, arguments);
  }

  Expression Expression_ExpressionCall(
    StringList const& original,
    CompileEnvironment& env)
  {
    StringList list = original;
    ScriptFunction function;

    if (!list->IsAtom() &&
        !list->Get(0)->IsAtom() &&
         list->Get(0)->GetSize() == 2)
    {
      list = original->Clone();
      ((StringListList*)list.t)->elements[0] = original->Get(0)->Get(0);
      ((StringListList*)list.t)->elements.insert(1, original->Get(0)->Get(1));
    }
    
    /* TODO : Replace with real lookup. */
    String const& name = list->IsAtom()
      ? list->GetValue()
      : list->Get(0)->GetValue();

    if (name.contains(':')) {
      Vector<String> strings;
      String_Split(strings, name, ':');
      String const& scriptName = strings[0];
      String const& functionName = strings[1];

      Script script = env.script->ResolveRelativePath(scriptName);
      if (script) {
        function = script->GetFunction(functionName);
        if (function)
          env.script->dependencies.push(script);
      }
    } else {
      /* TODO : Unified function lookup. */
      if (list->GetSize() > 1) {
        Expression member = Expression_Compile(list->Get(1), env);
        if (member) {
          Type const& type = member->GetType();

          /* Script Type. */
          if (type->GetAux() && type->GetAux().IsType<ScriptType>()) {
            ScriptType const& scriptType = type->GetAux().Convert<ScriptType>();
            function = scriptType->GetFunction(name);
          }
        }
      }

      if (!function && env.script) {
        function = env.script->GetFunction(name);
      }
    }

    if (!function) {
      env.ReportError(list, Stringize()
        | "no function named '" | name | "' found in this script or imported scripts");
      return nullptr;
    }

    if (list->GetSize() != function->parameters.size() + 1) {
      env.ReportError(list, Stringize()
        | "function '" | function->name | "' expects "
        | function->parameters.size() | " argument(s), but got "
        | (list->GetSize() - 1));
      return nullptr;
    }

    Vector<Expression> arguments;
    for (size_t i = 1; i < list->GetSize(); ++i) {
      Expression argument = Expression_Compile(list->Get(i), env);
      if (!argument) return nullptr;

      /* Capture the source type BEFORE the conversion attempt: on failure
         `argument` is nulled and the original expression is gone. (This
         used to report arguments.back() — the PREVIOUS argument's type,
         and undefined behavior when argument 1 itself failed.) */
      Type argType = argument->GetType();
      argument = Expression_Conversion(argument, function->parameters[i - 1].type);
      if (!argument) {
        env.ReportError(list->Get(i), Stringize()
          | "argument " | (i) | " to function '" | function->name
          | "' has type '" | argType->GetAliasName()
          | "' but expected '" | function->parameters[i - 1].type->GetAliasName() | "'");
        return nullptr;
      }

      arguments.push(argument);
    }

    return Expression_ExpressionCall(function, arguments);
  }
}
