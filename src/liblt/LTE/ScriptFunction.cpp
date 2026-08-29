#include "ScriptFunction.h"

#include "Environment.h"
#include "Evaluator.h"
#include "StackFrame.h"
#include "Type.h"

//#define ENABLE_SCRIPT_CACHING

#if 0

#ifdef ENABLE_SCRIPT_CACHING
      String cacheName = script->name;
      Hasher hash;
      for (size_t i = 0; i < args.size(); ++i)
        hash(args[i].data, args[i].type->size);

      cacheName += "_" + ToString((HashT)hash) + ".bin";
      Location cache = Location_Cache(cacheName);
      if (cache->Exists()) {
        debugprint;
        LoadFrom(result, cache, 0, 0);
        return;
      }
#endif

#ifdef ENABLE_SCRIPT_CACHING
      SaveTo(result, cache, 0);
#endif

#endif

namespace LTE {
  namespace {
    /* Marshalling between the engine's calling convention (void* slots tagged
       with an engine Type) and the Evaluator's Value representation.

       Argument slots are BORROWED from the caller: the engine owns that memory
       for the duration of the call, so the Value must not free it. */

    Value ValueFromArg(Type t, void* data) {
      if (!data)
        return Value::MakeNone();
      if (!t)
        return Value::MakeNone();

      /* Primitives are stored inline in the Value (no heap, no ownership). */
      if (t == Type_Find("Int"))
        return Value::MakeInt(*static_cast<int*>(data));
      if (t == Type_Find("Float"))
        return Value::MakeFloat(*static_cast<float*>(data));
      if (t == Type_Find("Bool"))
        return Value::MakeBool(*static_cast<bool*>(data));
      if (t == Type_Find("String"))
        return Value::MakeString(*static_cast<String*>(data));

      /* Engine-managed types (Vec3, Object, Widget, ...): borrow the slot. */
      return Value::MakePtr(t, data, /*owned*/ false);
    }

    void ValueToReturn(Value const& v, Type returnType, void* returnValue) {
      if (!returnValue)
        return;

      /* Nothing to write — leave the caller's slot untouched. */
      if (v.IsNone())
        return;

      if (returnType) {
        if (returnType == Type_Find("Int") && v.IsInt()) {
          *static_cast<int*>(returnValue) = v.AsInt();
          return;
        }
        if (returnType == Type_Find("Float") && v.IsFloat()) {
          *static_cast<float*>(returnValue) = v.AsFloat();
          return;
        }
        if (returnType == Type_Find("Bool") && v.IsBool()) {
          *static_cast<bool*>(returnValue) = v.AsBool();
          return;
        }
        if (returnType == Type_Find("String") && v.IsString()) {
          *static_cast<String*>(returnValue) = v.AsString();
          return;
        }
      }

      /* Engine-managed type: copy the evaluator's payload into the caller's
         slot via the type's assignment operator. */
      if (v.data && returnType && returnType->assign) {
        returnType->assign(returnType.t, v.data, returnValue);
        return;
      }
    }
  }

  void ScriptFunctionT::Call(void* returnValue, void** args) {
    /* --- New compiler path: evaluate the AST body with the Evaluator --- */
    if (astFunc) {
      FRAME(&name.front()) {
        Vector<LTE::Value> values;
        for (size_t i = 0; i < parameters.size(); ++i) {
          void* slot = args ? args[i] : nullptr;
          values.push(ValueFromArg(parameters[i].type, slot));
        }

        Evaluator eval;
        LTE::Value result = eval.CallFunction(astFunc, values, astImplicitThis);

        if (eval.HasErrors())
          eval.PrintErrors(name);

        ValueToReturn(result, returnType, returnValue);
      }
      return;
    }

    if (!expression)
      return;

    FRAME(&name.front()) {
      Environment env;
      env.registers.reserve(32);
      env.returnValue = returnValue;
      for (size_t i = 0; i < parameters.size(); ++i)
        env.registers.push(args[i]);
      expression->Evaluate(returnValue, env);
      /* A `return` inside the body set this flag to stop the body early;
         clear it so the caller (which may reuse this Environment) is not
         affected. A `return` only ever applies to the innermost function.
         Same for `break`: a top-level `break` (outside any loop) must not
         break an enclosing loop of the caller. */
      env.returnSignal = false;
      env.breakSignal = false;
    }
  }
}
