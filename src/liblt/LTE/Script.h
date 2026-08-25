#ifndef LTE_Script_h__
#define LTE_Script_h__

#include "Array.h"
#include "ScriptType.h"

namespace LTE {
  using ScriptTypeMapT = Map<String, ScriptType>;
  
  AutoClassDerived(ScriptT, RefCounted,
    String, name,
    HashT, hash,
    ScriptFunctionMapT, functions,
    ScriptTypeMapT, types,
    Vector<Script>, dependencies)

    ScriptT() : hash(0) {}

    bool DependsOn(Script const& script) const {
      if (script.t == this)
        return true;
      for (size_t i = 0; i < dependencies.size(); ++i)
        if (dependencies[i]->DependsOn(script))
          return true;
      return false;
    }

    ScriptFunction GetFunction(String const& name) const {
      ScriptFunction const* fn = functions.get(name);
      return fn ? *fn : nullptr;
    }

    ScriptType GetType(String const& name) const {
      ScriptType const* type = types.get(name);
      return type ? *type : nullptr;
    }

    Script ResolveRelativePath(String const& path) const;

    Type ResolveType(StringList const& name) const;

    LT_API void Reload();
  };

  LT_API void Script_ClearCache();

  LT_API Script Script_Load(
    String const& name);

  /* Offline compile check — compiles like Reload() but returns structured
     errors instead of printing, bypassing the script cache. See
     Script_CompileCheck in Script.cpp for details. */
  LT_API bool Script_CompileCheck(
    String const& name,
    Vector<String>& errors);

  LT_API void Script_Reload(
    String const& name);

  LT_API ScriptFunction ScriptFunction_Load(
    String const& name);

  LT_API ScriptType ScriptType_Load(
    String const& name);
}

#endif
