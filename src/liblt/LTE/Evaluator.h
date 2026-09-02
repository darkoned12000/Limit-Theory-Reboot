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

  // Call a specific script function with already-evaluated arguments.
  // This is the entry point used by ScriptFunctionT::Call when the engine
  // invokes a script function through the new compiler. `hasImplicitThis`
  // selects whether argument 0 is the receiver bound to `this` (true for
  // type methods) or simply the first declared parameter.
  Value CallFunction(ASTFuncDeclNodeT* func,
                     Vector<Value> const& args,
                     bool hasImplicitThis = false);

  // Attach the owning script so calls and member accesses can resolve script
  // functions and script-type fields. The Evaluator is otherwise a fresh
  // per-call object with no knowledge of its script. Defined in the .cpp
  // (out-of-line) so TUs including this header do not force instantiation of
  // Reference<ScriptT> without a complete ScriptT.
  void SetScript(ScriptT* ctx);

  /* --- Engine <-> Value marshalling -----------------------------------
     The engine's calling convention is raw memory slots (void*) tagged with
     an engine Type; the Evaluator works in Values. These convert between the
     two and are shared by ScriptFunctionT::Call and script-type field
     initialization.

     ValueFromSlot BORROWS the slot: the caller owns that memory for the
     duration of the call, so the returned Value must not free it. */
  static Value ValueFromSlot(Type t, void* data);
  static void ValueToSlot(Value const& v, Type t, void* dest);

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

  // Deep-own a borrowed engine value so persistent storage never aliases a
  // temporary that dies with the enclosing scope (e.g. `prevNode = node`
  // where `node` is a per-loop-iteration local).
  static Value OwnedCopy(Value const& v);

  // Statement evaluation
  Value EvalBlock(ASTBlockNodeT* block);
  Value EvalVarDecl(ASTDeclNodeT* decl);
  Value EvalFuncDecl(ASTFuncDeclNodeT* decl);
  Value EvalTypeDecl(ASTTypeDeclNodeT* decl);
  Value EvalIf(ASTIfNodeT* node);
  Value EvalWhile(ASTWhileNodeT* node);
  Value EvalFor(ASTForNodeT* node);
  Value EvalSwitch(ASTSwitchNodeT* node);
  Value EvalSwitchExpr(ASTSwitchExprNodeT* node);
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
  // Conversion to a scalar type (`(String x)`, `(Int x)`). Compensates for
  // the parser emitting lowercase type-cast heads as function calls instead
  // of cast nodes.
  Value ScalarCast(String const& typeName, Value const& val);
  Value EvalAddress(ASTAddressNodeT* node);
  Value EvalDeref(ASTDerefNodeT* node);
  Value EvalArrayLiteral(ASTArrayLiteralNodeT* node);
  Value EvalConstructor(ASTConstructorNodeT* node);
  Value EvalPrint(ASTPrintNodeT* node);

  // Engine function dispatch
  Value CallEngineFunction(String const& name, Vector<Value> const& args, SourceLocation const& loc = SourceLocation());

  // Script function dispatch
  Value CallScriptFunction(String const& name, Vector<Value> const& args, SourceLocation const& loc = SourceLocation());

  // Script-type field access. `recv` must be a PTR/CUSTOM value whose Type
  // carries a ScriptType in its Aux (created by ScriptType_CreateEngineType).
  // FieldGet returns NONE when `recv` has no such type or no such field;
  // FieldSet reports the write via its return value.
  ScriptType ScriptTypeOf(Value const& recv) const;
  Field const* FindField(ScriptType st, String const& name) const;
  Value FieldGet(Value const& recv, String const& name) const;
  bool FieldSet(Value const& recv, String const& name, Value const& value);

  // Build a default-constructed instance of a script type, boxed as a Data
  // (the legacy representation of a script value).
  Value MakeScriptTypeValue(ScriptType const& st);

  // `this`-relative field helpers (bare member names inside a method body).
  Value ThisFieldGet(String const& name) const;
  bool ThisFieldSet(String const& name, Value const& value);
  ScriptType ThisType() const;
  Value const* ThisValue() const;

  // Apply a compound assignment op to a stored field value.
  Value ApplyBinaryOp(String const& op, Value const& lhs, Value const& rhs);

  // `++`/`--` on a named variable or operand expression: mutate the scope slot
  // / field in place (the engine binding mutates by reference, which a
  // by-value call would discard) and return the new value.
  Value IncDecSlot(String const& op, String const& name);
  Value IncDecOperand(String const& op, ASTNode operand);

  // Script function dispatch by handle / name.
  Value CallScriptFunction(ScriptFunction const& sf,
                           Vector<Value> const& args);

  // Raw storage pointer for a Value, in the layout engine bindings expect
  // (primitives -> the union member, strings/heap values -> data).
  void* ValueArgPtr(Value const& v) const;

  // True when `name` resolves in any enclosing scope.
  bool ScopeHasName(String const& name) const;

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
