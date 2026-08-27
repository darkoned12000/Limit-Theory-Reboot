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
    last = Evaluate(module->statements[i].t);
    if (flowSignal != FLOW_NONE)
      break;
  }
  return last;
}

Value Evaluator::Evaluate(ASTNode* node) {
  if (!node)
    return Value::MakeNone();

  switch (node->kind) {
    case AST_MODULE:       return EvalBlock(static_cast<ASTBlockNodeT*>(node));
    case AST_BLOCK:        return EvalBlock(static_cast<ASTBlockNodeT*>(node));
    case AST_FUNC_DECL:    return EvalFuncDecl(static_cast<ASTFuncDeclNodeT*>(node));
    case AST_TYPE_DECL:    return EvalTypeDecl(static_cast<ASTTypeDeclNodeT*>(node));
    case AST_RETURN:       return EvalReturn(static_cast<ASTReturnNodeT*>(node));
    case AST_IF:           return EvalIf(static_cast<ASTIfNodeT*>(node));
    case AST_WHILE:        return EvalWhile(static_cast<ASTWhileNodeT*>(node));
    case AST_FOR:          return EvalFor(static_cast<ASTForNodeT*>(node));
    case AST_SWITCH:       return EvalSwitch(static_cast<ASTSwitchNodeT*>(node));
    case AST_VAR_DECL:
    case AST_REF_DECL:
    case AST_STATIC_DECL:  return EvalVarDecl(static_cast<ASTDeclNodeT*>(node));
    case AST_ASSIGN:       return EvalAssign(static_cast<ASTAssignNodeT*>(node));
    case AST_EXPR_STMT:    return EvalExprStmt(static_cast<ASTExprStmtNodeT*>(node));
    case AST_INT:          return EvalInt(static_cast<ASTIntNodeT*>(node));
    case AST_FLOAT:        return EvalFloat(static_cast<ASTFloatNodeT*>(node));
    case AST_STRING:       return EvalString(static_cast<ASTStringNodeT*>(node));
    case AST_BOOL:         return EvalBool(static_cast<ASTBoolNodeT*>(node));
    case AST_NULL:         return EvalNull(static_cast<ASTNullNodeT*>(node));
    case AST_IDENTIFIER:   return EvalIdentifier(static_cast<ASTIdentifierNodeT*>(node));
    case AST_BINARY_OP:    return EvalBinaryOp(static_cast<ASTBinaryOpNodeT*>(node));
    case AST_UNARY_OP:     return EvalUnaryOp(static_cast<ASTUnaryOpNodeT*>(node));
    case AST_METHOD_CALL:  return EvalMethodCall(static_cast<ASTMethodCallNodeT*>(node));
    case AST_FUNC_CALL:    return EvalFuncCall(static_cast<ASTFuncCallNodeT*>(node));
    case AST_CAST:         return EvalCast(static_cast<ASTCastNodeT*>(node));
    case AST_ADDRESS:      return EvalAddress(static_cast<ASTAddressNodeT*>(node));
    case AST_DEREF:        return EvalDeref(static_cast<ASTDerefNodeT*>(node));
    case AST_ARRAY_LITERAL:return EvalArrayLiteral(static_cast<ASTArrayLiteralNodeT*>(node));
    case AST_CONSTRUCTOR:  return EvalConstructor(static_cast<ASTConstructorNodeT*>(node));
    case AST_PRINT:        return EvalPrint(static_cast<ASTPrintNodeT*>(node));
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
    last = Evaluate(block->statements[i].t);
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
    init = Evaluate(decl->initializer.t);

  Declare(decl->name, init, decl->kind == AST_REF_DECL);
  return init;
}

Value Evaluator::EvalFuncDecl(ASTFuncDeclNodeT* decl) {
  if (!decl)
    return Value::MakeNone();

  // Store function reference in scope
  Value fnVal;
  fnVal.kind = FUNC_REF;
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
  tv.kind = TYPE_REF;
  tv.data = decl;
  tv.owned = false;
  Declare(decl->name, tv);
  return Value::MakeNone();
}

Value Evaluator::EvalIf(ASTIfNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value cond = Evaluate(node->condition.t);
  if (cond.IsBool() && cond.AsBool()) {
    return Evaluate(node->thenBlock.t);
  } else if (node->elseBlock.t) {
    return Evaluate(node->elseBlock.t);
  }
  return Value::MakeNone();
}

Value Evaluator::EvalWhile(ASTWhileNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value last = Value::MakeNone();
  while (true) {
    Value cond = Evaluate(node->condition.t);
    if (!cond.IsBool() || !cond.AsBool())
      break;

    last = Evaluate(node->body.t);
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
  Value init = Evaluate(node->init.t);
  Value last = Value::MakeNone();

  while (true) {
    Value cond = Evaluate(node->condition.t);
    if (!cond.IsBool() || !cond.AsBool())
      break;

    last = Evaluate(node->body.t);
    if (flowSignal == FLOW_BREAK) {
      flowSignal = FLOW_NONE;
      break;
    }
    if (flowSignal == FLOW_RETURN)
      break;

    Evaluate(node->update.t);
  }
  PopScope();
  return last;
}

Value Evaluator::EvalSwitch(ASTSwitchNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value expr = Evaluate(node->expression.t);
  for (size_t i = 0; i < node->cases.size(); ++i) {
    Value caseVal = Evaluate(node->cases[i].condition.t);
    // Simple equality check (works for int/float/string)
    bool match = false;
    if (expr.IsInt() && caseVal.IsInt())
      match = expr.AsInt() == caseVal.AsInt();
    else if (expr.IsFloat() && caseVal.IsFloat())
      match = expr.AsFloat() == caseVal.AsFloat();
    else if (expr.IsBool() && caseVal.IsBool())
      match = expr.AsBool() == caseVal.AsBool();

    if (match)
      return Evaluate(node->cases[i].body.t);
  }
  if (node->otherwise.t)
    return Evaluate(node->otherwise.t);
  return Value::MakeNone();
}

Value Evaluator::EvalAssign(ASTAssignNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value rhs = Evaluate(node->rhs.t);
  Value* target = &Lookup(node->name);

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
    flowValue = Evaluate(node->value.t);
  else
    flowValue = Value::MakeNone();
  return flowValue;
}

Value Evaluator::EvalExprStmt(ASTExprStmtNodeT* node) {
  if (!node)
    return Value::MakeNone();
  return Evaluate(node->expression.t);
}

// ─── Expression evaluation ─────────────────────────────────────────

Value Evaluator::EvalInt(ASTIntNodeT* node) {
  return node ? Value::MakeInt(node->value) : Value::MakeNone();
}

Value Evaluator::EvalFloat(ASTFloatNodeT* node) {
  return node ? Value::MakeFloat(node->value) : Value::MakeNone();
}

Value Evaluator::EvalString(ASTStringNodeT* node) {
  return node ? Value::MakeString(node->value) : Value::MakeNone();
}

Value Evaluator::EvalBool(ASTBoolNodeT* node) {
  return node ? Value::MakeBool(node->value) : Value::MakeNone();
}

Value Evaluator::EvalNull(ASTNullNodeT*) {
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
    Value left = Evaluate(node->left.t);
    if (left.IsBool() && !left.AsBool())
      return Value::MakeBool(false);
    Value right = Evaluate(node->right.t);
    return Value::MakeBool(right.IsBool() && right.AsBool());
  }
  if (node->op == "||") {
    Value left = Evaluate(node->left.t);
    if (left.IsBool() && left.AsBool())
      return Value::MakeBool(true);
    Value right = Evaluate(node->right.t);
    return Value::MakeBool(right.IsBool() && right.AsBool());
  }

  Value left = Evaluate(node->left.t);
  Value right = Evaluate(node->right.t);

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

  Value operand = Evaluate(node->operand.t);

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
  Value receiver = Evaluate(node->receiver.t);
  Vector<Value> args;
  args.push(receiver);
  for (size_t i = 0; i < node->arguments.size(); ++i)
    args.push(Evaluate(node->arguments[i].t));

  return CallEngineFunction(node->methodName, args);
}

Value Evaluator::EvalFuncCall(ASTFuncCallNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Vector<Value> args;
  for (size_t i = 0; i < node->arguments.size(); ++i)
    args.push(Evaluate(node->arguments[i].t));

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

  Value val = Evaluate(node->expression.t);

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

  Value val = Evaluate(node->expression.t);
  // Address-of: return a pointer to the value
  return Value::MakePtr(nullptr, val.data, false);
}

Value Evaluator::EvalDeref(ASTDerefNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value val = Evaluate(node->expression.t);
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
  for (size_t i = 0; i < node->arguments.size(); ++i)
    args.push(Evaluate(node->arguments[i].t));

  // Try engine type constructors
  return CallEngineFunction(node->typeName, args);
}

Value Evaluator::EvalPrint(ASTPrintNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value val = Evaluate(node->expression.t);
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
  for (size_t i = 0; i < fn->params.size() && i < args.size(); ++i)
    Declare(fn->params[i].name, args[i]);

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
