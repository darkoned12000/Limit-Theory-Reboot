// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef LTE_Evaluator_h__
#define LTE_Evaluator_h__

#include "AST.h"
#include "Reference.h"
#include "ScriptType.h"
#include "String.h"
#include "Type.h"
#include "Vector.h"

namespace LTE {

// Runtime value — a discriminated union for all LTSL types.
// Uses void* payload for engine-managed types (Vec3, Object, etc.)
// with a Type tag so the evaluator knows how to handle lifetime.
struct Value {
  enum Kind {
    NONE,
    INT,
    FLOAT,
    BOOL,
    STRING,
    VEC2,
    VEC3,
    VEC4,
    OBJECT,
    TYPE_REF,   // a ScriptType* pointer (for constructors)
    FUNC_REF,   // a ScriptFunction* pointer (for first-class functions)
    ARRAY,      // an engine Array*
    PTR,        // a pointer/reference value
    CUSTOM      // any other engine type, stored as void*
  };

  Kind kind;
  Type type;       // engine Type for CUSTOM/ARRAY/OBJECT; null for primitives
  void* data;      // heap storage for non-primitive values
  bool owned;      // true if evaluator owns data and must free on destruction

  // Primitive storage (avoids heap allocation for scalars)
  union {
    int intVal;
    float floatVal;
    bool boolVal;
  };

  Value() : kind(NONE), type(nullptr), data(nullptr), owned(false) {
    intVal = 0;
  }

  // Primitive constructors
  static Value MakeInt(int v) { Value r; r.kind = INT; r.intVal = v; return r; }
  static Value MakeFloat(float v) { Value r; r.kind = FLOAT; r.floatVal = v; return r; }
  static Value MakeBool(bool v) { Value r; r.kind = BOOL; r.boolVal = v; return r; }
  static Value MakeNone() { return Value(); }

  // Engine type constructor (allocates and copies from data)
  static Value MakeEngine(Type t, void const* src);

  // Wrap existing pointer (takes ownership if `owned` is true)
  static Value MakePtr(Type t, void* ptr, bool owned);

  // String value
  static Value MakeString(String const& s);

  // Copy/move
  Value(Value const& other);
  Value& operator=(Value const& other);
  ~Value();

  // Release owned data without destroying the Value
  void Release();

  // Type checks
  bool IsNone() const { return kind == NONE; }
  bool IsInt() const { return kind == INT; }
  bool IsFloat() const { return kind == FLOAT; }
  bool IsBool() const { return kind == BOOL; }
  bool IsString() const { return kind == STRING; }

  // Extractors (callers must check kind first)
  int AsInt() const { return intVal; }
  float AsFloat() const { return floatVal; }
  bool AsBool() const { return boolVal; }
  String const& AsString() const;
  void* AsPtr() const { return data; }
};

// Forward declarations
struct Scope;

// Evaluator — walks the AST and produces runtime values.
// Uses scope chains for variable lookup, dispatches engine functions
// via Function_Find, and calls script functions recursively.
class Evaluator {
public:
  Evaluator();
  ~Evaluator();

  // Evaluate a full module (set of statements).
  // Returns the value of the last expression, or NONE.
  Value EvaluateModule(ASTModuleNodeT* module);

  // Evaluate a single expression.
  Value Evaluate(ASTNode node);

  // Error state
  bool HasErrors() const { return !errors.empty(); }
  Vector<String> const& GetErrors() const { return errors; }
  void PrintErrors(String const& scriptName) const;

private:
  // Scope management
  void PushScope();
  void PopScope();
  void Declare(String const& name, Value const& val, bool isRef = false);
  Value& Lookup(String const& name);

  // Statement evaluation
  Value EvalBlock(ASTBlockNodeT* block);
  Value EvalVarDecl(ASTDeclNodeT* decl);
  Value EvalFuncDecl(ASTFuncDeclNodeT* decl);
  Value EvalTypeDecl(ASTTypeDeclNodeT* decl);
  Value EvalIf(ASTIfNodeT* node);
  Value EvalWhile(ASTWhileNodeT* node);
  Value EvalFor(ASTForNodeT* node);
  Value EvalSwitch(ASTSwitchNodeT* node);
  Value EvalAssign(ASTAssignNodeT* node);
  Value EvalReturn(ASTReturnNodeT* node);
  Value EvalExprStmt(ASTExprStmtNodeT* node);

  // Expression evaluation
  Value EvalInt(ASTIntLiteralNodeT* node);
  Value EvalFloat(ASTFloatLiteralNodeT* node);
  Value EvalString(ASTStringLiteralNodeT* node);
  Value EvalBool(ASTBoolLiteralNodeT* node);
  Value EvalNull(ASTNullLiteralNodeT* node);
  Value EvalIdentifier(ASTIdentifierNodeT* node);
  Value EvalBinaryOp(ASTBinaryOpNodeT* node);
  Value EvalUnaryOp(ASTUnaryOpNodeT* node);
  Value EvalMethodCall(ASTMethodCallNodeT* node);
  Value EvalFuncCall(ASTFuncCallNodeT* node);
  Value EvalCast(ASTCastNodeT* node);
  Value EvalAddress(ASTAddressNodeT* node);
  Value EvalDeref(ASTDerefNodeT* node);
  Value EvalArrayLiteral(ASTArrayLiteralNodeT* node);
  Value EvalConstructor(ASTConstructorNodeT* node);
  Value EvalPrint(ASTPrintNodeT* node);

  // Engine function dispatch
  Value CallEngineFunction(String const& name, Vector<Value> const& args);

  // Script function dispatch
  Value CallScriptFunction(String const& name, Vector<Value> const& args);

  // Engine function lookup
  Vector<Function> FindFunctions(String const& name);

  // Error reporting
  void RuntimeError(String const& message);
  void RuntimeError(String const& message, SourceLocation const& loc);

  // Control flow signals
  enum FlowSignal {
    FLOW_NONE,
    FLOW_RETURN,
    FLOW_BREAK
  };
  FlowSignal flowSignal;
  Value flowValue;

  // Scope chain
  Reference<Scope> currentScope;

  // Script context (for function lookup)
  Reference<ScriptT> script;

  // Errors collected during evaluation
  Vector<String> errors;
};

} // namespace LTE

#endif
