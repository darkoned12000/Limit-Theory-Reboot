#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/ScriptFunction.h"
#include "LTE/StringList.h"

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

  AutoClassDerived(ExpressionDynamicDispatch, ExpressionT,
    String, name,
    Expression, object,
    Array<ArgData>, args)
    DERIVED_TYPE_EX(ExpressionDynamicDispatch)
    POOLED_TYPE

    ExpressionDynamicDispatch() = default;

    ExpressionDynamicDispatch(
        String const& name,
        Expression const& object,
        Vector<Expression> const& args) :
      name(name),
      object(object)
    {
      this->args.resize(args.size());
      for (size_t i = 0; i < args.size(); i++)
        this->args[i] = ArgData(args[i]);
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      Data objectData;
      object->Evaluate(&objectData, env);

      Type const& type = objectData.type;
      if (!type) {
        Log_Critical("LTSL dynamic dispatch to typeless data.");
        return;
      }

      Data const& aux = type->GetAux();
      if (!aux.IsType<ScriptType>()) {
        Log_Critical("LTSL dynamic dispatch to non-LTSL type.");
        return;
      }

      ScriptType const& st = aux.Convert<ScriptType>();
      ScriptFunction* function = st->functions.get(name);

      if (!function) {
        Log_Critical("LTSL dynamic dispatch to '" + name + "' failed (function not found)");
        return;
      }

      ScriptFunction const& fn = *function;

      if (fn->parameters.size() != args.size() + 1) {
        Log_Critical("LTSL dynamic dispatch to '" + name + "' failed (wrong number of arguments)");
        return;
      }

      for (size_t i = 0; i < args.size(); i++) {
        if (args[i].type != fn->parameters[i + 1].type) {
          Log_Critical("LTSL dynamic dispatch to '" + name + "' failed (argument type mismatch)");
          return;
        }
      }

      /* TODO : Unify with expressioncall. */
      uint prevBase = env.base;
      uint newBase = env.registers.size();

      env.registers.push(objectData.data);

      for (size_t i = 0; i < args.size(); ++i) {
        ArgData const& arg = args[i];
        void* lv = arg.expression->GetLValue(env);
        if (lv)
          env.registers.push(lv);
        else {
          void* lv = env.Allocate(arg.type);
          arg.expression->Evaluate(lv, env);
          env.registers.push(lv);
        }
      }

      env.base = newBase;
      if (fn->returnType->allocate) {
        void* lv = env.Allocate(fn->returnType);
        fn->expression->Evaluate(lv, env);
        env.Free(fn->returnType, lv);
      } else {
        fn->expression->Evaluate(0, env);
      }
      /* A `return` inside the dispatched body only applies to that body;
         clear the flag so the caller (which reuses this Environment) is
         not affected. Same for `break`: a top-level `break` in the body
         must not break an enclosing loop of the caller. */
      env.returnSignal = false;
      env.breakSignal = false;

      for (size_t i = 0; i < args.size(); ++i) {
        size_t index = args.size() - i - 1;
        if (!args[index].expression->IsLValue())
          env.Free(args[index].type, env.registers[env.base + index + 1]);
        env.registers.pop();
      }
      env.registers.pop();

      env.base = prevBase;
    }

    Type GetType() const override {
      return Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return false;
    }
  };
}

namespace LTE {
  Expression Expression_DynamicDispatch(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() < 3) {
      env.ReportError(list, "'call' expects at least 2 arguments (object, method)");
      return nullptr;
    }

    Expression object = Expression_Compile(list->Get(1), env);
    if (!object) {
      env.ReportError(list, "'call' -- object expression failed to compile");
      return nullptr;
    }

    Type const& type = object->GetType();
    if (type != Type_Get<Data>()) {
      env.ReportError(list, Stringize()
        | "'call' object must be of type 'Data', but got '" | type->name | "'");
      return nullptr;
    }

    if (!list->Get(2)->IsAtom()) {
      env.ReportError(list, "'call' -- method name (argument 2) must be a plain name");
      return nullptr;
    }

    String name = list->Get(2)->GetValue();

    Vector<Expression> args;
    for (size_t i = 3; i < list->GetSize(); i++) {
      Expression e = Expression_Compile(list->Get(i), env);
      if (!e) {
        env.ReportError(list, Stringize()
          | "'call' -- argument " | (i - 2) | " to method '" | name
          | "' failed to compile");
        return nullptr;
      }
      args.push(e);
    }

    return new ExpressionDynamicDispatch(name, object, args);
  }
}
