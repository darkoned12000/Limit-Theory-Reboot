// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Evaluator.h"
#include "AST.h"
#include "Function.h"
#include "Map.h"
#include "Parameter.h"
#include "Script.h"
#include "Stack.h"
#include "Type.h"

#include <iostream>

namespace LTE {

// ─── Value implementation ──────────────────────────────────────────

Value Value::MakeEngine(Type t, void const* src) {
  Value r;
  r.kind = CUSTOM;
  r.type = t;
  r.owned = true;
  r.data = t->Allocate();
  if (src)
    t->Assign(r.data, const_cast<void*>(src));
  return r;
}

Value Value::MakePtr(Type t, void* ptr, bool own) {
  Value r;
  r.kind = PTR;
  r.type = t;
  r.data = ptr;
  r.owned = own;
  return r;
}

Value Value::MakeString(String const& s) {
  Value r;
  r.kind = STRING;
  // Store String in data via heap allocation
  String* heapStr = new String(s);
  r.data = heapStr;
  r.owned = true;
  return r;
}

Value::Value(Value const& other)
  : kind(other.kind), type(other.type), data(nullptr), owned(false),
    intVal(other.intVal)
{
  if (other.kind == STRING && other.data) {
    String* src = static_cast<String*>(other.data);
    String* dst = new String(*src);
    data = dst;
    owned = true;
  } else if (other.owned && other.data && other.type) {
    data = other.type->Allocate();
    other.type->Assign(data, other.data);
    owned = true;
  } else {
    data = other.data;
    owned = false;
  }
}

Value& Value::operator=(Value const& other) {
  if (this != &other) {
    Release();
    kind = other.kind;
    type = other.type;
    intVal = other.intVal;
    owned = false;
    data = nullptr;

    if (other.kind == STRING && other.data) {
      String* src = static_cast<String*>(other.data);
      String* dst = new String(*src);
      data = dst;
      owned = true;
    } else if (other.owned && other.data && other.type) {
      data = other.type->Allocate();
      other.type->Assign(data, other.data);
      owned = true;
    } else {
      data = other.data;
    }
  }
  return *this;
}

Value::~Value() { Release(); }

void Value::Release() {
  if (owned && data) {
    if (kind == STRING) {
      delete static_cast<String*>(data);
    } else if (type) {
      type->Deallocate(data);
    }
    data = nullptr;
  }
  if (kind != STRING) {
    kind = NONE;
    type = nullptr;
    owned = false;
  }
}

String const& Value::AsString() const {
  static String empty;
  if (kind == STRING && data)
    return *static_cast<String const*>(data);
  return empty;
}

// ─── Scope ─────────────────────────────────────────────────────────

struct Scope : public RefCounted {
  Reference<Scope> parent;
  Map<String, Value> locals;
  Map<String, bool> refs;  // track which locals are ref-qualified

  Scope() = default;
  Scope(Scope* p) : parent(p) {}
};

// ─── Evaluator ─────────────────────────────────────────────────────

Evaluator::Evaluator()
  : flowSignal(FLOW_NONE), currentScope(nullptr), script(nullptr)
{
  currentScope = new Scope();
}

Evaluator::~Evaluator() {}

Value Evaluator::EvaluateModule(ASTModuleNodeT* module) {
  if (!module)
    return Value::MakeNone();

  Value last = Value::MakeNone();
  for (size_t i = 0; i < module->statements.size(); ++i) {
    last = Evaluate(module->statements[i]);
    if (flowSignal != FLOW_NONE)
      break;
  }
  return last;
}

Value Evaluator::Evaluate(ASTNode node) {
  if (!node)
    return Value::MakeNone();

  switch (node->kind) {
    case AST_MODULE:       return EvalBlock(static_cast<ASTBlockNodeT*>(node.t));
    case AST_BLOCK:        return EvalBlock(static_cast<ASTBlockNodeT*>(node.t));
    case AST_FUNC_DECL:    return EvalFuncDecl(static_cast<ASTFuncDeclNodeT*>(node.t));
    case AST_TYPE_DECL:    return EvalTypeDecl(static_cast<ASTTypeDeclNodeT*>(node.t));
    case AST_RETURN:       return EvalReturn(static_cast<ASTReturnNodeT*>(node.t));
    case AST_IF:           return EvalIf(static_cast<ASTIfNodeT*>(node.t));
    case AST_WHILE:        return EvalWhile(static_cast<ASTWhileNodeT*>(node.t));
    case AST_FOR:          return EvalFor(static_cast<ASTForNodeT*>(node.t));
    case AST_SWITCH:       return EvalSwitch(static_cast<ASTSwitchNodeT*>(node.t));
    case AST_VAR_DECL:
    case AST_REF_DECL:
    case AST_STATIC_DECL:  return EvalVarDecl(static_cast<ASTDeclNodeT*>(node.t));
    case AST_ASSIGN:       return EvalAssign(static_cast<ASTAssignNodeT*>(node.t));
    case AST_EXPR_STMT:    return EvalExprStmt(static_cast<ASTExprStmtNodeT*>(node.t));
    case AST_INT_LITERAL:          return EvalInt(static_cast<ASTIntLiteralNodeT*>(node.t));
    case AST_FLOAT_LITERAL:        return EvalFloat(static_cast<ASTFloatLiteralNodeT*>(node.t));
    case AST_STRING_LITERAL:       return EvalString(static_cast<ASTStringLiteralNodeT*>(node.t));
    case AST_BOOL_LITERAL:         return EvalBool(static_cast<ASTBoolLiteralNodeT*>(node.t));
    case AST_NULL_LITERAL:         return EvalNull(static_cast<ASTNullLiteralNodeT*>(node.t));
    case AST_IDENTIFIER:   return EvalIdentifier(static_cast<ASTIdentifierNodeT*>(node.t));
    case AST_BINARY_OP:    return EvalBinaryOp(static_cast<ASTBinaryOpNodeT*>(node.t));
    case AST_UNARY_OP:     return EvalUnaryOp(static_cast<ASTUnaryOpNodeT*>(node.t));
    case AST_METHOD_CALL:  return EvalMethodCall(static_cast<ASTMethodCallNodeT*>(node.t));
    case AST_FUNC_CALL:    return EvalFuncCall(static_cast<ASTFuncCallNodeT*>(node.t));
    case AST_CAST:         return EvalCast(static_cast<ASTCastNodeT*>(node.t));
    case AST_ADDRESS:      return EvalAddress(static_cast<ASTAddressNodeT*>(node.t));
    case AST_DEREF:        return EvalDeref(static_cast<ASTDerefNodeT*>(node.t));
    case AST_ARRAY_LITERAL:return EvalArrayLiteral(static_cast<ASTArrayLiteralNodeT*>(node.t));
    case AST_CONSTRUCTOR:  return EvalConstructor(static_cast<ASTConstructorNodeT*>(node.t));
    case AST_PRINT:        return EvalPrint(static_cast<ASTPrintNodeT*>(node.t));
    case AST_BREAK:
      flowSignal = FLOW_BREAK;
      return Value::MakeNone();
    default:
      RuntimeError("unhandled AST node kind");
      return Value::MakeNone();
  }
}

// ─── Scope management ──────────────────────────────────────────────

void Evaluator::PushScope() {
  currentScope = new Scope(currentScope.t);
}

void Evaluator::PopScope() {
  if (currentScope.t)
    currentScope = currentScope->parent.t;
}

void Evaluator::Declare(String const& name, Value const& val, bool isRef) {
  if (!currentScope.t) {
    RuntimeError("declare outside scope");
    return;
  }
  currentScope->locals[name] = val;
  currentScope->refs[name] = isRef;
}

Value& Evaluator::Lookup(String const& name) {
  Scope* s = currentScope.t;
  while (s) {
    Value* v = s->locals.get(name);
    if (v)
      return *v;
    s = s->parent.t;
  }
  static Value none;
  none = Value::MakeNone();
  return none;
}

// ─── Statement evaluation ──────────────────────────────────────────

Value Evaluator::EvalBlock(ASTBlockNodeT* block) {
  if (!block)
    return Value::MakeNone();

  PushScope();
  Value last = Value::MakeNone();
  for (size_t i = 0; i < block->statements.size(); ++i) {
    last = Evaluate(block->statements[i]);
    if (flowSignal != FLOW_NONE)
      break;
  }
  PopScope();
  return last;
}

Value Evaluator::EvalVarDecl(ASTDeclNodeT* decl) {
  if (!decl)
    return Value::MakeNone();

  Value init = Value::MakeNone();
  if (decl->initializer.t)
    init = Evaluate(decl->initializer);

  Declare(decl->name, init, decl->kind == AST_REF_DECL);
  return init;
}

Value Evaluator::EvalFuncDecl(ASTFuncDeclNodeT* decl) {
  if (!decl)
    return Value::MakeNone();

  // Store function reference in scope
  Value fnVal;
  fnVal.kind = Value::FUNC_REF;
  fnVal.data = decl;
  fnVal.owned = false;
  Declare(decl->name, fnVal);
  return Value::MakeNone();
}

Value Evaluator::EvalTypeDecl(ASTTypeDeclNodeT* decl) {
  if (!decl)
    return Value::MakeNone();

  // Type declarations are handled by the script system
  // For now, just store the type name
  Value tv;
  tv.kind = Value::TYPE_REF;
  tv.data = decl;
  tv.owned = false;
  Declare(decl->name, tv);
  return Value::MakeNone();
}

Value Evaluator::EvalIf(ASTIfNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value cond = Evaluate(node->condition);
  if (cond.IsBool() && cond.AsBool()) {
    return Evaluate(node->thenBlock);
  } else if (node->elseBlock.t) {
    return Evaluate(node->elseBlock);
  }
  return Value::MakeNone();
}

Value Evaluator::EvalWhile(ASTWhileNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value last = Value::MakeNone();
  while (true) {
    Value cond = Evaluate(node->condition);
    if (!cond.IsBool() || !cond.AsBool())
      break;

    last = Evaluate(node->body);
    if (flowSignal == FLOW_BREAK) {
      flowSignal = FLOW_NONE;
      break;
    }
    if (flowSignal == FLOW_RETURN)
      break;
  }
  return last;
}

Value Evaluator::EvalFor(ASTForNodeT* node) {
  if (!node)
    return Value::MakeNone();

  PushScope();
  Value init = Evaluate(node->init);
  Value last = Value::MakeNone();

  while (true) {
    Value cond = Evaluate(node->condition);
    if (!cond.IsBool() || !cond.AsBool())
      break;

    last = Evaluate(node->body);
    if (flowSignal == FLOW_BREAK) {
      flowSignal = FLOW_NONE;
      break;
    }
    if (flowSignal == FLOW_RETURN)
      break;

    Evaluate(node->step);
  }
  PopScope();
  return last;
}

Value Evaluator::EvalSwitch(ASTSwitchNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value expr = Evaluate(node->expression);
  for (size_t i = 0; i < node->cases.size(); ++i) {
    Value caseVal = Evaluate(node->cases[i].condition);
    // Simple equality check (works for int/float/string)
    bool match = false;
    if (expr.IsInt() && caseVal.IsInt())
      match = expr.AsInt() == caseVal.AsInt();
    else if (expr.IsFloat() && caseVal.IsFloat())
      match = expr.AsFloat() == caseVal.AsFloat();
    else if (expr.IsBool() && caseVal.IsBool())
      match = expr.AsBool() == caseVal.AsBool();

    if (match)
      return Evaluate(node->cases[i].body);
  }
  if (node->otherwise.t)
    return Evaluate(node->otherwise);
  return Value::MakeNone();
}

Value Evaluator::EvalAssign(ASTAssignNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value rhs = Evaluate(node->value);
  // node->target is an ASTNode (usually an identifier) for `x` in `x = 1`
  // We need to get the identifier name for Lookup
  String targetName;
  if (node->target && node->target->kind == AST_IDENTIFIER) {
    targetName = static_cast<ASTIdentifierNodeT*>(node->target.t)->name;
  } else if (node->target) {
    // For complex lvalues like `a.b`, use the full target as string (fallback)
    // For now, just try to evaluate the target as identifier
    // This handles `x` in `x = 1`, but for `a.b = 1`, it would need more handling
    // For simplicity, assume target is identifier
    if (node->target->kind == AST_IDENTIFIER) {
      targetName = static_cast<ASTIdentifierNodeT*>(node->target.t)->name;
    } else {
      fprintf(stderr, "unsupported assignment target\n");
      return Value::MakeNone();
    }
  }
  Value* target = &Lookup(targetName);

  if (node->op == "=") {
    *target = rhs;
  } else if (node->op == "+=") {
    if (target->IsInt() && rhs.IsInt())
      *target = Value::MakeInt(target->AsInt() + rhs.AsInt());
    else if (target->IsFloat() && rhs.IsFloat())
      *target = Value::MakeFloat(target->AsFloat() + rhs.AsFloat());
  } else if (node->op == "-=") {
    if (target->IsInt() && rhs.IsInt())
      *target = Value::MakeInt(target->AsInt() - rhs.AsInt());
    else if (target->IsFloat() && rhs.IsFloat())
      *target = Value::MakeFloat(target->AsFloat() - rhs.AsFloat());
  } else if (node->op == "*=") {
    if (target->IsInt() && rhs.IsInt())
      *target = Value::MakeInt(target->AsInt() * rhs.AsInt());
    else if (target->IsFloat() && rhs.IsFloat())
      *target = Value::MakeFloat(target->AsFloat() * rhs.AsFloat());
  } else if (node->op == "/=") {
    if (target->IsInt() && rhs.IsInt() && rhs.AsInt() != 0)
      *target = Value::MakeInt(target->AsInt() / rhs.AsInt());
    else if (target->IsFloat() && rhs.IsFloat())
      *target = Value::MakeFloat(target->AsFloat() / rhs.AsFloat());
  }

  return *target;
}

Value Evaluator::EvalReturn(ASTReturnNodeT* node) {
  if (!node)
    return Value::MakeNone();

  flowSignal = FLOW_RETURN;
  if (node->value.t)
    flowValue = Evaluate(node->value);
  else
    flowValue = Value::MakeNone();
  return flowValue;
}

Value Evaluator::EvalExprStmt(ASTExprStmtNodeT* node) {
  if (!node)
    return Value::MakeNone();
  return Evaluate(node->expression);
}

// ─── Expression evaluation ─────────────────────────────────────────

Value Evaluator::EvalInt(ASTIntLiteralNodeT* node) {
  return node ? Value::MakeInt(node->value) : Value::MakeNone();
}

Value Evaluator::EvalFloat(ASTFloatLiteralNodeT* node) {
  return node ? Value::MakeFloat(node->value) : Value::MakeNone();
}

Value Evaluator::EvalString(ASTStringLiteralNodeT* node) {
  return node ? Value::MakeString(node->value) : Value::MakeNone();
}

Value Evaluator::EvalBool(ASTBoolLiteralNodeT* node) {
  return node ? Value::MakeBool(node->value) : Value::MakeNone();
}

Value Evaluator::EvalNull(ASTNullLiteralNodeT*) {
  return Value::MakeNone();
}

Value Evaluator::EvalIdentifier(ASTIdentifierNodeT* node) {
  if (!node)
    return Value::MakeNone();
  return Lookup(node->name);
}

Value Evaluator::EvalBinaryOp(ASTBinaryOpNodeT* node) {
  if (!node)
    return Value::MakeNone();

  // Short-circuit for logical operators
  if (node->op == "&&") {
    Value left = Evaluate(node->left);
    if (left.IsBool() && !left.AsBool())
      return Value::MakeBool(false);
    Value right = Evaluate(node->right);
    return Value::MakeBool(right.IsBool() && right.AsBool());
  }
  if (node->op == "||") {
    Value left = Evaluate(node->left);
    if (left.IsBool() && left.AsBool())
      return Value::MakeBool(true);
    Value right = Evaluate(node->right);
    return Value::MakeBool(right.IsBool() && right.AsBool());
  }

  Value left = Evaluate(node->left);
  Value right = Evaluate(node->right);

  // Integer operations
  if (left.IsInt() && right.IsInt()) {
    int l = left.AsInt(), r = right.AsInt();
    if (node->op == "+")  return Value::MakeInt(l + r);
    if (node->op == "-")  return Value::MakeInt(l - r);
    if (node->op == "*")  return Value::MakeInt(l * r);
    if (node->op == "/")  return Value::MakeInt(r != 0 ? l / r : 0);
    if (node->op == "%")  return Value::MakeInt(r != 0 ? l % r : 0);
    if (node->op == "<")  return Value::MakeBool(l < r);
    if (node->op == ">")  return Value::MakeBool(l > r);
    if (node->op == "<=") return Value::MakeBool(l <= r);
    if (node->op == ">=") return Value::MakeBool(l >= r);
    if (node->op == "==") return Value::MakeBool(l == r);
    if (node->op == "!=") return Value::MakeBool(l != r);
  }

  // Float operations
  if (left.IsFloat() || right.IsFloat()) {
    float l = left.IsFloat() ? left.AsFloat() : (float)left.AsInt();
    float r = right.IsFloat() ? right.AsFloat() : (float)right.AsInt();
    if (node->op == "+")  return Value::MakeFloat(l + r);
    if (node->op == "-")  return Value::MakeFloat(l - r);
    if (node->op == "*")  return Value::MakeFloat(l * r);
    if (node->op == "/")  return Value::MakeFloat(r != 0.0f ? l / r : 0.0f);
    if (node->op == "<")  return Value::MakeBool(l < r);
    if (node->op == ">")  return Value::MakeBool(l > r);
    if (node->op == "<=") return Value::MakeBool(l <= r);
    if (node->op == ">=") return Value::MakeBool(l >= r);
    if (node->op == "==") return Value::MakeBool(l == r);
    if (node->op == "!=") return Value::MakeBool(l != r);
  }

  // String concatenation
  if (left.IsString() && node->op == "+") {
    String result = left.AsString() + right.AsString();
    return Value::MakeString(result);
  }

  RuntimeError("unsupported binary op: " + node->op);
  return Value::MakeNone();
}

Value Evaluator::EvalUnaryOp(ASTUnaryOpNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value operand = Evaluate(node->operand);

  if (node->op == "-") {
    if (operand.IsInt())  return Value::MakeInt(-operand.AsInt());
    if (operand.IsFloat()) return Value::MakeFloat(-operand.AsFloat());
  }
  if (node->op == "!") {
    if (operand.IsBool()) return Value::MakeBool(!operand.AsBool());
    if (operand.IsInt())  return Value::MakeBool(operand.AsInt() == 0);
  }

  RuntimeError("unsupported unary op: " + node->op);
  return Value::MakeNone();
}

Value Evaluator::EvalMethodCall(ASTMethodCallNodeT* node) {
  if (!node)
    return Value::MakeNone();

  // Evaluate receiver and arguments
  Value receiver = Evaluate(node->object);
  Vector<Value> args;
  args.push(receiver);
  for (size_t i = 0; i < node->args.size(); ++i)
    args.push(Evaluate(node->args[i]));

  return CallEngineFunction(node->methodName, args);
}

Value Evaluator::EvalFuncCall(ASTFuncCallNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Vector<Value> args;
  for (size_t i = 0; i < node->args.size(); ++i)
    args.push(Evaluate(node->args[i]));

  // Check for built-in functions first
  if (node->name == "if" || node->name == "while" || node->name == "for") {
    // These should not appear as function calls in a well-formed AST
    RuntimeError("unexpected keyword as function call: " + node->name);
    return Value::MakeNone();
  }

  return CallEngineFunction(node->name, args);
}

Value Evaluator::EvalCast(ASTCastNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value val = Evaluate(node->operand);

  // Type casting — for now handle int/float and string conversions
  if (node->typeName == "Int") {
    if (val.IsFloat()) return Value::MakeInt((int)val.AsFloat());
    if (val.IsInt())   return val;
    if (val.IsBool())  return Value::MakeInt(val.AsBool() ? 1 : 0);
  }
  if (node->typeName == "Float") {
    if (val.IsInt())   return Value::MakeFloat((float)val.AsInt());
    if (val.IsFloat()) return val;
  }
  if (node->typeName == "Bool") {
    if (val.IsInt())   return Value::MakeBool(val.AsInt() != 0);
    if (val.IsBool())  return val;
  }

  // Engine type casts — pass through to engine conversion system
  return val;
}

Value Evaluator::EvalAddress(ASTAddressNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value val = Evaluate(node->operand);
  // Address-of: return a pointer to the value
  return Value::MakePtr(nullptr, val.data, false);
}

Value Evaluator::EvalDeref(ASTDerefNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value val = Evaluate(node->operand);
  if (val.kind == Value::PTR && val.data)
    return Value::MakeEngine(val.type, val.data);
  return Value::MakeNone();
}

Value Evaluator::EvalArrayLiteral(ASTArrayLiteralNodeT* node) {
  if (!node)
    return Value::MakeNone();

  // For now, return a simple representation
  // Full array support requires engine Array type integration
  return Value::MakeNone();
}

Value Evaluator::EvalConstructor(ASTConstructorNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Vector<Value> args;
  for (size_t i = 0; i < node->args.size(); ++i)
    args.push(Evaluate(node->args[i]));

  // Try engine type constructors
  return CallEngineFunction(node->typeName, args);
}

Value Evaluator::EvalPrint(ASTPrintNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value val = Evaluate(node->operand);
  if (val.IsInt())      std::cout << val.AsInt() << std::endl;
  else if (val.IsFloat()) std::cout << val.AsFloat() << std::endl;
  else if (val.IsBool())  std::cout << (val.AsBool() ? "true" : "false") << std::endl;
  else if (val.IsString()) std::cout << val.AsString() << std::endl;
  else                   std::cout << "<none>" << std::endl;
  return val;
}

// ─── Engine function dispatch ──────────────────────────────────────

Value Evaluator::CallEngineFunction(String const& name, Vector<Value> const& args) {
  Vector<Function> fns = FindFunctions(name);
  if (fns.empty()) {
    RuntimeError("undefined function: " + name);
    return Value::MakeNone();
  }

  // Try each overload
  for (size_t fi = 0; fi < fns.size(); ++fi) {
    Function const& fn = fns[fi];
    if (fn->paramCount != (uint)args.size())
      continue;

    // Build void** args array
    Vector<void*> rawArgs;
    for (size_t i = 0; i < args.size(); ++i) {
      // For primitive types, we need to pass by value through void*
      // This is a simplified version — full implementation needs type-aware boxing
      rawArgs.push(const_cast<void*>(static_cast<void const*>(&args[i])));
    }

    // Allocate return value if needed
    void* retBuf = nullptr;
    if (fn->returnType && fn->returnType->allocate)
      retBuf = fn->returnType->Allocate();

    // Call the engine function
    fn->call(fn->binding, rawArgs.data(), retBuf);

    // Wrap return value
    Value result;
    if (fn->returnType && retBuf) {
      result = Value::MakeEngine(fn->returnType, retBuf);
      fn->returnType->Deallocate(retBuf);
    }
    return result;
  }

  RuntimeError("no matching overload for: " + name);
  return Value::MakeNone();
}

Value Evaluator::CallFunction(ASTFuncDeclNodeT* fn,
                              Vector<Value> const& args,
                              bool hasImplicitThis) {
  if (!fn) {
    RuntimeError("attempt to call a null script function");
    return Value::MakeNone();
  }

  PushScope();

  /* Bind arguments. For type methods the receiver arrives as argument 0 and
     is bound to `this`; the declared parameters follow. */
  size_t argBase = 0;
  if (hasImplicitThis && args.size() > 0) {
    Declare("this", args[0]);
    argBase = 1;
  }
  for (size_t i = 0; i < fn->paramNames.size(); ++i) {
    if (argBase + i < args.size())
      Declare(fn->paramNames[i], args[argBase + i]);
  }

  Value result = Value::MakeNone();
  if (fn->body && fn->body->kind == AST_BLOCK)
    result = EvalBlock(static_cast<ASTBlockNodeT*>(fn->body.t));
  else if (fn->body)
    result = Evaluate(fn->body);

  PopScope();

  if (flowSignal == FLOW_RETURN) {
    result = flowValue;
    flowSignal = FLOW_NONE;
    flowValue = Value::MakeNone();
  }

  return result;
}

Value Evaluator::CallScriptFunction(String const& name, Vector<Value> const& args) {
  // Look up script function
  Value fnVal = Lookup(name);
  if (fnVal.kind != Value::FUNC_REF || !fnVal.data) {
    RuntimeError("undefined function: " + name);
    return Value::MakeNone();
  }

  ASTFuncDeclNodeT* fn = static_cast<ASTFuncDeclNodeT*>(fnVal.data);
  PushScope();

  // Bind parameters
  for (size_t i = 0; i < fn->paramNames.size() && i < args.size(); ++i)
    Declare(fn->paramNames[i], args[i]);

  // Evaluate body
  Value result = EvalBlock(static_cast<ASTBlockNodeT*>(fn->body.t));
  PopScope();

  if (flowSignal == FLOW_RETURN) {
    result = flowValue;
    flowSignal = FLOW_NONE;
    flowValue = Value::MakeNone();
  }

  return result;
}

Vector<Function> Evaluator::FindFunctions(String const& name) {
  return Function_Find(name);
}

// ─── Error reporting ───────────────────────────────────────────────

void Evaluator::RuntimeError(String const& message) {
  errors.push(message);
  std::cerr << "Runtime error: " << message << std::endl;
}

void Evaluator::RuntimeError(String const& message, SourceLocation const& loc) {
  String formatted;
  if (loc.line > 0)
    formatted = Stringize() | "line " | loc.line | ": " | message;
  else
    formatted = message;
  RuntimeError(formatted);
}

void Evaluator::PrintErrors(String const& scriptName) const {
  if (errors.empty()) return;
  std::cout << "'" << scriptName << "' -- " << errors.size()
            << " runtime error(s):" << std::endl;
  for (size_t i = 0; i < errors.size(); ++i)
    std::cout << "  " << errors[i] << std::endl;
}

} // namespace LTE
