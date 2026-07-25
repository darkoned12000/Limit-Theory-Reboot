#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionDeclareLocal, ExpressionT,
    Expression, initializer,
    Type, type,
    String, name)
    DERIVED_TYPE_EX(ExpressionDeclareLocal)
    POOLED_TYPE

    ExpressionDeclareLocal() = default;

    String Emit(Vector<String>& context) const override {
      String valueCall = initializer->Emit(context);
      context.push(Stringize()
        | initializer->GetType()->name | " "
        | name | " = "
        | valueCall | ";");
      return "";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      void* lv = env.Allocate(type);
      initializer->Evaluate(lv, env);
      env.registers.push(lv);
    }

    Type GetType() const override {
      return Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return true;
    }
  };

  AutoClassDerived(ExpressionDeclareReference, ExpressionT,
    Expression, initializer,
    String, name)
    DERIVED_TYPE_EX(ExpressionDeclareReference)
    POOLED_TYPE

    ExpressionDeclareReference() = default;

    void Evaluate(void* returnValue, Environment& env) const override {
      env.registers.push(initializer->GetLValue(env));
    }

    Type GetType() const override {
      return Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return true;
    }
  };

  AutoClassDerived(ExpressionDeclareReferencePtr, ExpressionT,
    Expression, initializer,
    String, name)
    DERIVED_TYPE_EX(ExpressionDeclareReferencePtr)
    POOLED_TYPE

    ExpressionDeclareReferencePtr() = default;

    void Evaluate(void* returnValue, Environment& env) const override {
      void* lv;
      initializer->Evaluate(&lv, env);
      env.registers.push(lv);
    }

    Type GetType() const override {
      return Type_Get<void>();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return true;
    }
  };

  AutoClassDerived(ExpressionDeclareStatic, ExpressionT,
    Expression, initializer,
    Type, type,
    Data, value,
    String, name)
    DERIVED_TYPE_EX(ExpressionDeclareStatic)
    POOLED_TYPE

    ExpressionDeclareStatic() = default;

    void Evaluate(void* returnValue, Environment& env) const override {
      if (!value) {
        Mutable(value).Construct(type);
        initializer->Evaluate(value.data, env);
      }
      env.registers.push(value.data);
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
  Expression Expression_DeclareLocal(
    Expression const& value,
    String const& name)
  {
    return new ExpressionDeclareLocal(value, value->GetType(), name);
  }

  Expression Expression_DeclareLocal(
    StringList const& list,
    CompileEnvironment& env,
    Vector<String>* locals)
  {
    if (list->GetSize() != 3) {
      env.ReportError(list, Stringize()
        | "'var' expects 2 arguments (name, value), but got "
        | (list->GetSize() - 1));
      return nullptr;
    }

    if (!locals) {
      env.ReportError(list,
        "'var' used in a scope that does not support variable binding "
        "(e.g. inside a function call argument)");
      return nullptr;
    }

    Expression value = Expression_Compile(list->Get(2), env);
    if (!value) return nullptr;

    Type valueType = value->GetType();
    if (!valueType->assign) {
      env.ReportError(list, Stringize()
        | "cannot declare variable of type '" | valueType->name
        | "' (type is not assignable)");
      return nullptr;
    }

    String name = list->Get(1)->GetValue();
    env.Allocate(name, valueType, value->IsConstant(env), false);
    locals->push(name);
    return Expression_DeclareLocal(value, name);
  }

  Expression Expression_DeclareReference(
    Expression const& value,
    String const& name)
  {
    return value->IsLValue()
      ? (Expression)(new ExpressionDeclareReference(value, name))
      : (Expression)(new ExpressionDeclareReferencePtr(value, name));
  }

  Expression Expression_DeclareReference(
    StringList const& list,
    CompileEnvironment& env,
    Vector<String>* locals)
  {
    if (list->GetSize() != 3) {
      env.ReportError(list, Stringize()
        | "'ref' expects 2 arguments (name, value), but got "
        | (list->GetSize() - 1));
      return nullptr;
    }

    if (!locals) {
      env.ReportError(list,
        "'ref' used in a scope that does not support variable binding");
      return nullptr;
    }

    Expression value = Expression_Compile(list->Get(2), env);
    if (!value) return nullptr;

    Type valueType = value->GetType();
    if (value->IsLValue() || valueType->GetPointeeType()) {
      String name = list->Get(1)->GetValue();
      Type realType = value->IsLValue() ? valueType : valueType->GetPointeeType();
      env.Allocate(name, realType, value->IsConstant(env), true);
      locals->push(name);
      return Expression_DeclareReference(value, name);
    } else {
      env.ReportError(list,
        "'ref' target must be an l-value (variable) or a pointer");
      return nullptr;
    }
  }

  Expression Expression_DeclareStatic(
    Expression const& value,
    String const& name)
  {
    return new ExpressionDeclareStatic(value, value->GetType(), Data(), name);
  }

  Expression Expression_DeclareStatic(
    StringList const& list,
    CompileEnvironment& env,
    Vector<String>* locals)
  {
    if (list->GetSize() != 3) {
      env.ReportError(list, Stringize()
        | "'static' expects 2 arguments (name, value), but got "
        | (list->GetSize() - 1));
      return nullptr;
    }

    if (!locals) {
      env.ReportError(list,
        "'static' used in a scope that does not support variable binding");
      return nullptr;
    }

    Expression value = Expression_Compile(list->Get(2), env);
    if (!value) return nullptr;

    Type valueType = value->GetType();
    String name = list->Get(1)->GetValue();
    env.Allocate(name, valueType, value->IsConstant(env), true);
    locals->push(name);
    return Expression_DeclareStatic(value, name);
  }
}
