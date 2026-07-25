#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/ProgramLog.h"
#include "LTE/StringList.h"

namespace {
  AutoClassDerived(ExpressionDereference, ExpressionT,
    Expression, location,
    Type, type,
    Type, pointeeType)
    DERIVED_TYPE_EX(ExpressionDereference)
    POOLED_TYPE

    ExpressionDereference() = default;

    String Emit(Vector<String>& scope) const override {
      String label = location->Emit(scope);
      return "(*" + label + ")";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      void* lv = location->GetLValue(env);
      if (lv) {
        pointeeType->Assign(*(void**)lv, returnValue);
      } else {
        lv = env.Allocate(type);
        location->Evaluate(lv, env);
        pointeeType->Assign(*(void**)lv, returnValue);
        env.Free(type, lv);
      }
    }

    void* GetLValue(Environment& env) const override {
      void* lv = location->GetLValue(env);
      return lv ?  *(void**)lv : nullptr;
    }

    Type GetType() const override {
      return pointeeType;
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return false;
    }

    bool IsLValue() const override {
      return location->IsLValue();
    }
  };

  AutoClassDerived(ExpressionDereferencePointer, ExpressionT,
    Expression, location,
    Type, type)
    DERIVED_TYPE_EX(ExpressionDereferencePointer)
    POOLED_TYPE

    ExpressionDereferencePointer() = default;

    ExpressionDereferencePointer(Expression const& location) :
      location(location),
      type(location->GetType()->GetPointeeType())
      {}

    String Emit(Vector<String>& scope) const override {
      String label = location->Emit(scope);
      return "(*" + label + ")";
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      void* value;
      location->Evaluate(&value, env);
      type->Assign(value, returnValue);
    }

    void* GetLValue(Environment& env) const override {
      void* value;
      location->Evaluate(&value, env);
      return value;
    }

    Type GetType() const override {
      return type;
    }

    bool IsConstant(CompileEnvironment& env) const override {
      return false;
    }

    bool IsLValue() const override {
      return true;
    }
  };
}

namespace LTE {
  Expression Expression_Dereference(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() != 2) {
      env.ReportError(list, "'->' dereference expects 1 argument (pointer)");
      return nullptr;
    }

    Expression location = Expression_Compile(list->Get(1), env);
    if (!location) {
      env.ReportError(list, "'->' -- pointer expression failed to compile");
      return nullptr;
    }

    Type const& type = location->GetType();
    if (!type->GetPointeeType()) {
      env.ReportError(list, Stringize()
        | "'->' requires a pointer type, but got '" | type->name | "'");
      return nullptr;
    }

    Type const& pointeeType = type->GetPointeeType();
    /* TODO : Recursive dereferencing. */
    if (pointeeType->GetPointeeType()) {
      env.ReportError(list, Stringize()
        | "'->' on pointer-to-pointer type '" | type->name
        | "' is not yet supported");
      return nullptr;
    }

    String const& fieldName = list->Get(0)->GetValue();
    FieldType field = pointeeType->FindField(0, fieldName);
    if (!field.type) {
      env.ReportError(list, Stringize()
        | "type '" | pointeeType->name
        | "' (pointed-to by '" | type->name
        | "') has no field named '" | fieldName | "'");
      return nullptr;
    }

    return Expression_Access(
      new ExpressionDereference(location, type, pointeeType),
      (size_t)field.address, field.type, fieldName);
  }

  Expression Expression_DereferencePointer(Expression const& location) {
    return new ExpressionDereferencePointer(location);
  }

  Expression Expression_DereferencePointer(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() != 2) {
      env.ReportError(list, "'deref' expects 1 argument (pointer)");
      return nullptr;
    }

    Expression location = Expression_Compile(list->Get(1), env);
    if (!location) {
      env.ReportError(list, "'deref' -- expression failed to compile");
      return nullptr;
    }

    Type const& type = location->GetType();
    if (!type->GetPointeeType()) {
      env.ReportError(list, Stringize()
        | "'deref' requires a pointer type, but got '" | type->name | "'");
      return nullptr;
    }

    return Expression_DereferencePointer(location);
  }
}
