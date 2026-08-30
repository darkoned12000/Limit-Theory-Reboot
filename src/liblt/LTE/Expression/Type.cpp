#include "../Expressions.h"

#include "LTE/Environment.h"
#include "LTE/ProgramLog.h"
#include "LTE/Script.h"
#include "LTE/StringList.h"

#include "LTE/Debug.h"

#include <sstream>


namespace LTE {
  Expression Expression_Type(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() < 2) {
      env.ReportError(list, "'type' expects a name argument");
      return nullptr;
    }

    String const& name = list->Get(1)->GetValue();
    ScriptType type = env.script->types[name];

    if (type) {
      env.ReportError(list, Stringize()
        | "type '" | name | "' is already defined");
      return nullptr;
    }

    type = new ScriptTypeT;
    type->name = name;
    size_t alignment = 1;

    /* TODO : Unify with outer parsing. */
    /* TODO : Function declaration / definition separation. */

    for (size_t i = 2; i < list->GetSize(); ++i) {
      StringList const& sub = list->Get(i);
      String const& value = sub->Get(0)->GetValue();

      if (value == "#") {}

      else if (value == "function") {}

      else {
        if (sub->GetSize() < 2 ||
            sub->GetSize() > 3)
        {
          env.ReportError(sub,
            "type field must have 1 or 2 arguments (type name, [initializer])");
          return nullptr;
        }

        StringList const& typeName = sub->Get(0);
        Type fieldType = env.script->ResolveType(typeName);

        if (!fieldType) {
          env.ReportError(sub, Stringize()
            | "unknown type '" | typeName->GetString() | "' for field");
          return nullptr;
        }

        String const& fieldName = sub->Get(1)->GetValue();
        for (size_t j = 0; j < type->fields.size(); ++j) {
          if (fieldName == type->fields[j].name) {
            env.ReportError(sub, Stringize()
              | "duplicate field name '" | fieldName | "' in type '" | name | "'");
            return nullptr;
          }
        }
        
        Expression initializer;
        if (sub->GetSize() == 3) {
          CompileEnvironment subEnv;
          subEnv.script = env.script;

          initializer = Expression_Compile(sub->Get(2), subEnv);
          if (!initializer) {
            env.ReportError(sub, Stringize()
              | "initializer for field '" | fieldName
              | "' failed to compile");
            return nullptr;
          }

          initializer = Expression_Conversion(initializer, fieldType);
          if (!initializer) {
            env.ReportError(sub, Stringize()
              | "initializer for field '" | fieldName
              | "' cannot convert to type '" | fieldType->name | "'");
            return nullptr;
          }
        }

        type->initializers.push(initializer);

        /* Field padding. */ {
          size_t padding = type->size % fieldType->alignment;
          type->size += padding;
        }

        type->fields.push(Field(fieldName, fieldType, type->size));
        type->size += fieldType->size;
        alignment = Max(alignment, fieldType->alignment);
      }
    }

    /* Final padding. */
    if (type->size)
      type->size += type->size % alignment;

    /* TODO : Type leakage. */
    /* Build the reflected engine type and install the script-type hooks.
       Shared with the new pipeline (see ScriptType.cpp) so both compilers
       produce identical layout and runtime behavior. */
    ScriptType_CreateEngineType(type, alignment);

    env.script->types[name] = type;
    env.context.push(type);

    /* Pass A : pre-register every method's signature (with implicit 'this')
       before any method body compiles. This lets a method call a method that
       is declared LATER in the same type — without it, Expression_ExpressionCall
       fails the arity/type check at compile time, the statement is silently
       dropped, and the call never happens at runtime. */
    for (size_t i = 2; i < list->GetSize(); ++i) {
      StringList const& sub = list->Get(i);
      if (sub->Get(0)->GetValue() == "function") {
        ScriptFunction signature = ScriptFunction_ParseSignature(sub, env, type);
        if (!signature)
          return nullptr;
        type->functions[signature->name] = signature;
      }
    }

    /* Pass B : compile the method bodies into the pre-registered signatures. */
    for (size_t i = 2; i < list->GetSize(); ++i) {
      StringList const& sub = list->Get(i);
      if (sub->Get(0)->GetValue() == "function")
        Expression_Function(sub, env);
    }

    env.context.pop();
    return nullptr;
  }
}
