#include "Script.h"

#include "Environment.h"
#include "Location.h"
#include "LTSL.h"
#include "Map.h"
#include "ProgramLog.h"
#include "StackFrame.h"
#include "StringList.h"
#include "Types.h"
#include "LTE/FunctionBind.h"

#define AUTORELOAD 1

char const* const kScriptExtension = ".lts";

namespace {
  using ScriptCacheT = Map<String, Script>;

  ScriptCacheT& GetScriptCache() {
    static ScriptCacheT cache;
    return cache;
  }

  Script& GetCache(String const& name) {
    return GetScriptCache()[name];
  }
}

namespace LTE {
  void ScriptT::Reload() {
    String scriptPath = name + kScriptExtension;
    Location location = Location_Script(scriptPath);
    if (!location->Exists()) {
      Log_Error(Stringize()
        | "Script file not found: '" | scriptPath | "'");
      return;
    }

    HashT hash = Max((HashT)1, location->GetHash());
    if (hash == this->hash)
      return;
    this->hash = hash;

    functions.clear();
    types.clear();
    dependencies.clear();

    StringList list = StringList_Load(location);
    list = LTSL_ApplyRewrites(list);

    FRAME(&name.front()) {
      CompileEnvironment env;
      env.script = this;
      for (size_t i = 0; i < list->GetSize(); ++i)
        Expression_Compile(list->Get(i), env);
      if (env.hasErrors)
        env.PrintErrors(name);
    }
  }

  Script ScriptT::ResolveRelativePath(String const& path) const {

    Vector<String> pathComponents;
    String_Split(pathComponents, name, '/');

    Script script = nullptr;

    /* First try the exact path from root — allows cross-directory imports.
       Check existence first to suppress errors during ancestor search loop below. */
    String scriptPathRoot = path + kScriptExtension;
    Location locationRoot = Location_Script(scriptPathRoot);
    if (locationRoot->Exists()) {
      script = Script_Load(path);
      return script;
    }

    /* Then search relative to this script's directory and ancestors upward.
       Check existence before loading to avoid noise/errors for expected misses. */
    for (size_t i = 0; i < pathComponents.size() && !script; ++i) {
      String potentialName;
      size_t depth = pathComponents.size() - i;
      for (size_t j = 0; j < depth; ++j) {
        if (!potentialName.empty()) potentialName += "/";
        potentialName += pathComponents[j];
      }
      potentialName += "/" + path;

      String scriptPath = potentialName + kScriptExtension;
      Location location = Location_Script(scriptPath);
      if (location->Exists()) {
        script = Script_Load(potentialName);
      }
    }

    return script;
  }

  Type ScriptT::ResolveType(StringList const& list) const {
    if (list->IsAtom()) {
      String const& name = list->GetValue();

      /* Cross-script lookup. */
      if (name.contains(':')) {
        Vector<String> strings;
        String_Split(strings, name, ':');
        String const& scriptName = strings[0];
        String const& typeName = strings[1];

        Script script = ResolveRelativePath(scriptName);
        if (!script)
          return nullptr;

        ScriptType t = script->GetType(typeName);
        if (t) {
          Mutable(dependencies).push(script);
          return t->type;
        }
      }

      /* In-script lookup. */
      ScriptType type = GetType(name);
      if (type)
        return type->type;

      /* Global lookup. */
      return Type_Find(name);
    }

    if (list->GetSize() == 0)
      return nullptr;

    String const& value = list->Get(0)->GetValue();
    if (value == "Array") {
      if (list->GetSize() != 2)
        return nullptr;
      Type elemType = ResolveType(list->Get(1));
      if (!elemType)
        return nullptr;
      return Type_Array(elemType);
    }

    else if (value == "Pointer") {
      if (list->GetSize() != 2)
        return nullptr;
      Type elemType = ResolveType(list->Get(1));
      if (!elemType)
        return nullptr;
      return Type_Pointer(elemType);
    }

    return nullptr;
  }

  void Script_ClearCache() {
    GetScriptCache().clear();
  }
static Function const Script_ClearCache_Registration = Function_Bind(
  "Script_ClearCache",
  "None",
  &Script_ClearCache);



  Script Script_Load(String const& name) {
    Script& script = GetCache(name);
    if (script)
      return script;

    String scriptPath = name + kScriptExtension;
    Location location = Location_Script(scriptPath);
    if (!location->Exists()) {
      Log_Error(Stringize()
        | "Script file not found: '" | scriptPath | "'");
      return nullptr;
    }

    script = new ScriptT;
    script->name = name;
    script->Reload();
    return script;
  }
static Function const Script_Load_Registration = Function_Bind(
  "Script_Load",
  "None",
  &Script_Load,
  "name");



  void Script_Reload(String const& name) {
    bool loaded = GetCache(name) != nullptr;
    Script script = Script_Load(name);
    if (loaded) {
      Vector<Script> scripts;
      scripts.push(script);
      for (size_t i = 0; i < scripts.size(); ++i) {
        Script script = scripts[i];
        for (size_t j = 0; j < script->dependencies.size(); ++j)
          if (!scripts.contains(script->dependencies[j]))
            scripts.push(script->dependencies[j]);
      }

      for (size_t i = 0; i < scripts.size(); ++i)
        scripts[i]->Reload();
    }
  }
static Function const Script_Reload_Registration = Function_Bind(
  "Script_Reload",
  "None",
  &Script_Reload,
  "name");



  ScriptFunction ScriptFunction_Load(String const& name) {
    Vector<String> strings;
    String_Split(strings, name, ':');
    if (strings.size() != 2) {
      Log_Error(Stringize()
        | "ScriptFunction_Load received bad path '" | name
        | "' (expected 'ScriptName:functionName')");
      return nullptr;
    }

    String const& scriptName = strings[0];
    String const& functionName = strings[1];

    Script script = Script_Load(scriptName);
    if (!script) {
      Log_Error(Stringize()
        | "Failed to load script '" | scriptName
        | "' (required by function '" | functionName | "')");
      return nullptr;
    }

#if AUTORELOAD
    Script_Reload(scriptName);
#endif

    ScriptFunction fn = script->GetFunction(functionName);
    if (!fn) {
      Log_Error(Stringize()
        | "Script '" | scriptName
        | "' does not contain function '" | functionName | "'");
      return nullptr;
    }

    return fn;
  }
static Function const ScriptFunction_Load_Registration = Function_Bind(
  "ScriptFunction_Load",
  "None",
  &ScriptFunction_Load,
  "name");



  ScriptType ScriptType_Load(String const& name) {
    Vector<String> strings;
    String_Split(strings, name, ':');
    if (strings.size() != 2) {
      Log_Error(Stringize()
        | "ScriptType_Load received bad path '" | name
        | "' (expected 'ScriptName:typeName')");
      return nullptr;
    }

    String const& scriptName = strings[0];
    String const& typeName = strings[1];

    Script script = Script_Load(scriptName);
    if (!script) {
      Log_Error(Stringize()
        | "Failed to load script '" | scriptName
        | "' (required by type '" | typeName | "')");
      return nullptr;
    }

    ScriptType type = script->GetType(typeName);
    if (!type) {
      Log_Error(Stringize()
        | "Script '" | scriptName
        | "' does not contain type '" | typeName | "'");
      return nullptr;
    }

    return type;
  }
static Function const ScriptType_Load_Registration = Function_Bind(
  "ScriptType_Load",
  "None",
  &ScriptType_Load,
  "name");


}
