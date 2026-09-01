#ifndef LTE_ScriptFunction_h__
#define LTE_ScriptFunction_h__

#include "AST.h"
#include "AutoClass.h"
#include "Expression.h"
#include "String.h"
#include "Vector.h"

namespace LTE {
  struct ScriptT;   // owner script; defined in Script.h (included via ScriptType.h)

  AutoClassDerived(ScriptFunctionT, RefCounted,
    String, name,
    Expression, expression,
    Type, returnType,
    Vector<Parameter>, parameters,
    Function, function)

    ScriptFunctionT() = default;

    /* --- New compiler (AST) path ----------------------------------------
       When `astFunc` is set, Call() evaluates the function body with the
       Evaluator instead of the legacy `Expression` tree. These are plain
       members (deliberately NOT part of the AutoClass field list) so the
       reflected/serialized shape of ScriptFunctionT is unchanged and no
       circular type dependency is introduced.

       astFunc         — AST node of the function body to evaluate.
       astOwner        — script that owns this function (raw back-pointer;
                         lifetime is owned by ScriptT, never by this handle).
       astImplicitThis — true for type methods: the receiver is passed as
                         argument 0 and bound to `this`, matching the old
                         interpreter's calling convention. */
    ASTFuncDeclNodeT* astFunc = nullptr;
    ScriptT* astOwner = nullptr;
    bool astImplicitThis = false;

    LT_API void Call(void* returnValue, void** args);

    template <class T>
    void Call(T& returnValue) {
      LTE_ASSERT(returnType == Type_Get<T>())
      LTE_ASSERT(parameters.size() == 0)

      Call(&returnValue, (void**)nullptr);
    }

    template <class T, class P0>
    void Call(T& returnValue, P0 const& p0) {
      LTE_ASSERT(returnType == Type_Get<T>())
      LTE_ASSERT(parameters.size() == 1)
      LTE_ASSERT(parameters[0].type == Type_Get<P0>());

      void* args[] = { (void*)&p0 };
      Call(&returnValue, args);
    }

    template <class T, class P0, class P1>
    void Call(T& returnValue, P0 const& p0, P1 const& p1) {
      LTE_ASSERT(returnType == Type_Get<T>())
      LTE_ASSERT(parameters.size() == 2)
      LTE_ASSERT(parameters[0].type == Type_Get<P0>());
      LTE_ASSERT(parameters[1].type == Type_Get<P1>());

      void* args[] = { (void*)&p0, (void*)&p1 };
      Call(&returnValue, args);
    }

    template <class T, class P0, class P1, class P2>
    void Call(T& returnValue, P0 const& p0, P1 const& p1, P2 const& p2) {
      LTE_ASSERT(returnType == Type_Get<T>())
      LTE_ASSERT(parameters.size() == 3)
      LTE_ASSERT(parameters[0].type == Type_Get<P0>());
      LTE_ASSERT(parameters[1].type == Type_Get<P1>());
      LTE_ASSERT(parameters[2].type == Type_Get<P2>());

      void* args[] = { (void*)&p0, (void*)&p1, (void*)&p2 };
      Call(&returnValue, args);
    }

    void VoidCall(void* returnValue) {
      LTE_ASSERT(parameters.size() == 0)

      bool constructRV = !returnValue && returnType && returnType->allocate;
      if (constructRV) returnValue = returnType->Allocate();
      Call(returnValue, (void**)nullptr);
      if (constructRV) returnType->Deallocate(returnValue);
    }

    void VoidCall(
      void* returnValue,
      DataRef const& p0)
    {
      LTE_ASSERT(parameters.size() == 1)
      LTE_ASSERT(parameters[0].type == p0.type)

      void* args[] = { p0.data };
      bool constructRV = !returnValue && returnType && returnType->allocate;
      if (constructRV) returnValue = returnType->Allocate();
      Call(returnValue, args);
      if (constructRV) returnType->Deallocate(returnValue);
    }

    void VoidCall(
      void* returnValue,
      DataRef const& p0,
      DataRef const& p1)
    {
      LTE_ASSERT(parameters.size() == 2)
      LTE_ASSERT(parameters[0].type == p0.type)
      LTE_ASSERT(parameters[1].type == p1.type)

      void* args[] = { p0.data, p1.data };
      bool constructRV = !returnValue && returnType && returnType->allocate;
      if (constructRV) returnValue = returnType->Allocate();
      Call(returnValue, args);
      if (constructRV) returnType->Deallocate(returnValue);
    }

    void VoidCall(
      void* returnValue,
      DataRef const& p0,
      DataRef const& p1,
      DataRef const& p2)
    {
      LTE_ASSERT(parameters.size() == 3)
      LTE_ASSERT(parameters[0].type == p0.type)
      LTE_ASSERT(parameters[1].type == p1.type)
      LTE_ASSERT(parameters[2].type == p2.type)

      void* args[] = { p0.data, p1.data, p2.data };
      bool constructRV = !returnValue && returnType && returnType->allocate;
      if (constructRV) returnValue = returnType->Allocate();
      Call(returnValue, args);
      if (constructRV) returnType->Deallocate(returnValue);
    }
  };
}

#endif
