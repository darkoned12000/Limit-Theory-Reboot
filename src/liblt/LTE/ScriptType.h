#ifndef LTE_ScriptType_h__
#define LTE_ScriptType_h__

#include "Field.h"
#include "Map.h"
#include "ScriptFunction.h"
#include "Vector.h"

namespace LTE {
  using ScriptFunctionMapT = Map<String, ScriptFunction>;

  AutoClassDerived(ScriptTypeT, RefCounted,
    String, name,
    size_t, size,
    Vector<Field>, fields,
    Vector<Expression>, initializers,
    Type, type,
    ScriptFunctionMapT, functions)

    ScriptTypeT() : size(0) {}

    /* --- New compiler (AST) field initializers ------------------------
       `initializers` holds legacy Expression IR, which the new pipeline does
       not produce. Without a parallel AST vector, field defaults such as
       `Bool enabled true` would silently fall back to zero/false -- a quiet
       behavior change rather than a crash. Index-aligned with `fields`; an
       entry may be null when a field has no default. Plain member, not an
       AutoClass field, so the reflected shape is unchanged. */
    Vector<ASTNode> astInitializers;

    ScriptFunction GetFunction(String const& name) const {
      ScriptFunction const* fn = functions.get(name);
      return fn ? *fn : nullptr;
    }
  };

  /* Build the reflected engine Type for a laid-out script type and install the
     script-type runtime hooks (allocate/assign/construct/destruct/mapper/
     toString). Also assigns `type->type`.

     Shared by BOTH compilers: the legacy interpreter (Expression/Type.cpp) and
     the new pipeline (Script.cpp) must produce identical layout and runtime
     behavior, so the hook installation lives in exactly one place. */
  LT_API Type ScriptType_CreateEngineType(ScriptType const& type,
                                          size_t alignment = 1);
}

#endif
