// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Unit tests for the LTSL binding bridge: Function_Bind / Function_Bind_Member
// (see ltsl-binding-bridge-replacement.md §5.2–§5.4). These bindings are the
// new seam between C++ functions and the LTSL interpreter.
//
// Coverage matrix (plan §5.4): free-fn bind, lambda bind, void-return
// dispatch, const-ref/by-value param decay, raw-pointer params, 0-arity,
// overload buckets, member bind, receiver fixup via TypeT::AddFunction,
// alias-after-source, and binding lifetime.
//
// NOTE: the "parameter name count must equal parameter count" mismatch is a
// COMPILE-TIME static_assert in Function_Bind / Function_Bind_Member — it
// cannot be exercised at runtime; the build itself is the guarantee.
//
// Test bindings use T_-prefixed names and only exist inside the lte_tests
// process (the API-DB dump is a separate binary), so they never leak into the
// language-server database.

#include "Harness.h"
#include "LTE/FunctionBind.h"
#include "LTE/Function.h"
#include "LTE/String.h"
#include "LTE/Type.h"
#include "LTE/Vector.h"

using namespace LTE;

// ── test subjects ────────────────────────────────────────────────────────

struct TestObj {
  using BaseType = NoBase;
  using SelfType = TestObj;

  int Method(int a, int b) {
    return a * 10 + b;
  }

  int ConstMethod(int x) const {
    return x + 1;
  }

  FIELDS {}
  DefineMetadataInline(TestObj)
};

struct MarketData {
  using BaseType = NoBase;
  using SelfType = MarketData;

  int value;

  FIELDS {
    MAPFIELD(value)
  }
  DefineMetadataInline(MarketData)
};

static int FreeFn(int a, int b) {
  return a * 10 + b;
}

static int ZeroFn() {
  return 42;
}

static int g_voidAccum = 0;

static void VoidFn(int x) {
  g_voidAccum += x;
}

static int RefParams(String const& s, int n) {
  return (int)s.size() + n;
}

static int PointerParams(MarketData* data, int qty) {
  return data->value * qty;
}

static int CallInt(Function fn, void** args) {
  int result = 0;
  fn->call(fn->binding, args, &result);
  return result;
}

// ── tests ────────────────────────────────────────────────────────────────

LTE_TEST(T_FreeFnBind) {
  Function fn = Function_Bind("T_FreeFn", "desc", &FreeFn, "a", "b");
  LTE_CHECK_EQ(fn->name, String("T_FreeFn"));
  LTE_CHECK_EQ(fn->description, String("desc"));
  LTE_CHECK_EQ(fn->paramCount, uint(2));
  LTE_CHECK_EQ(fn->params[0].name, String("a"));
  LTE_CHECK_EQ(fn->params[1].name, String("b"));
  LTE_CHECK_EQ(fn->params[0].type, Type_Get<int>());
  LTE_CHECK_EQ(fn->params[1].type, Type_Get<int>());
  LTE_CHECK_EQ(fn->returnType, Type_Get<int>());

  int a = 2, b = 3;
  void* args[] = { &a, &b };
  LTE_CHECK_EQ(CallInt(fn, args), 23);
}

LTE_TEST(T_LambdaBind) {
  // The FF migration shape: a stateless lambda (finding #11: empty-capture).
  Function fn = Function_Bind(
    "T_Lambda", "desc", [](int x) -> int { return x * 2; }, "x");
  LTE_CHECK_EQ(fn->paramCount, uint(1));
  LTE_CHECK_EQ(fn->params[0].type, Type_Get<int>());
  LTE_CHECK_EQ(fn->returnType, Type_Get<int>());

  int x = 21;
  void* args[] = { &x };
  LTE_CHECK_EQ(CallInt(fn, args), 42);
}

LTE_TEST(T_VoidReturnDispatch) {
  g_voidAccum = 0;
  Function fn = Function_Bind("T_VoidReturn", "desc", &VoidFn, "x");
  LTE_CHECK_EQ(fn->returnType, Type_Get<void>());

  int x = 5;
  void* args[] = { &x };
  int result = 0;
  fn->call(fn->binding, args, &result);   // out is ignored on the void path
  LTE_CHECK_EQ(g_voidAccum, 5);
}

LTE_TEST(T_ConstRefByValueParams) {
  // References decay to the value type for the parameter registry (plan §5.2).
  Function fn = Function_Bind("T_RefParams", "desc", &RefParams, "s", "n");
  LTE_CHECK_EQ(fn->paramCount, uint(2));
  LTE_CHECK_EQ(fn->params[0].type, Type_Get<String>());
  LTE_CHECK_EQ(fn->params[1].type, Type_Get<int>());

  String s = "hello";
  int n = 100;
  void* args[] = { &s, &n };
  LTE_CHECK_EQ(CallInt(fn, args), 105);
}

LTE_TEST(T_RawPointerParam) {
  Function fn = Function_Bind("T_Pointer", "desc", &PointerParams, "data", "qty");
  LTE_CHECK_EQ(fn->paramCount, uint(2));
  LTE_CHECK(fn->params[0].type);

  MarketData data;
  data.value = 7;
  MarketData* dataPtr = &data;       // in[0] points to the pointer slot
  int qty = 3;
  void* args[] = { &dataPtr, &qty };
  LTE_CHECK_EQ(CallInt(fn, args), 21);
}

LTE_TEST(T_ZeroArity) {
  Function fn = Function_Bind("T_Zero", "desc", &ZeroFn);
  LTE_CHECK_EQ(fn->paramCount, uint(0));
  LTE_CHECK(!fn->params);
  LTE_CHECK_EQ(CallInt(fn, nullptr), 42);
}

LTE_TEST(T_OverloadBucket) {
  // Same-name bindings are grouped, not errors — mirrors RNG_Int (plan §6.8).
  Function a = Function_Bind("T_Over", "desc", &FreeFn, "a", "b");
  Function b = Function_Bind("T_Over", "desc", &ZeroFn);
  (void)a;
  (void)b;

  Vector<Function> funcs = Function_Find("T_Over");
  LTE_CHECK_EQ(funcs.size(), size_t(2));

  int x = 1, y = 2;
  void* args[] = { &x, &y };
  LTE_CHECK_EQ(CallInt(funcs[0], args), 12);
  LTE_CHECK_EQ(CallInt(funcs[1], nullptr), 42);
}

LTE_TEST(T_MemberBind) {
  Function fn = Function_Bind_Member(
    "T_Member", "desc", &TestObj::Method, "a", "b");
  LTE_CHECK_EQ(fn->paramCount, uint(3));   // receiver + 2
  LTE_CHECK_EQ(fn->params[0].name, String("object"));
  LTE_CHECK(!fn->params[0].type);          // null until TypeT::AddFunction
  LTE_CHECK_EQ(fn->params[1].type, Type_Get<int>());
  LTE_CHECK_EQ(fn->params[2].type, Type_Get<int>());

  TestObj obj;
  int a = 2, b = 3;
  void* args[] = { &obj, &a, &b };
  LTE_CHECK_EQ(CallInt(fn, args), 23);
}

LTE_TEST(T_ConstMemberBind) {
  Function fn = Function_Bind_Member(
    "T_ConstMember", "desc", &TestObj::ConstMethod, "x");
  LTE_CHECK_EQ(fn->paramCount, uint(2));

  TestObj obj;
  int x = 41;
  void* args[] = { &obj, &x };
  LTE_CHECK_EQ(CallInt(fn, args), 42);
}

LTE_TEST(T_ReceiverFixup) {
  // TypeT::AddFunction replaces params[0]'s null type with the receiver type
  // (Type.cpp:155) — the Vector registration path.
  Function fn = Function_Bind_Member(
    "T_MemberFix", "desc", &TestObj::Method, "a", "b");
  Type objType = Type_Get<TestObj>();
  objType->AddFunction(fn);
  LTE_CHECK_EQ(fn->params[0].type, objType);
}

LTE_TEST(T_AliasAfterSource) {
  Function src = Function_Bind("T_Src", "desc", &FreeFn, "a", "b");
  (void)src;
  (void)Function_Alias("T_Src", "T_Alias");

  Vector<Function> aliases = Function_Find("T_Alias");
  LTE_CHECK_EQ(aliases.size(), size_t(1));
  LTE_CHECK(aliases[0]);

  int x = 4, y = 5;
  void* args[] = { &x, &y };
  LTE_CHECK_EQ(CallInt(aliases[0], args), 45);
}

LTE_TEST(T_BindingLifetime) {
  // ~FunctionT deletes the binding exactly once. A double-free/leak is caught
  // under ASAN (plan §5.4); this exercises the dtor path in normal builds.
  {
    Function fn = Function_Bind("T_Lifetime", "desc", &ZeroFn);
    LTE_CHECK(fn);
    LTE_CHECK_EQ(CallInt(fn, nullptr), 42);
  }
  LTE_CHECK(true);
}
