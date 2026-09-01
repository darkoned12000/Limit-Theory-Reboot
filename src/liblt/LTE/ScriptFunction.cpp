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
  void ScriptFunctionT::Call(void* returnValue, void** args) {
    /* --- New compiler path: evaluate the AST body with the Evaluator --- */
    if (astFunc) {
      FRAME(&name.front()) {
        Vector<Value> values;
        for (size_t i = 0; i < parameters.size(); ++i) {
          void* slot = args ? args[i] : nullptr;
          values.push(Evaluator::ValueFromSlot(parameters[i].type, slot));
        }

        Evaluator eval;
        eval.SetScript(astOwner);
        Value result = eval.CallFunction(astFunc, values, astImplicitThis);

        if (eval.HasErrors())
          eval.PrintErrors(name);

        Evaluator::ValueToSlot(result, returnType, returnValue);
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
