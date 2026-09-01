// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Evaluator.h"
#include "AST.h"
#include "Data.h"
#include "Function.h"
#include "Map.h"
#include "Parameter.h"
#include "Script.h"
#include "Stack.h"
#include "Type.h"
#include "V2.h"
#include "V3.h"
#include "V4.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace LTE {

// ─── Value implementation ──────────────────────────────────────────

Value Value::MakeEngine(Type t, void const* src) {
  Value r;
  r.kind = CUSTOM;
  r.type = t;
  r.owned = true;
  r.data = t->Allocate();
  if (t->construct) t->construct(t.t, r.data);
  if (src)
    t->Assign(const_cast<void*>(src), r.data);
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
    if (other.type->construct) other.type->construct(other.type.t, data);
    other.type->Assign(other.data, data);
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
      if (other.type->construct) other.type->construct(other.type.t, data);
      other.type->Assign(other.data, data);
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
      // Reference<T> now has a ref-counting-aware assign/destruct via the
      // type system, so the standard deallocate path is correct.
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

void Evaluator::SetScript(ScriptT* ctx) {
  script = ctx;
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
    case AST_SWITCH_EXPR:  return EvalSwitchExpr(static_cast<ASTSwitchExprNodeT*>(node.t));
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

/* `++`/`--` on a named variable: the engine binding mutates storage by
   reference, which a by-value call would discard, so mutate the scope slot
   in place. Returns the new value, or NONE if the name isn't a numeric local. */
Value Evaluator::IncDecSlot(String const& op, String const& name) {
  Value& slot = Lookup(name);
  if (slot.IsInt()) {
    int v = slot.AsInt() + ((op == "++") ? 1 : -1);
    slot = Value::MakeInt(v);
    return slot;
  }
  if (slot.IsFloat()) {
    float v = slot.AsFloat() + ((op == "++") ? 1.0f : -1.0f);
    slot = Value::MakeFloat(v);
    return slot;
  }
  return Value::MakeNone();
}

/* `++`/`--` on an arbitrary operand expression (a variable, or a field of
   `this` / of a receiver — e.g. `this.count.++`): mutate in place and return
   the new value. */
Value Evaluator::IncDecOperand(String const& op, ASTNode operand) {
  if (!operand)
    return Value::MakeNone();
  if (operand->kind == AST_IDENTIFIER) {
    Value r = IncDecSlot(op, static_cast<ASTIdentifierNodeT*>(operand.t)->name);
    if (r.kind != Value::NONE)
      return r;
    /* Bare member name resolving to a field of `this`. */
    Value f = ThisFieldGet(static_cast<ASTIdentifierNodeT*>(operand.t)->name);
    if (f.IsInt()) return Value::MakeInt(f.AsInt() + ((op == "++") ? 1 : -1));
    if (f.IsFloat()) return Value::MakeFloat(f.AsFloat() + ((op == "++") ? 1.0f : -1.0f));
    return Value::MakeNone();
  }
  if (operand->kind == AST_METHOD_CALL) {
    ASTMethodCallNodeT* m = static_cast<ASTMethodCallNodeT*>(operand.t);
    Value recv = Evaluate(m->object);
    Value f = FieldGet(recv, m->methodName);
    if (f.IsInt()) { Value n = Value::MakeInt(f.AsInt() + ((op == "++") ? 1 : -1)); FieldSet(recv, m->methodName, n); return n; }
    if (f.IsFloat()) { Value n = Value::MakeFloat(f.AsFloat() + ((op == "++") ? 1.0f : -1.0f)); FieldSet(recv, m->methodName, n); return n; }
  }
  return Value::MakeNone();
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
  bool initEvaluated = false;
  if (decl->initializer.t) {
    /* `var x TypeName`: a bare-identifier initializer that names a script type
       is a TYPED declaration (a default-constructed instance), matching the
       legacy `var self App` / `var job MyJob` semantics — NOT an expression
       that evaluates the type name. */
    if (decl->initializer->kind == AST_IDENTIFIER && script.t) {
      String iname = static_cast<ASTIdentifierNodeT*>(decl->initializer.t)->name;
      ScriptType st = script->GetType(iname);
      if (st) {
        init = MakeScriptTypeValue(st);
        initEvaluated = true;
      }
    }
    if (!initEvaluated)
      init = Evaluate(decl->initializer);
  } else if (!decl->typeName.empty()) {
    Type t = Type_Find(decl->typeName);
    std::string tn = decl->typeName.c_str();
    /* Script-defined types live in the script's type table, not the global
       engine registry (`var self App`, `var job MyJob`). */
    if (!t && script.t) {
      ScriptType st = script->GetType(decl->typeName);
      if (st && st->type)
        t = st->type;
    }
    if (!t && tn.find("Vector") != std::string::npos) {
      t = Type_Find("Vector");
      if (!t) t = Type_Find("Array");
      // Try generic Vector<...> lookup via string with alias
      if (!t) {
        // Fallback: look for any Vector type via list
        for (auto &tt : Type_GetList()) {
          if (std::string(tt->name.c_str()).find("Vector") != std::string::npos) { t = tt; break; }
        }
      }
    }
    if (t) {
      void* raw = t->Allocate();
      if (t->construct) t->construct(t.t, raw);
      init = Value::MakePtr(t, raw, true);
    } else {
      Type arrBase = Type_Find("Vector");
      if (!arrBase) arrBase = Type_Find("Array");
      if (arrBase && arrBase->allocate) {
        void* raw = arrBase->Allocate();
        if (arrBase->construct) arrBase->construct(arrBase.t, raw);
        init = Value::MakePtr(arrBase, raw, true);
      }
    }
  }

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

  /* The loop variable shares its scope with the header agents: `for i init
     cond step` declares `i` from the initializer into the loop scope. */
  if (node->init.t) {
    Value init = Evaluate(node->init);
    Declare(node->iteratorName, init);
  }

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

  // `switch` with no expression: each case is a boolean predicate + body
  // (`self.focusMouse Colors:Secondary`). First truthy predicate wins.
  if (!node->expression) {
    for (size_t i = 0; i < node->cases.size(); ++i) {
      Value caseVal = Evaluate(node->cases[i].condition);
      if (caseVal.IsBool() && caseVal.AsBool())
        return Evaluate(node->cases[i].body);
    }
    if (node->otherwise.t)
      return Evaluate(node->otherwise);
    return Value::MakeNone();
  }

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

// `? (pred body) (pred body) ... (otherwise default)` — boolean predicate
// chain, first truthy predicate wins; falls back to the default (or None).
Value Evaluator::EvalSwitchExpr(ASTSwitchExprNodeT* node) {
  if (!node)
    return Value::MakeNone();

  for (size_t i = 0; i + 1 < node->cases.size(); i += 2) {
    Value caseVal = Evaluate(node->cases[i]);
    if (caseVal.IsBool() && caseVal.AsBool())
      return Evaluate(node->cases[i + 1]);
  }
  // Odd trailing case is a pred-only case: value is the predicate itself.
  if (node->cases.size() % 2 == 1) {
    Value caseVal = Evaluate(node->cases[node->cases.size() - 1]);
    if (caseVal.IsBool() && caseVal.AsBool())
      return caseVal;
  }
  if (node->defaultExpr)
    return Evaluate(node->defaultExpr);
  return Value::MakeNone();
}

Value Evaluator::EvalAssign(ASTAssignNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value rhs = Evaluate(node->value);

  /* Dotted lvalues: `a.b = x` / `a.b += y`. */
  if (node->target && node->target->kind == AST_METHOD_CALL) {
    ASTMethodCallNodeT* m = static_cast<ASTMethodCallNodeT*>(node->target.t);
    Value recv = Evaluate(m->object);
    Value written = (node->op == "=") ? rhs
      : ApplyBinaryOp(node->op, FieldGet(recv, m->methodName), rhs);
    if (!FieldSet(recv, m->methodName, written)) {
      RuntimeError("cannot assign to member '" + m->methodName + "'", node->loc);
      return Value::MakeNone();
    }
    return written;
  }

  String targetName;
  if (node->target && node->target->kind == AST_IDENTIFIER) {
    targetName = static_cast<ASTIdentifierNodeT*>(node->target.t)->name;
  } else {
    RuntimeError("unsupported assignment target", node->loc);
    return Value::MakeNone();
  }

  /* Uniform assignment through `this` fields: bare member names inside a
     method body write the receiver's field. */
  if (!ScopeHasName(targetName)) {
    if (node->op == "=") {
      if (ThisFieldSet(targetName, rhs))
        return rhs;
    } else {
      Value cur = ThisFieldGet(targetName);
      if (cur.kind != Value::NONE) {
        Value written = ApplyBinaryOp(node->op, cur, rhs);
        ThisFieldSet(targetName, written);
        return written;
      }
    }
    /* Fall back to a dynamic local (unresolved at compile time). */
    Declare(targetName, rhs);
    return rhs;
  }

  Value* target = &Lookup(targetName);

  if (node->op == "=") {
    *target = rhs;
  } else {
    bool prim = target->IsInt() || target->IsFloat() || target->IsString();
    if (prim) {
      if (node->op == "+=") {
        if (target->IsInt() && rhs.IsInt())
          *target = Value::MakeInt(target->AsInt() + rhs.AsInt());
        else if (target->IsFloat() && rhs.IsFloat())
          *target = Value::MakeFloat(target->AsFloat() + rhs.AsFloat());
        else if (target->IsString() && rhs.IsString())
          *target = Value::MakeString(target->AsString() + rhs.AsString());
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
    } else {
      /* Engine/collection operands (`boxes += box` -> List_Append). Dispatch
         to the operator overload registered for the compound op. Append-like
         ops return void (a NONE result), in which case keep the target
         unchanged; mutating/arithmetic ops that return a value are assigned
         back. */
      Value r = CallEngineFunction(node->op, {*target, rhs}, node->loc);
      if (r.kind != Value::NONE)
        *target = r;
    }
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

namespace {
  /* Hard "Data" type used to box script-type instances as Values. The legacy
     interpreter exposed script values as a Data (`.type` = the script's
     runtime type, `.data` = the instance buffer), which is what bindings like
     `Widget_Custom(Widget const&, Data const&)` and `Thread_Create` consume.
     Registered lazily so `Type_Find("Data")` stays usable for the whole
     evaluator. */
  Type GetDataValueType() {
    Type t = Type_Find("Data");
    if (t)
      return t;
    t = Type_Create("Data", sizeof(Data));
    t->alignment = alignof(Data);
    t->allocate = __type_default_allocator<Data>;
    t->assign = __type_default_assign<Data>;
    t->construct = __type_default_construct<Data>;
    t->deallocate = __type_default_deallocator<Data>;
    t->destruct = __type_default_destruct<Data>;
    return t;
  }

  bool IsDataWrapped(Value const& v) {
    return v.data && v.type && v.type == GetDataValueType();
  }

  /* True if `t` is the reflected hard type of a script type (aux holds the
     ScriptType metadata). */
  bool IsScriptType(Type t) {
    if (!t)
      return false;
    Data& aux = t->GetAux();
    return aux && aux.IsType<ScriptType>();
  }

  /* The instance buffer of a script value — unwraps the Data box. `recv.data`
     is the Data struct; the payload lives at `data`. */
  void* ScriptInstance(Value const& recv) {
    if (IsDataWrapped(recv))
      return static_cast<Data*>(recv.data)->data;
    return recv.data;
  }
}

Value Evaluator::MakeScriptTypeValue(ScriptType const& st) {
  if (!st || !st->type || !st->type->allocate)
    return Value::MakeNone();
  Data* d = new Data;
  d->type = st->type;
  d->data = st->type->Allocate();
  return Value::MakePtr(GetDataValueType(), d, true);
}

Value Evaluator::EvalIdentifier(ASTIdentifierNodeT* node) {
  if (!node)
    return Value::MakeNone();
  Value r = Lookup(node->name);

  /* Bare member names inside a method body resolve to fields of `this`. */
  if (r.kind == Value::NONE && node->name != "this") {
    Value f = ThisFieldGet(node->name);
    if (f.kind != Value::NONE)
      return f;
  }

  /* Legacy name resolution for bare identifiers in expression/argument
     position (old LTSL compiled these away; the runtime evaluator must too):
       - a registered script/engine function name is a ZERO-ARG CALL
         (e.g. `Widget` -> Widget_Create(), bare `Camera_Create`, ...);
       - a script type name constructs a default instance boxed as a Data
         (e.g. `Custom Widget ThreadsScreen`). */
  if (r.kind == Value::NONE) {
    if (script.t && script->functions.get(node->name)) {
      Value f = CallScriptFunction(node->name, Vector<Value>(), node->loc);
      if (f.kind != Value::NONE)
        return f;
    }
    if (Function_Exists(node->name)) {
      Value c = CallEngineFunction(node->name, Vector<Value>(), node->loc);
      if (c.kind != Value::NONE)
        return c;
    }
    if (script.t) {
      ScriptType st = script->GetType(node->name);
      if (st)
        return MakeScriptTypeValue(st);
    }
  }
  return r;
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

  /* Engine-managed operand types (Vec2/Vec3/Vec4/Position/Object/Array/...):
     dispatch to the operator overloads registered for the operator name.
     Vector arithmetic (V3F+V3F, V3F*scalar, etc.) is bound to "+", "-", "*",
     "/", "<", ">", "<=", ">=", "==", "!=" as script functions, so route them
     through the normal engine-overload resolution. */
  bool leftPrim = left.IsInt() || left.IsFloat() || left.IsBool() || left.IsString();
  bool rightPrim = right.IsInt() || right.IsFloat() || right.IsBool() || right.IsString();
  if (!leftPrim || !rightPrim)
    return CallEngineFunction(node->op, {left, right}, node->loc);

  RuntimeError("unsupported binary op: " + node->op, node->loc);
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

  RuntimeError("unsupported unary op: " + node->op, node->loc);
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

  /* Postfix increment/decrement on a variable: `i.++` / `i.--`. The engine
     binding (`Int_Increment`, alias "++") takes the int BY REFERENCE and
     mutates the caller's storage — but the receiver here is a by-value copy of
     the variable, so calling it would discard the change. Instead, mutate the
     actual scope slot directly and return the new value. */
  if ((node->methodName == "++" || node->methodName == "--") && node->args.empty())
    if (node->object && node->object->kind == AST_IDENTIFIER)
      if (Value mutated = IncDecSlot(node->methodName,
            static_cast<ASTIdentifierNodeT*>(node->object.t)->name); mutated.kind != Value::NONE)
        return mutated;

  /* Property access on a script-type value (`msg.slotName`). */
  ScriptType rt = ScriptTypeOf(receiver);
  if (rt && FindField(rt, node->methodName))
    return FieldGet(receiver, node->methodName);

  /* Receiver's script-type method first — `this.DoDelete` must find the
     method on the receiver's type, not the script-global which may have been
     overwritten by a later type with the same method name (e.g. SwitchProbe
     vs TestMethodCall both have DoDelete). */
  if (rt) {
    ScriptFunction sf = rt->GetFunction(node->methodName);
    if (sf)
      return CallScriptFunction(sf, args);
  }

  /* Script method / hoisted function fallback. */
  if (script.t) {
    ScriptFunction sf = script->GetFunction(node->methodName);
    if (sf)
      return CallScriptFunction(sf, args);
  }

  return CallEngineFunction(node->methodName, args, node->loc);
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
    RuntimeError("unexpected keyword as function call: " + node->name, node->loc);
    return Value::MakeNone();
  }

  /* Prefix increment/decrement: `(++ i)` / `-- i` parse as a call to the
     `++`/`--` alias with the operand as its single argument. The operand must
     be a mutable variable/field (the binding mutates by reference), so mutate
     it in place and return the new value. */
  if ((node->name == "++" || node->name == "--") && node->args.size() == 1) {
    Value r = IncDecOperand(node->name, node->args[0]);
    if (r.kind != Value::NONE)
      return r;
  }

  /* `(TypeName expr)` casts: lowercase scalar type heads parse as function
     calls, but LTSL semantics are a conversion (`(String x)`, `(Int x)`). */
  if (node->args.size() == 1) {
    bool isScalarCast = node->name == "String" || node->name == "Int" ||
                        node->name == "Float" || node->name == "Bool";
    if (isScalarCast) {
      Value v = ScalarCast(node->name, args[0]);
      if (v.kind != Value::NONE)
        return v;
    }
  }

  /* Script function first (includes methods/hoisted nested functions).
     Then engine functions. */
  return CallScriptFunction(node->name, args, node->loc);
}

Value Evaluator::EvalCast(ASTCastNodeT* node) {
  if (!node)
    return Value::MakeNone();

  Value val = Evaluate(node->operand);

  /* Script-type cast: view the operand buffer as the script type's layout.
     `(cast ProbeMsgSelect data)` reinterprets the Data payload. */
  if (script.t) {
    ScriptType st = script->GetType(node->typeName);
    if (st) {
      void* inst = val.data;
      if (val.type == Type_Find("Data") && val.data) {
        Data* d = static_cast<Data*>(val.data);
        if (d->type && d->data)
          inst = d->data;
      } else if (val.type == st->type) {
        inst = val.data;
      }
      if (inst) {
        /* Box the cast result like a constructor-built instance: Value.type is
           the generic Data wrapper and v.data points at a Data{st->type, data}
           so engine functions expecting `Data` (e.g. List_Append) read a
           consistent element type. Deep-own a copy of the instance so the
           borrowed/owned lifetimes match the constructor path. */
        Data* d = new Data;
        d->type = st->type;
        d->data = st->type->Allocate();
        if (st->type->assign)
          st->type->Assign(inst, d->data);
        return Value::MakePtr(GetDataValueType(), d, true);
      }
      RuntimeError("cast to '" + node->typeName + "' of an empty value");
      return Value::MakeNone();
    }
  }

  // Type casting — for now handle int/float/bool/string conversions
  return ScalarCast(node->typeName, val);
}

Value Evaluator::ScalarCast(String const& typeName, Value const& val) {
  if (typeName == "Int") {
    if (val.IsFloat()) return Value::MakeInt((int)val.AsFloat());
    if (val.IsInt())   return val;
    if (val.IsBool())  return Value::MakeInt(val.AsBool() ? 1 : 0);
    if (val.IsString()) {
      char const* s = val.AsString();
      return Value::MakeInt((int)strtol(s, nullptr, 10));
    }
  }
  if (typeName == "Float") {
    if (val.IsInt())   return Value::MakeFloat((float)val.AsInt());
    if (val.IsFloat()) return val;
    if (val.IsString()) {
      char const* s = val.AsString();
      return Value::MakeFloat(strtof(s, nullptr));
    }
  }
  if (typeName == "Bool") {
    if (val.IsInt())   return Value::MakeBool(val.AsInt() != 0);
    if (val.IsBool())  return val;
  }
  if (typeName == "String") {
    if (val.IsString()) return val;
    if (val.IsInt()) {
      return Value::MakeString(Stringize() | val.AsInt());
    }
    if (val.IsFloat()) {
      return Value::MakeString(Stringize() | val.AsFloat());
    }
    if (val.IsBool()) {
      return Value::MakeString(val.AsBool() ? "true" : "false");
    }
  }
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

  /* Script-type constructor: `(Box center size)` builds a default instance
     and stores the args into the type's fields in declaration order. */
  if (script.t) {
    ScriptType st = script->GetType(node->typeName);
    if (st && st->fields.size() >= args.size()) {
      Value inst = MakeScriptTypeValue(st);
      if (!inst.IsNone()) {
        for (size_t i = 0; i < args.size(); ++i)
          FieldSet(inst, st->fields[i].name, args[i]);
        return inst;
      }
    }
  }

  // Try engine type constructors
  return CallEngineFunction(node->typeName, args, node->loc);
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

/* Does argument `v` satisfy parameter type `pt`? Primitives match by Value
   kind; engine types by type identity, a registered conversion, or the same
   layout under an alias/Reference name; unknown param types (null) are
   treated as matching so manually-configured functions still dispatch. */
static bool EvalParamMatch(Type pt, Value const& v) {
  if (!pt)
    return true;
  if (pt == Type_Get<int>())
    return v.IsInt() || v.IsFloat();
  if (pt == Type_Get<float>() || pt == Type_Get<double>())
    return v.IsFloat() || v.IsInt();
  if (pt == Type_Get<bool>())
    return v.IsBool();
  if (pt == Type_Get<String>())
    return v.IsString();
  // Also handle alias lookup (LTSL uses "Int"/"Float"/"String")
  if (pt == Type_Find("Int"))
    return v.IsInt() || v.IsFloat();
  if (pt == Type_Find("Float"))
    return v.IsFloat() || v.IsInt();
  if (pt == Type_Find("String"))
    return v.IsString();

  /* Integer-compatible primitives that lack a registered Type identity or are
     registered under a different name than Type_Get<int>(). */
  if (pt->name == "uint" || pt->name == "uint32" || pt->name == "uint64" ||
      pt->name == "long" || pt->name == "unsigned int" ||
      pt->name == "size_t" || pt->name == "int32" || pt->name == "int64")
    return v.IsInt() || v.IsFloat();
  if (pt->name == "double" || pt->name == "Float64")
    return v.IsFloat() || v.IsInt();

  if (v.IsInt())
    return pt->size == sizeof(int) && pt->alignment == alignof(int);
  if (v.IsFloat())
    return pt->size == sizeof(float) && pt->alignment == alignof(float);
  if (v.IsBool())
    return pt->size == sizeof(bool) && pt->alignment == alignof(bool);
  if (v.IsString())
    return v.type == pt;

  if (!v.type)
    return true;

  if (v.type == pt)
    return true;

  /* A registered conversion between the value's type and the parameter
     (`DefineConversion`-style, e.g. V3F -> Color). */
  if (v.type->GetConversions().size() > 0) {
    for (size_t i = 0; i < v.type->GetConversions().size(); ++i)
      if (v.type->GetConversions()[i].other == pt)
        return true;
  }

  /* Reference<T> params reflect as `Reference<T>` while values are tagged
     `T`; and alias types (Position = Float3) register as distinct objects
     from the concrete value type. Compare by name/layout when the storage
     matches. */
  if (v.type->size == pt->size) {
    String ptName = pt->name;
    if (ptName == "Reference<" + v.type->name + ">")
      return true;
    if (v.type->name == "Reference<" + ptName + ">")
      return true;
    if (v.type->name == ptName)
      return true;
  }
  return false;
}

Value Evaluator::CallEngineFunction(String const& name, Vector<Value> const& args, SourceLocation const& loc) {
  Vector<Function> fns = FindFunctions(name);
  if (fns.empty()) {
    if (loc.line > 0) RuntimeError("undefined function: " + name, loc);
    else RuntimeError("undefined function: " + name);
    return Value::MakeNone();
  }

  /* Select the best overload: prefer a FULL param-type match (resolves
     aliases like `Length` -> String_Length vs Vec3f_Length), otherwise the
     overload with the most matching param types. Guards for overloads whose
     `params` are missing/null. */
  int bestIndex = -1;
  size_t bestScore = 0;
  for (size_t fi = 0; fi < fns.size(); ++fi) {
    Function const& fn = fns[fi];
    if (fn->paramCount != (uint)args.size())
      continue;

    size_t score = 0;
    bool full = true;
    for (size_t i = 0; i < args.size(); ++i) {
      Type pt = (fn->params) ? fn->params[i].type : nullptr;
      if (EvalParamMatch(pt, args[i]))
        ++score;
      else
        full = false;
    }
    if (full) {
      bestIndex = (int)fi;
      break;
    }
    if (score > bestScore) {
      bestIndex = (int)fi;
      bestScore = score;
    }
  }

  if (bestIndex < 0) {
    // Fast path for single-arg Vec constructions like `Vec4 0.0` / `Vec3 15.012`
    // which are registered as conversions (float->Vec) not as 4-arg functions.
    if (args.size() == 1 && (args[0].IsFloat() || args[0].IsInt())) {
      float f = args[0].IsFloat() ? args[0].AsFloat() : (float)args[0].AsInt();
      bool isVec4 = (name == String("Vec4") || name == String("V4") || name == String("Vec4f") || name == String("V4T") || name == String("Vec4d"));
      bool isVec3 = (name == String("Vec3") || name == String("V3") || name == String("Vec3f") || name == String("V3T") || name == String("Vec3d") || name == String("Vec3D"));
      bool isVec2 = (name == String("Vec2") || name == String("V2"));
      if (isVec4) {
        Type t = Type_Find(name);
        if (!t) t = Type_Find("Vec4");
        if (!t) t = Type_Find("V4");
        if (!t) t = Type_Find("V4F");
        if (t) {
          void* mem = t->Allocate();
          *static_cast<V4T<float>*>(mem) = V4T<float>(f);
          return Value::MakePtr(t, mem, true);
        }
      }
      if (isVec3) {
        Type t = Type_Find(name);
        if (!t) t = Type_Find("Vec3");
        if (!t) t = Type_Find("V3");
        if (!t) t = Type_Find("V3F");
        if (t) {
          void* mem = t->Allocate();
          // Use size to distinguish float vs double
          if (t->size == sizeof(V3T<double>)) {
            *static_cast<V3T<double>*>(mem) = V3T<double>((double)f);
          } else {
            *static_cast<V3T<float>*>(mem) = V3T<float>(f);
          }
          return Value::MakePtr(t, mem, true);
        }
      }
      if (isVec2) {
        Type t = Type_Find(name);
        if (!t) t = Type_Find("Vec2");
        if (t) {
          void* mem = t->Allocate();
          *static_cast<V2T<float>*>(mem) = V2T<float>(f);
          return Value::MakePtr(t, mem, true);
        }
      }
    }
    // Fallback: try type conversion, e.g. `Vec4 0.0` where 0.0 (float) -> Vec4
    // via Conversion_Bind. This handles single-arg constructions that are
    // registered as conversions, not as Function_Bind.
    if (args.size() == 1) {
      Type target = Type_Find(name);
      if (target) {
        Type srcType = nullptr;
        if (args[0].IsInt()) srcType = Type_Get<int>();
        else if (args[0].IsFloat()) srcType = Type_Get<float>();
        else if (args[0].IsBool()) srcType = Type_Get<bool>();
        else if (args[0].IsString()) srcType = Type_Get<String>();
        else srcType = args[0].type;
        if (srcType) {
          for (auto &conv : target->GetConversions()) {
            if (conv.other == srcType) {
              void* dest = target->Allocate();
              // Need raw pointer to source value
              void* srcPtr = ValueArgPtr(args[0]);
              conv.fn(target.t, srcPtr, dest);
              // Wrap as owned PTR
              Value r = Value::MakePtr(target, dest, true);
              // Convert primitive returns if needed
              return r;
            }
          }
          // Also try cross float/int for Vec types (int -> float conversion)
          if (srcType == Type_Get<int>()) {
            Type floatType = Type_Get<float>();
            for (auto &conv : target->GetConversions()) {
              if (conv.other == floatType) {
                float tmp = (float)args[0].AsInt();
                void* dest = target->Allocate();
                conv.fn(target.t, &tmp, dest);
                return Value::MakePtr(target, dest, true);
              }
            }
          }
          if (srcType == Type_Get<float>()) {
            Type intType = Type_Get<int>();
            for (auto &conv : target->GetConversions()) {
              if (conv.other == intType) {
                int tmp = (int)args[0].AsFloat();
                void* dest = target->Allocate();
                conv.fn(target.t, &tmp, dest);
                return Value::MakePtr(target, dest, true);
              }
            }
          }
        }
      }
    }
    String msg = String("no matching overload for: ") + name;
    if (loc.line > 0) RuntimeError(msg, loc);
    else RuntimeError(msg);
    return Value::MakeNone();
  }

  Function const& fn = fns[bestIndex];

  // Build the raw argument array. Engine bindings expect each void* to
  // point at a real parameter object (value or ref type), so we hand out
  // the Value's own storage: the inline union for primitives, the heap
  // String*, or the borrowed pointer for engine/heap types.
  Vector<void*> rawArgs;
  rawArgs.reserve(args.size());
  for (size_t i = 0; i < args.size(); ++i)
    rawArgs.push(ValueArgPtr(args[i]));

  // Allocate return value if needed
  void* retBuf = nullptr;
  if (fn->returnType && fn->returnType->allocate) {
    retBuf = fn->returnType->Allocate();
    if (fn->returnType->construct) fn->returnType->construct(fn->returnType.t, retBuf);
  }

  // Call the engine function
  fn->call(fn->binding, rawArgs.data(), retBuf);

  // Wrap return value — map primitive returns to INT/FLOAT/BOOL/STRING kinds
  Value result;
  if (fn->returnType && retBuf) {
    if (fn->returnType == Type_Get<int>() || fn->returnType == Type_Find("Int"))
      result = Value::MakeInt(*static_cast<int*>(retBuf));
    else if (fn->returnType == Type_Get<float>() || fn->returnType == Type_Get<double>() || fn->returnType == Type_Find("Float"))
      result = Value::MakeFloat(*static_cast<float*>(retBuf));
    else if (fn->returnType == Type_Get<bool>() || fn->returnType == Type_Find("Bool"))
      result = Value::MakeBool(*static_cast<bool*>(retBuf));
    else if (fn->returnType == Type_Get<String>() || fn->returnType == Type_Find("String"))
      result = Value::MakeString(*static_cast<String*>(retBuf));
    else {
      result = Value::MakeEngine(fn->returnType, retBuf);
    }
    fn->returnType->Deallocate(retBuf);
  }
  return result;
}

Value Evaluator::ValueFromSlot(Type t, void* data) {
  if (!data || !t)
    return Value::MakeNone();

  /* Primitives are stored inline in the Value (no heap, no ownership). */
  if (t == Type_Get<int>() || t == Type_Find("Int"))
    return Value::MakeInt(*static_cast<int*>(data));
  if (t == Type_Get<float>() || t == Type_Get<double>() || t == Type_Find("Float"))
    return Value::MakeFloat(*static_cast<float*>(data));
  if (t == Type_Get<bool>() || t == Type_Find("Bool"))
    return Value::MakeBool(*static_cast<bool*>(data));
  if (t == Type_Get<String>() || t == Type_Find("String"))
    return Value::MakeString(*static_cast<String*>(data));

  /* Engine-managed types (Vec3, Object, Widget, ...): borrow the slot. */
  return Value::MakePtr(t, data, /*owned*/ false);
}

void Evaluator::ValueToSlot(Value const& v, Type t, void* dest) {
  if (!dest || v.IsNone())
    return;

  if (t) {
    if ((t == Type_Get<int>() || t == Type_Find("Int")) && v.IsInt()) {
      *static_cast<int*>(dest) = v.AsInt();
      return;
    }
    if ((t == Type_Get<float>() || t == Type_Get<double>() || t == Type_Find("Float")) && v.IsFloat()) {
      *static_cast<float*>(dest) = v.AsFloat();
      // Allow int -> float promotion
      return;
    }
    if ((t == Type_Get<float>() || t == Type_Get<double>() || t == Type_Find("Float")) && v.IsInt()) {
      *static_cast<float*>(dest) = (float)v.AsInt();
      return;
    }
    if ((t == Type_Get<int>() || t == Type_Find("Int")) && v.IsFloat()) {
      *static_cast<int*>(dest) = (int)v.AsFloat();
      return;
    }
    if ((t == Type_Get<bool>() || t == Type_Find("Bool")) && v.IsBool()) {
      *static_cast<bool*>(dest) = v.AsBool();
      return;
    }
    if ((t == Type_Get<String>() || t == Type_Find("String")) && v.IsString()) {
      *static_cast<String*>(dest) = v.AsString();
      return;
    }
  }

  /* Engine-managed type: copy the payload into the destination slot via the
     type's assignment operator. Script instances arrive Data-boxed; unwrap
     before assigning into a script-typed slot. */
  if (v.data && t && t->assign) {
    void* src = v.data;
    if (IsDataWrapped(v) && IsScriptType(t))
      src = static_cast<Data*>(v.data)->data;
    if (src) {
      t->assign(t.t, src, dest);
      return;
    }
  }
  if (v.data && t && t->size) {
    std::memcpy(dest, v.data, t->size);
    return;
  }
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

Value Evaluator::CallScriptFunction(ScriptFunction const& sf,
                                    Vector<Value> const& args) {
  if (!sf || !sf->astFunc) {
    RuntimeError(sf ? "script function '" + sf->name + "' has no AST body"
                    : "null script function");
    return Value::MakeNone();
  }
  return CallFunction(sf->astFunc, args, sf->astImplicitThis);
}

Value Evaluator::CallScriptFunction(String const& name, Vector<Value> const& args, SourceLocation const& loc) {
  /* First-class function in scope (nested declarations) wins. */
  Value fnVal = Lookup(name);
  if (fnVal.kind == Value::FUNC_REF && fnVal.data) {
    ASTFuncDeclNodeT* fn = static_cast<ASTFuncDeclNodeT*>(fnVal.data);
    return CallFunction(fn, args, false);
  }

  if (script.t) {
    ScriptFunction sf = script->GetFunction(name);
    if (sf)
      return CallScriptFunction(sf, args);

    /* ``(Box center size)``: a name that is a script TYPE (not a function)
       constructs a default instance with the args stored into its fields in
       declaration order. */
    ScriptType st = script->GetType(name);
    if (st) {
      Value inst = MakeScriptTypeValue(st);
      if (!inst.IsNone()) {
        size_t n = Min(args.size(), st->fields.size());
        for (size_t i = 0; i < n; ++i)
          FieldSet(inst, st->fields[i].name, args[i]);
        return inst;
      }
    }
  }

  /* No script function: try the engine registry. */
  return CallEngineFunction(name, args, loc);
}

Vector<Function> Evaluator::FindFunctions(String const& name) {
  return Function_Find(name);
}

void* Evaluator::ValueArgPtr(Value const& v) const {
  switch (v.kind) {
    case Value::INT:   return const_cast<int*>(&v.intVal);
    case Value::FLOAT: return const_cast<float*>(&v.floatVal);
    case Value::BOOL:  return const_cast<bool*>(&v.boolVal);
    case Value::STRING:
    default:           return v.data;
  }
}

bool Evaluator::ScopeHasName(String const& name) const {
  Scope* s = currentScope.t;
  while (s) {
    if (s->locals.get(name))
      return true;
    s = s->parent.t;
  }
  return false;
}

// ─── Script-type fields (member access on script values) ────────────

ScriptType Evaluator::ScriptTypeOf(Value const& recv) const {
  if (!recv.type)
    return nullptr;
  if (IsDataWrapped(recv)) {
    Data* d = static_cast<Data*>(recv.data);
    if (!d || !d->type)
      return nullptr;
    Data const& aux = d->type->GetAux();
    if (!aux || !aux.IsType<ScriptType>())
      return nullptr;
    return aux.Convert<ScriptType>();
  }
  Data const& aux = recv.type->GetAux();
  if (!aux || !aux.IsType<ScriptType>())
    return nullptr;
  return aux.Convert<ScriptType>();
}

Field const* Evaluator::FindField(ScriptType st, String const& name) const {
  if (!st)
    return nullptr;
  for (size_t i = 0; i < st->fields.size(); ++i) {
    if (st->fields[i].name == name)
      return &st->fields[i];
  }
  return nullptr;
}

Value Evaluator::FieldGet(Value const& recv, String const& name) const {
  ScriptType st = ScriptTypeOf(recv);
  Field const* f = FindField(st, name);
  void* inst = ScriptInstance(recv);
  if (!f || !inst)
    return Value::MakeNone();
  return ValueFromSlot(f->type, static_cast<char*>(inst) + f->offset);
}

bool Evaluator::FieldSet(Value const& recv, String const& name,
                         Value const& value) {
  ScriptType st = ScriptTypeOf(recv);
  Field const* f = FindField(st, name);
  void* inst = ScriptInstance(recv);
  if (!f || !inst)
    return false;
  char* dest = (char*)inst + f->offset;
  ValueToSlot(value, f->type, dest);
  return true;
}

Value const* Evaluator::ThisValue() const {
  Scope* s = currentScope.t;
  while (s) {
    Value const* v = s->locals.get("this");
    if (v)
      return v;
    s = s->parent.t;
  }
  return nullptr;
}

ScriptType Evaluator::ThisType() const {
  Value const* v = ThisValue();
  return v ? ScriptTypeOf(*v) : nullptr;
}

Value Evaluator::ThisFieldGet(String const& name) const {
  Value const* v = ThisValue();
  return v ? FieldGet(*v, name) : Value::MakeNone();
}

bool Evaluator::ThisFieldSet(String const& name, Value const& value) {
  Value const* v = ThisValue();
  bool ok = v && FieldSet(*v, name, value);
  return ok;
}

Value Evaluator::ApplyBinaryOp(String const& op, Value const& lhs,
                               Value const& rhs) {
  if (op == "=")
    return rhs;
  if (lhs.IsInt() && rhs.IsInt()) {
    int l = lhs.AsInt(), r = rhs.AsInt();
    if (op == "+=")  return Value::MakeInt(l + r);
    if (op == "-=")  return Value::MakeInt(l - r);
    if (op == "*=")  return Value::MakeInt(l * r);
    if (op == "/=")  return Value::MakeInt(r != 0 ? l / r : 0);
  }
  if (lhs.IsFloat() && rhs.IsFloat()) {
    float l = lhs.AsFloat(), r = rhs.AsFloat();
    if (op == "+=")  return Value::MakeFloat(l + r);
    if (op == "-=")  return Value::MakeFloat(l - r);
    if (op == "*=")  return Value::MakeFloat(l * r);
    if (op == "/=")  return Value::MakeFloat(r != 0 ? l / r : 0);
  }
  if (lhs.IsString() && rhs.IsString() && op == "+=")
    return Value::MakeString(lhs.AsString() + rhs.AsString());
  /* Engine/collection operands (`self.plates += ...` -> List_Append): route
     through the engine operator overload. A void (NONE) result means a
     mutating op already applied in place, so the caller keeps the old target. */
  Value r = CallEngineFunction(op, {lhs, rhs}, SourceLocation());
  if (r.kind != Value::NONE)
    return r;
  RuntimeError("unsupported compound assignment operator: " + op);
  return Value::MakeNone();
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
