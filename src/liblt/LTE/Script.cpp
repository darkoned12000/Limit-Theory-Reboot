#include "Script.h"

#include "Environment.h"
#include "Evaluator.h"
#include "Lexer.h"
#include "Location.h"
#include "LTSL.h"
#include "Map.h"
#include "Parser.h"
#include "ProgramLog.h"
#include "StackFrame.h"
#include "StringList.h"
#include "SymbolResolver.h"
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
  namespace {
    /* Resolve an LTSL type name coming from the AST (e.g. "Int", "Widget",
       "List") into an engine Type. Script-defined types win, then the global
       engine registry. Falls back to Void so callers always get a Type. */
    Type ResolveAstType(ScriptT* script, String const& typeName) {
      if (!typeName.empty()) {
        ScriptType st = script->GetType(typeName);
        if (st && st->type)
          return st->type;
        Type t = Type_Find(typeName);
        if (t)
          return t;
      }
      return Type_Find("Void");
    }

    void BuildFunction(ScriptT* script,
                       ASTFuncDeclNodeT* fn,
                       ScriptType ownerType) {
      ScriptFunction sf = new ScriptFunctionT;
      sf->name = fn->name;
      sf->returnType = ResolveAstType(script, fn->returnType);

      /* Type methods receive the receiver as argument 0, bound to `this`,
         matching the old interpreter's calling convention. */
      if (ownerType) {
        sf->parameters.push(Parameter("this", ownerType->type));
        sf->astImplicitThis = true;
      }

      for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
        String pname = (i < fn->paramNames.size()) ? fn->paramNames[i] : String("p");
        sf->parameters.push(Parameter(pname, ResolveAstType(script, fn->paramTypes[i])));
      }

      sf->astFunc = fn;
      sf->astOwner = script;

      /* Methods live on the type, and are ALSO registered on the script so
         `Script:Method` lookups (Widget/Window:CreateChildren) resolve — the
         old interpreter exposed them this way (see migration plan, Open
         Blocker #1). */
      if (ownerType)
        ownerType->functions[fn->name] = sf;
      script->functions[fn->name] = sf;
    }

    void BuildType(ScriptT* script, ASTTypeDeclNodeT* td) {
      ScriptType type = new ScriptTypeT;
      type->name = td->name;

      size_t alignment = 1;

      /* Pass 1: lay out fields and collect methods. Methods are registered
         first so a method can reference the type (and other methods). */
      for (size_t i = 0; i < td->members.size(); ++i) {
        ASTNode member = td->members[i];
        if (!member) continue;
        /* Members are only fields (ASTDeclNodeT, one of the *_DECL kinds) or
           methods (AST_FUNC_DECL). */
        if (member->kind == AST_FUNC_DECL) continue;

        ASTDeclNodeT* decl = static_cast<ASTDeclNodeT*>(member.t);
        Type fieldType = ResolveAstType(script, decl->typeName);
        if (!fieldType) continue;

        /* Field padding, then the field occupies size bytes at the offset. */
        if (fieldType->alignment)
          type->size += type->size % fieldType->alignment;

        type->fields.push(Field(decl->name, fieldType, type->size));
        type->size += fieldType->size;
        alignment = Max(alignment, fieldType->alignment);

        /* One initializer slot per field, index-aligned with `fields`. */
        type->initializers.push(nullptr);
        type->astInitializers.push(decl->initializer);
      }

      if (type->size)
        type->size += type->size % alignment;

      /* Create the reflected engine type so instances can be allocated,
         assigned, constructed and mapped like any other engine type. Shared
         with the legacy interpreter so both produce identical layout. */
      ScriptType_CreateEngineType(type, alignment);

      script->types[type->name] = type;

      /* Pass 2: methods, now that the type (and its `this` type) exists. */
      for (size_t i = 0; i < td->members.size(); ++i) {
        ASTNode member = td->members[i];
        if (!member) continue;
        if (member->kind != AST_FUNC_DECL) continue;
        BuildFunction(script, static_cast<ASTFuncDeclNodeT*>(member.t), type);
      }
    }

    bool CompileWithNewPipeline(ScriptT* script,
                                Location const& location,
                                Vector<String>& errors) {
      errors.clear();

      String source = location->ReadAscii();
      if (source.empty()) {
        errors.push("empty script source");
        return false;
      }

      Lexer lexer(source);
      std::vector<Token> tokens = lexer.Tokenize();
      if (!lexer.GetErrors().empty()) {
        for (size_t i = 0; i < lexer.GetErrors().size(); ++i)
          errors.push(Stringize() | "line " | lexer.GetErrors()[i].line
                      | ": " | lexer.GetErrors()[i].message);
        return false;
      }

      Parser parser(tokens);
      ASTNode module = parser.Parse();
      if (!parser.GetErrors().empty()) {
        for (size_t i = 0; i < parser.GetErrors().size(); ++i)
          errors.push(Stringize() | "line " | parser.GetErrors()[i].line
                      | ": " | parser.GetErrors()[i].message);
        return false;
      }
      if (!module) {
        errors.push("parser produced no module");
        return false;
      }

      SymbolResolver resolver;
      if (!resolver.Resolve(module)) {
        for (size_t i = 0; i < resolver.GetErrors().size(); ++i)
          errors.push(Stringize() | "line " | resolver.GetErrors()[i].line
                      | ": " | resolver.GetErrors()[i].message);
        return false;
      }

      /* Own the AST for the lifetime of the script: function handles only
         keep back-pointers into it. */
      script->astModule = module;

      ASTModuleNodeT* mod = static_cast<ASTModuleNodeT*>(module.t);
      for (size_t i = 0; i < mod->statements.size(); ++i) {
        ASTNode stmt = mod->statements[i];
        if (!stmt) continue;
        if (stmt->kind == AST_FUNC_DECL)
          BuildFunction(script, static_cast<ASTFuncDeclNodeT*>(stmt.t), nullptr);
        else if (stmt->kind == AST_TYPE_DECL)
          BuildType(script, static_cast<ASTTypeDeclNodeT*>(stmt.t));
      }

      return true;
    }

    /* Selects the compiler. Old is still the default until the new pipeline
       has been verified end-to-end at runtime; set LTSL_NEW_COMPILER=1 to
       drive scripts through Lexer -> Parser -> SymbolResolver -> Evaluator. */
    bool UseNewCompiler() {
      char const* env = getenv("LTSL_NEW_COMPILER");
      return env && env[0] == '1';
    }
  }

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

    /* --- New compiler path (opt-in via LTSL_NEW_COMPILER=1) --- */
    if (UseNewCompiler()) {
      Vector<String> errors;
      FRAME(&name.front()) {
        if (!CompileWithNewPipeline(this, location, errors)) {
          for (size_t i = 0; i < errors.size(); ++i)
            Log_Error(Stringize() | name | ": " | errors[i]);
        }
      }
      return;
    }

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

  /* Offline compile check: compiles a script exactly as Reload() does but
     captures the diagnostics in `errors` instead of printing them, and
     bypasses the script cache so the result never depends on (or pollutes)
     prior load state. Cross-file references still resolve through the cache
     via ResolveRelativePath, so dependency failures surface on stdout from
     their own Reload while this file's own errors come back structured.
     Used by the compile-gate tool (tools/compile_gate.cpp) and unit tests:
     it is the engine-truth answer to "does this .lts compile?", which the
     LSP analyzer can only approximate (see AGENTS.md A.14 #14). */
  bool Script_CompileCheck(String const& name, Vector<String>& errors) {
    errors.clear();

    String scriptPath = name + kScriptExtension;
    Location location = Location_Script(scriptPath);
    if (!location->Exists()) {
      errors.push(Stringize() | "file not found: '" | scriptPath | "'");
      return false;
    }

    Script script = new ScriptT;
    script->name = name;

    /* New compiler path — mirrors Reload() so the compile gate reports on the
       pipeline that will actually run, bypassing the script cache exactly as
       the old path does. */
    if (UseNewCompiler())
      return CompileWithNewPipeline(script.t, location, errors);

    StringList list = StringList_Load(location);
    list = LTSL_ApplyRewrites(list);

    bool ok = true;
    FRAME(&name.front()) {
      CompileEnvironment env;
      env.script = script;
      for (size_t i = 0; i < list->GetSize(); ++i)
        Expression_Compile(list->Get(i), env);
      ok = !env.hasErrors;
      errors = env.errors;
    }
    return ok;
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
