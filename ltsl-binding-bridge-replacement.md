# LTSL Binding Bridge Replacement — Master Plan & Handoff Document

**Status:** IN PROGRESS — design finalized, migration not started.
**Owner:** darkoned12000 (Revamp Work, GPL-3.0).
**Rollback point:** commit `9fdc702` (crash-fix work, clean build + tests). Any
migration commit is recoverable from here.
**Companion docs:** `AGENTS.md` §6–§8, `docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md`.

> Read this whole document before resuming work. It is the single source of
> truth for *why*, *what*, *how*, and *where we are*. Progress is tracked in
> §12 (checklist).

---

## 1. Mission

Replace the two fragile, arity-counting **macro families** that bind C++
functions into the LTSL script engine — `Function_Generated.h` (1590 lines)
and `DeclareFunction.h` (882 lines), plus the Python generator that produced
them — with a hand-written **C++17 variadic-template core** and a
**macro-free `Bind()` API**. Rewrite **all ~700 call sites** to the new API.
Delete the generated headers and the generator.

**User decision (verbatim intent):** "Full macro-free Bind API … if the macros
can pose a problem why keep them around? … if doing this can assist with
modernizing the engine then I am in favor of a full replacement." Breakage risk
is accepted; the latest commit is a safe rollback point.

A second AI design review was performed (`general` agent, full findings in §8)
before committing to this path. Its corrections are folded into the design.
A third review (Claude) flagged five concrete issues; its addendum is folded in
as §5.1a (signature-coexistence shim), §6.8 (name-collision pre-pass), the
`remove_cvref_t` polyfill in §5.2, the automated alias-order gate in §8 gate 6,
and the SIOF note below.

**SIOF scope (explicit expectation):** this migration fixes the **arity-macro
fragility** (§3.1/§3.2) — it does **NOT** fix static-initialization-order
fragility. The new binder uses the same eager static-init shape
(`static Function const X = Function_Bind(...)`; §5.2) as the old
`static Function Name##_Metadata`, so cross-TU static-init order stays
unspecified and **unchanged**. If a post-migration SIOF bug appears, it is the
pre-existing bug class (AGENTS.md A.1/A.4, §3.3), NOT something the new binder
introduced. Do not chase it as a regression of this work.

---

## 2. Current Architecture (what we are replacing)

### 2.1 The runtime contract

```cpp
// src/liblt/LTE/Function.h
struct FunctionT : public RefCounted {
  String name;
  String description;
  void (*call)(void**, void*);   // <-- the ONLY thing the interpreter needs
  uint paramCount;
  Parameter const* params;       // raw new[] array; freed in ~FunctionT
  Type returnType;
};
using Function = Reference<FunctionT>;   // Common.h:127
```

- `Function_Create(name)` (`Function.cpp:28`) allocates a `FunctionImpl`,
  pushes it into a global `Vector<Function>` (`GetFunctionList`) and a global
  `Map<String, Vector<Function>>` (`GetFunctionMap`), keyed by script name.
  Both live forever (registry holds refs).
- `Function_ForEach`/`Function_Find`/`Function_GetList` read them. The LSP API
  database (`script/ltsl-lsp/tools/dump_api.cpp`) dumps `Function_ForEach` →
  `fn->GetSignature()`, `returnType`, `params[i].name/type`, `description`.
- **The interpreter calls the trampoline exactly once:**
  `src/liblt/LTE/Expression/FunctionCall.cpp:74`:
  ```cpp
  function->call(argStack.data(), returnValue);
  ```
  `argStack[i]` points to the typed lvalue of argument `i` (either a register
  slot allocated via `env.Allocate(arg.type)` or an lvalue expression). The
  trampoline is responsible for: cast `*(T_i*)in[i]`, call the impl, write
  `*(RT*)out = result` (or just call, when RT is void).
- `~FunctionT()` (`Function.cpp:42`) does `delete[] params`.

### 2.2 `Function_Generated.h` — the inline-definition family (~556 sites)

Macros: `FreeFunction`, `VoidFreeFunction`, `MemberFunction`,
`ConstMemberFunction`, `VirtualMemberFunction`, `ConstVoidMemberFunction`,
`VirtualVoidMemberFunction`, `ConstVirtualMemberFunction`,
`ConstVirtualVoidMemberFunction`, and `...NoParams` variants of each.
Arities 0–8 (`FreeFunction0`…`FreeFunction8`, 1590 lines total).

A call site (all in `.cpp`, at global scope):

```cpp
FreeFunction(String, Object_GetName,
  "Return the name of 'object'",
  Object, object)
{
  return object->GetName();
} FunctionAlias(Object_GetName, GetName);
```

Each `FreeFunctionN(Prefix, Postfix, ReturnType, Name, Description, T0..T7, N0..N7)`
expands to:
1. `LT_API Prefix ReturnType Name##_Impl(T0 const& N0, ...) Postfix;` — forward decl.
2. `Name##_Call(void** in, void* out)` — the trampoline:
   `*(ReturnType*)out = Name##_Impl(*(T0*)in[0], ...);`
3. `template<int unused> Function Name##_GetMetadata()` — builds the Function:
   `Function_Create(#Name)`, sets `call`/`description`/`paramCount`/`params`
   (`new Parameter[n]` with `(#N0, Type_Get<T0>())`)/`returnType`
   (`Type_Get<ReturnType>()`).
4. `static Function Name##_Metadata = Name##_GetMetadata<0>();` — eager
   static-init registration. The `template<int>` is an ODR-avoidance trick.
5. `Prefix ReturnType Name##_Impl(T0 const& N0, ...) Postfix` — opens the user body.

`MemberFunctionN` additionally reads the receiver from `in[0]`:
`((SelfType*)in[0])->Name##_Impl(...)` and sets `params[0] = Parameter("object", nullptr)`
(see §2.5 for why `nullptr`).

**Arity dispatch (the fragile core):** the public macro appends a counting
tail and re-dispatches:
```cpp
#define FreeFunction(RT, Name, Desc, ...) \
  MACRO_IDENTITY(_FreeFunction(RT,Name,Desc,__VA_ARGS__,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0))
```
`_FreeFunction` picks `x` = number of arg pairs from the tail position and
expands `FreeFunction##x(...)`. Failure modes:
- Passing **more than 8 args** silently truncates the extra params.
- Passing a wrong count produces a cryptic token-paste error.
- The `T0..T7/N0..N7` names in each arity macro are *placeholders*; real
  names come only from `#N0` stringization — one tiny typo silently changes
  the script param name.
- No compile-time check that `sizeof...(args)` matches anything.

### 2.3 `DeclareFunction.h` — the declare-in-header / define-in-cpp family (~294 decl + ~280 def)

Macros: `DeclareFunction(Name, ReturnType, ...)` (arity 0–24, i.e. 0–12
params), `DeclareFunctionArgBind` (adds a persistent `_Args` value struct),
`DeclareFunctionNoParams`, and `DefineFunction(Name)`.

A pair (e.g. `Component/Zoned.h` + `Zoned.cpp`):

```cpp
// header
DeclareFunction(Object_GetZone, Zone,
  Object, object)
// cpp
DefineFunction(Object_GetZone) {
  return object->GetZone();
} FunctionAlias(Object_GetZone, GetZone);
```

`DeclareFunctionN` expands in the header to:
- `Name##_ParamCount()`, `Name##_ParamName(i)` (a `char const* const table[]`).
- `typedef ReturnType Name##_ReturnType; typedef T0 Name##_ParamType0; …`
- `struct Name##_ArgRefs { T0 const& N0; … ctor };` — a **bundle of
  references** passed to the impl.
- `LT_API ReturnType Name(Name##_ArgRefs const&);` — the impl declaration.
- inline overloads `Name(T0 const& N0, …)` → `Name(Name##_ArgRefs(N0, …))`
  (this is what C++ callers use), plus `Name()` for 0-arg.
- `Name##_ExplicitCall(T0 const& N0, …)` → forwards to `Name(ArgRefs)` — the
  fn used for metadata/trampoline.
- `Name##_Call(void** in, void* out)` → `CallAndAssign(in, out, Name##_ExplicitCall)`.

`DefineFunction(Name)` = `RegisterFunction(Name) Name##_ReturnType Name(Name##_ArgRefs const& args)`,
and `RegisterFunction` (in the header) does the `Function_Create` +
`Infer_MetaData(fn, &Name##_ExplicitCall)` + `static Function Name##_Metadata`.

`Infer_MetaData`/`CallAndAssign` are the **per-arity template overloads**
(one for 0..12 params, duplicated for void/non-void) at the bottom of each
arity block — 25 duplicated template families.

`DeclareFunctionArgBindN` additionally emits:
```cpp
AutoClass(Name##_Args, T0, N0, T1, N1, …)          // persistent VALUE struct
  Name##_Args() {}
  Name##_Args(Name##_ArgRefs const& args) : N0(args.N0), … {}
};
inline ReturnType Name(Name##_Args const& args) { … }
```
The `_Args` struct is **load-bearing C++ data** (see §7.2).

**Critical:** the script-visible signature for ArgBind functions is the
**unwrapped per-param** signature, not the `_Args` struct —
`Infer_MetaData(fn, &Name##_ExplicitCall)` deduces from
`Name##_ExplicitCall(T0 const&, T1 const&, …)`. Confirmed by the API DB dump:
`Generator_Nebula(Float roughness, Float seed, Color color1, Color color2, …)`.

`DeclareFunction` sites have **no description** — `RegisterFunction` sets
`fn->description = "None"`.

### 2.4 `DefineConversion`, `FunctionAlias`, `TypeAlias` (Function.h / Type.h)

```cpp
#define DefineConversion(Name, SourceType, DestType)   // Function.h:43
  LT_API void Name(SourceType const&, DestType&);      // + trampoline + registration
#define FunctionAlias(source, alias)                   // Function.h:67
  // registers Function_AddAlias(#source, #alias) at static init
#define TypeAlias(SourceType, alias)                   // Type.h:99
  // registers Type_AddAlias(Type_Get<SourceType>(), #alias) at static init
```

- `DefineConversion` body is defined inline after the macro (e.g.
  `Color.cpp:14` `DefineConversion(int_to_color, int, Color) { dest = (Color)src; }`).
  Conversion trampolines have a **different signature**:
  `ConversionFn = void(*)(TypeT*, void const*, void*)` (`Type.h:125`) — NOT the
  function trampoline. Do not unify them.
- Conversion functions are never called cross-TU (verified: 0 external callers).
- `FunctionAlias` is used 523 times, almost always textually immediately after
  the defining binding in the same TU (e.g. `} FunctionAlias(Object_GetZone, GetZone);`).
  `Function_AddAlias` copies the *current* source bucket into the alias bucket
  — so an alias registered **before** its source yields a silently empty alias.

### 2.5 Special runtime machinery to preserve

**Vector member functions (lazy registration).** `src/liblt/LTE/Vector.h:299–325`
has the only 4 `MemberFunction` sites, inside `template<class T> struct Vector`:

```cpp
VoidMemberFunction(Append, "…", T, element) { this->push(element); }
MemberFunction(T, Get, "…", int, index)     { return (*this)[index]; }
VoidMemberFunction(Set, "…", int, index, T, element) { (*this)[index] = element; }
MemberFunctionNoParams(int, Size, "…")      { return (int)this->size(); }
// …
METADATA {
  MEMBERFUNCTION(Append) MEMBERFUNCTION(Get) MEMBERFUNCTION(Size) MEMBERFUNCTION(Set)
}
```

Registration is **lazy** and **per-class-template-specialization**:
- `METADATA` = `static void FillMetadata(Type const& type)` (`Type.h:118`).
  It is invoked by the reflection `_Type_Get` path during type creation
  (`Type.h:46/71/94`, `BaseType.h:29/64`) — i.e. when `Type_Get<Vector<T>>()`
  is first resolved at runtime.
- `MEMBERFUNCTION(name)` = `type->AddFunction(name##_GetMetadata<0>())`.
- `TypeT::AddFunction` (`Type.cpp:155`) **fixes up the receiver param**:
  `Mutable(fn->params[0]) = Parameter("object", this);` — so the macro can
  leave `params[0].type = nullptr`, and the *type* fills it in.
- The class-template static `Name##_Metadata` is never odr-used → never
  instantiated → eager registration never fires. **Laziness is load-bearing.**

**ArrayCustom** (`Type/Array.cpp:180–202, 262–323`): 6 hand-written handlers
(`Append/AppendArray/Clear/Get/Remove/Size`) with `void(void**, void*)`
signature; registered via `Function_Create("+=" / "Clear" / "Get" / "Remove" /
"Size")`, `params[0] = Parameter("object", self)`, and additionally pushed onto
`self->GetFunctions()`.

**`_Impl` functions are pure binding glue.** Verified: no `_Impl` function is
ever called from C++ outside its own macro expansion. They can be fully hidden
inside the new API (as lambdas / function bodies).

---

## 3. Why This Is Fragile (the concrete case)

1. **Arity-counting dispatch** (§2.2, §2.3) — silent truncation past 8 args,
   cryptic errors, no compile-time arity checking.
2. **~2,650 lines of generated boilerplate** maintained by a Python generator
   (`script/meta/DeclareFunction.py`) that is **not wired into the build** —
   the checked-in `DeclareFunction.h` and the generator have already drifted
   (the header has hand-added `[[maybe_unused]]` fixes the generator lacks).
3. **Static-init SIOF hazards**: `Type_Get<T>()` + `Function_Create()` + alias
   registration all run at static-init in unspecified cross-TU order. This
   family has already been implicated in the `Reference<unknown type>` SIOF
   bug class (AGENTS.md A.1/A.4).
4. **Raw unchecked casts**: `*(ReturnType*)out = …` / `*(T0*)in[0]` with no
   safety; wrong type at a call site is a silent memory bug, not an error.
5. **`volatile static` registration + `template<int unused>` ODR tricks** are
   subtle and easy to misread; headers included in 39 TUs (DeclareFunction.h)
   already produce per-TU duplicate registrations (pre-existing, harmless).
6. **Three parallel hand-written template families** (`CallAndAssign`,
   `Infer_MetaData`, per-arity macros) must stay in sync; adding an arity
   requires touching all three.
7. The two macro families are **redundant with each other** (same trampoline +
   metadata pattern, different call-site shapes), doubling the surface.

---

## 4. Design Review Findings (second-opinion agent, folded in)

Performed against the real codebase. Full report summarized:

| # | Finding | Severity | Mitigation (adopted) |
|---|---------|----------|----------------------|
| 1 | `tests/TestStringBindings.cpp:126,131,144,162` call `fn->call(args,&result)` directly | blocker | Update the 4 sites to the new 3-arg signature (pass `fn->binding`) |
| 2 | `FunctionT` layout change is safe (RefCounted, no value copies/serialize), but `binding` must be init'd on **every** creation path | important | Init in `Function_Create`, ArrayCustom, ScriptFunction |
| 3 | C++17 confirmed (`CMakeLists.txt:195`), `-fno-exceptions`, RTTI on, `-Werror` on `lt`/`launch` | ok | Binder must be `-Werror`-clean; `std::apply` fine |
| 4 | ODR: binding registration statics in headers already duplicate per-TU (pre-existing) — do not make worse | info | Keep registrations in `.cpp`; header-only bindings only where unavoidable |
| 5 | **Alias-after-source invariant** — `Function_AddAlias` copies current source bucket | important | Keep alias textually after its source; audit |
| 6 | **`_Args` structs must unwrap to per-param script signatures**; they are C++-side data (10 glyph base classes, `Meta_*` aliases, convenience overloads), not script types | important | ArgBind registration via a per-param wrapper; keep `_Args` reflected struct + overloads |
| 7 | **Vector member functions: must keep laziness + raw `new Parameter[]` + `AddFunction` receiver fixup** | blocker | Member binder sets `params[0]=(“object”, nullptr)`; registered from `FillMetadata` |
| 8 | Generator not a build step; `common.py` helper is `macro()` (not `mac()`); only used by `DeclareFunction.py`; `src/old/codegen` is dead | ok | Delete generator + `common.py`; leave `src/old` |
| 9 | Raw-pointer params exist (`MarketData*`, `TaskInstance*`, `Component##x*`) but `remove_cvref_t` handles them; **no `char const*` params anywhere** | ok | Use `remove_cvref_t` for cast + `Type_Get` |
| 10 | ArrayCustom migration trivial; registration bypasses `AddFunction` (sets `params[0]` itself) | ok | Update 6 handlers to 3-arg; set `binding=null` |
| 11 | No capturing lambdas needed (all impls reference only params + file-scope statics) | ok | Empty-capture lambdas |
| 12 | No `Tuple`/`Array` name conflicts; `<tuple>` safe | ok | — |
| – | **`test-rpc.js`/`smoke.js` only validate the LSP DB, not the runtime trampoline** | important | Add **app runs** to the verification gate |
| – | Likely silent breakers ranked: receiver fixup → eager Vector registration → `std::vector<Parameter>` while `AddFunction` writes raw → `_Args` collapsed to one param → uninit `binding` double-free → alias-before-source | — | See design below |

> **NOTE (baseline reality, committed 2026-08-03):** `TestStringBindings.cpp` is
> **unregistered WIP and does NOT build** against the committed engine — it
> references `String_Trim` and a String-delimiter
> `String_Split(Vector<String>&, const String&, String)` overload that do not
> exist yet (`String.h:196` has only the `char` overload; there is no
> `String_Trim` anywhere). It is excluded from the green suite (274 checks).
> The finding-1 `fn->call` sites at `:126,131,144,162` are real but **dormant** —
> they only become active once (a) the String API work lands and (b) the file is
> added to `tests/CMakeLists.txt`. Leave it untracked until then; migrate the 4
> sites when registering it.

---

## 5. New Architecture

### 5.1 `FunctionT` contract change (`Function.h`)

```cpp
struct BindingBase;   // forward-declared; defined in FunctionBind.h

struct FunctionT : public RefCounted {
  String name;
  String description;
  void (*call)(void* binding, void** in, void* out);  // WAS (void**, void*)
  BindingBase* binding;                                // owned; null for manual handlers
  uint paramCount;
  Parameter const* params;
  Type returnType;
  LT_API ~FunctionT();    // add: delete binding;
};
```

- `Expression/FunctionCall.cpp:74` becomes
  `function->call(function->binding, argStack.data(), returnValue);`
- `Function.cpp:32` init: `self->call = 0; self->binding = 0;`
- `~FunctionT` adds `delete binding;` (BindingBase has virtual dtor).
- Blast radius (verified): 1 invocation site, 6 ArrayCustom handlers,
  `ScriptFunction_Call` stub (`Expression/Function.cpp:9`), 4 test call sites,
  `Function_Create`. Nothing else reads/writes `.call`.

### 5.1a Coexistence during migration — the Step-1 compatibility shim (resolves a real sequencing flaw)

Claude review caught a **planning bug in the original Step 1**: §5.1 flips
`FunctionT::call` to 3-arg in one step, but §2.2/§2.3's macro-generated
trampolines (still present through Steps 2–9 per §9) emit the **old 2-arg**
`Name##_Call(void** in, void* out)` and assign it to `fn->call` at their own
eager static-init sites across the ~700 un-migrated call sites. The moment the
field type changes, **every one of them is a function-pointer type mismatch** —
a compile failure at Step 1, not at some later gate. Step 1 is not
independently buildable as originally scoped.

**Resolution: a temporary compatibility shim on the two macro headers** (both
are deleted wholesale in Step 10, so the shim dies with them and never needs to
be clean). Mechanically, prepend an unnamed `void*` first parameter to every
emitted trampoline signature — an unnamed parameter cannot trigger
`-Wunused-parameter`, and the body still reads `in`/`out` unchanged:

```bash
# apply to BOTH src/liblt/LTE/Function_Generated.h and src/liblt/LTE/DeclareFunction.h
python3 - <<'EOF'
import re, sys
for p in sys.argv[1:]:
    t = open(p).read()
    t2 = re.sub(r'Name##_Call\(\s*void\*\* in,', 'Name##_Call(void*, void** in,', t)
    assert t2 != t, p
    open(p, 'w').write(t2)
EOF
src/liblt/LTE/Function_Generated.h src/liblt/LTE/DeclareFunction.h
```

- `Function_Generated.h`: ~90 arity/variant macro definitions (FreeFunction/
  VoidFreeFunction/MemberFunction/…/NoParams) each get the extra leading
  `void*,`. The member variants still read the receiver from `in[0]` — the
  leading unnamed param shifts nothing because `in` is unchanged.
- `DeclareFunction.h`: 12 `Name##_Call` signature lines. The 25
  `CallAndAssign`/`Infer_MetaData` template families stay **2-arg** — only the
  `Name##_Call` wrapper that assigns to `fn->call` needs the 3-arg shape.
  `CallAndAssign` is invoked as `CallAndAssign(in, out, …)`; untouched.
- The `[[maybe_unused]]` on `CallAndAssign`'s `in` (line 45) is already
  there and is compatible.

**Consequence:** with the shim, the tree compiles green the instant Step 1
lands; per-subsystem migration proceeds chunk-by-chunk exactly as §9 describes,
and Step 10's delete removes the shim. **`Function.h` must keep
`#include "Function_Generated.h"` until Step 10** (the original "delete the
include last" instruction is replaced by this rule). Verification: the Step-1
gate (build + tests) now exercises 100% of the old trampolines through the new
3-arg field, so the shim is validated at the very first build.

### 5.2 New header: `src/liblt/LTE/FunctionBind.h` (hand-written, ~150 lines)

Declared at **global scope** (like `Function_Create`) so call sites need no
namespace qualification.

```cpp
#ifndef LTE_FunctionBind_h__
#define LTE_FunctionBind_h__

#include "Function.h"
#include "Mutable.h"
#include <tuple>
#include <type_traits>
#include <utility>

/* C++17 polyfill for remove_cvref_t (P0550R2 is C++20, not in
   <type_traits> under C++17 — see CMakeLists.txt:195). Verified: no existing
   polyfill anywhere in src/. Use the unqualified name throughout this file. */
template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

/* Signature introspection. */
template <class T>
struct FunctionTraits;

template <class RT, class... Args>
struct FunctionTraits<RT (*)(Args...)> {
  using ReturnType = RT;
  using Args = std::tuple<Args...>;
};

template <class RT, class... Args>
struct FunctionTraits<RT (Args...)> {           // function reference form
  using ReturnType = RT;
  using Args = std::tuple<Args...>;
};

template <class RT, class C, class... Args>     // member fn (non-const)
struct FunctionTraits<RT (C::*)(Args...)> {
  using ReturnType = RT;
  using Class = C;
  using Args = std::tuple<Args...>;
};

template <class RT, class C, class... Args>     // member fn (const)
struct FunctionTraits<RT (C::*)(Args...) const> {
  using ReturnType = RT;
  using Class = C;
  using Args = std::tuple<Args...>;
};

template <class T>                              // lambda / functor
struct FunctionTraits<T> : FunctionTraits<decltype(&T::operator())> {};

/* Owned binding closure; freed by ~FunctionT. */
struct BindingBase {
  virtual ~BindingBase() = default;
};

template <class RT, class ArgsTuple, class FnT>
struct FunctionBinding : BindingBase {
  FnT fn;

  template <class F>
  FunctionBinding(F&& f) : fn(std::forward<F>(f)) {}

  static void Call(void* self, void** in, void* out) {
    static_cast<FunctionBinding*>(self)->Invoke(
      in, out, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
  }

  template <size_t... Is>
  void Invoke(void** in, void* out, std::index_sequence<Is...>) {
    using ArgT = std::tuple_element_t<Is, ArgsTuple>;
    auto args = std::forward_as_tuple(
      *static_cast<remove_cvref_t<ArgT>*>(in[Is])...);
    if constexpr (std::is_void_v<RT>)
      std::apply(fn, args);
    else
      *static_cast<remove_cvref_t<RT>*>(out) = std::apply(fn, args);
  }
};

/* Fill params[i] with (name, Type_Get<remove_cvref_t<Arg>>()). */
template <class ArgsTuple, size_t... Is, class... NameT>
void FillParams(Parameter* params, std::index_sequence<Is...>,
                NameT const&... names) {
  using ArgT = std::tuple_element_t<Is, ArgsTuple>;
  ((Mutable(params[Is]) = Parameter(
      names, Type_Get<remove_cvref_t<ArgT>>())), ...);
}

template <class FnT, class... NameT>
Function Function_Bind(String const& name, String const& desc, FnT&& fn,
                       NameT const&... names) {
  using Traits = FunctionTraits<std::remove_reference_t<FnT>>;
  using RT = typename Traits::ReturnType;
  using ArgsTuple = typename Traits::Args;
  static constexpr size_t kArity = std::tuple_size_v<ArgsTuple>;
  static_assert(sizeof...(NameT) == kArity,
    "Function_Bind: parameter name count must equal parameter count");

  auto* binding = new FunctionBinding<RT, ArgsTuple, remove_cvref_t<FnT>>(
    std::forward<FnT>(fn));

  Function f = Function_Create(name);
  f->description = desc;
  f->call = &FunctionBinding<RT, ArgsTuple, remove_cvref_t<FnT>>::Call;
  f->binding = binding;
  f->paramCount = kArity;
  f->returnType = Type_Get<remove_cvref_t<RT>>();
  if constexpr (kArity > 0) {
    auto* params = new Parameter[kArity];
    FillParams<ArgsTuple>(params,
      std::make_index_sequence<kArity>{}, names...);
    f->params = params;
  }
  return f;
}

/* Alias registration (static-init; MUST appear after the source binding). */
inline int Function_Alias(char const* source, char const* alias) {
  Function_AddAlias(source, alias);
  return 0;
}

#endif
```

Notes:
- `remove_cvref_t` is the **local polyfill** at the top of the file (C++17 has
  no `std::remove_cvref_t` — that is C++20 P0550R2). `grep -rn "remove_cvref" src/`
  currently finds **nothing**, so this is the first and only definition; use the
  unqualified name everywhere inside `FunctionBind.h`. Missing it is a
  Step-1/line-1 compile failure under `CMakeLists.txt:195` (`CMAKE_CXX_STANDARD 17`).
- `Type_Get<remove_cvref_t<RT>>()`: `RT=void` hits the existing
  `Type_Get<void>()` specialization; references decay to the value type.
- `std::apply` (C++17) with a tuple of lvalue-refs; empty-capture lambdas
  only (per finding #11).
- Same eager static-init timing as the old macros
  (`static Function const X = Function_Bind(...)`); cross-TU order remains
  unspecified (pre-existing, unchanged).
- `Function_Alias` returns `int` so it can seed a `static int const`.

### 5.3 Member-function binder (Vector case) — `FunctionBind.h` addition

```cpp
template <class RT, class C, class ArgsTuple, class MemT>
struct FunctionMemberBinding : BindingBase {
  MemT mem;                                     // RT (C::*)(Args...)
  template <class M>
  FunctionMemberBinding(M&& m) : mem(std::forward<M>(m)) {}

  static void Call(void* self, void** in, void* out) {
    static_cast<FunctionMemberBinding*>(self)->Invoke(
      in, out, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
  }

  template <size_t... Is>
  void Invoke(void** in, void* out, std::index_sequence<Is...>) {
    using ArgT = std::tuple_element_t<Is, ArgsTuple>;
    C* receiver = static_cast<C*>(in[0]);
    auto args = std::forward_as_tuple(
      *static_cast<remove_cvref_t<ArgT>*>(in[Is + 1])...);
    if constexpr (std::is_void_v<RT>)
      std::apply(mem, std::tuple_cat(std::tie(receiver), args));   // or manual
    else
      *static_cast<remove_cvref_t<RT>*>(out) =
        std::apply(mem, std::tuple_cat(std::tie(receiver), args));
  }
};

template <class MemT, class... NameT>
Function Function_Bind_Member(String const& name, String const& desc,
                              MemT mem, NameT const&... names) {
  using Traits = FunctionTraits<MemT>;   // member-fn pointer
  using RT = typename Traits::ReturnType;
  using C = typename Traits::Class;
  using ArgsTuple = typename Traits::Args;
  static constexpr size_t kArity = std::tuple_size_v<ArgsTuple>;
  static_assert(sizeof...(NameT) == kArity,
    "Function_Bind_Member: name count must equal member param count");

  auto* binding = new FunctionMemberBinding<RT, C, ArgsTuple, MemT>(mem);

  Function f = Function_Create(name);
  f->description = desc;
  f->call = &FunctionMemberBinding<RT, C, ArgsTuple, MemT>::Call;
  f->binding = binding;
  f->paramCount = kArity + 1;                 // receiver + args
  auto* params = new Parameter[kArity + 1];
  params[0] = Parameter("object", nullptr);   // filled by TypeT::AddFunction
  FillParams<ArgsTuple>(params + 1, std::make_index_sequence<kArity>{}, names...);
  f->params = params;
  f->returnType = Type_Get<remove_cvref_t<RT>>();
  return f;
}
```

**Correctness requirements (from finding #7):**
- `params[0].type = nullptr` — `TypeT::AddFunction` replaces it with
  `Parameter("object", this)`.
- `params` must be a **raw `new Parameter[]`** array (AddFunction mutates via
  `Mutable`); never `std::vector<Parameter>`.
- Registration must be **lazy**, called from `FillMetadata`, not static-init.

### 5.4 Unit tests for the new binder (new file: `tests/TestFunctionBind.cpp`)

The binder is the new core seam between C++ and LTSL, so it gets its own
headless test file (no window/audio — same shape as `TestStringBindings.cpp`).
Register it in `tests/CMakeLists.txt` `TEST_SRC` (it already links `lt`).
The 4 `fn->call` sites in `TestStringBindings.cpp` (`:126,131,144,162`) are
updated to the 3-arg signature as part of Step 1 (finding #1) **when that file
is registered** — note it is currently unregistered, non-building WIP (see the
baseline note in §4), so migrating it is deferred until the String API lands.

**Coverage matrix** (each row maps to a design guarantee in §5.2/§5.3; use a
`T_`-prefixed name so nothing collides with real bindings; test bindings only
exist in the `lte_tests` process and never reach the API-DB dump, which is a
separate binary):

| Guarantee | Test |
|-----------|------|
| Free-fn bind | `Function_Bind("T_FreeFn", "desc", &FreeFn, "a", "b")` → `name`, `description`, `paramCount`, `returnType` correct; `fn->call(fn->binding, args, &out)` returns the right value |
| Lambda bind (the FF migration shape) | `Function_Bind("T_Lambda", …, [](int x) -> int { return x * 2; }, "x")` |
| Void RT dispatch | a void lambda / free fn — the `if constexpr` path (`std::apply(fn, args)` without `out`) |
| Const-ref & by-value params | `String const&` and `int` params → `params[i].type == Type_Get<String>()` / `Type_Get<int>()` (references decay to value type; §5.2) |
| Raw-pointer params | a pointer param (`MarketData*`-style) — cast via `remove_cvref_t` + call |
| 0-arity | `Function_Bind("T_Zero", "desc", &ZeroFn)` → `paramCount == 0`, `params == nullptr`, call works |
| Overload bucket | two `Function_Bind("T_Over", …)` with the same name → `Function_Find("T_Over").size() == 2`, both callable (mirrors `RNG_Int`, §6.8) |
| Member bind | `Function_Bind_Member("T_Member", "desc", &TestObj::Method, …)` → `paramCount == arity + 1`, `params[0].name == "object" && params[0].type == nullptr`, `params[1..]` filled |
| Receiver fixup | mirror the Vector path: `TypeT::AddFunction(memberFn)` then check `params[0].type == this` (receiver type filled; `Type.cpp:155`) |
| Alias after source | `Function_Bind("T_Src", …)` then `Function_Alias("T_Src", "T_Alias")` → `Function_Find("T_Alias")` populated and callable |
| Binding lifetime | create + drop `Reference` refs; the `binding` is deleted exactly once in `~FunctionT` (assert under an ASAN build — `9fdc702` runs ASAN) |
| Arity `static_assert` | name-count ≠ arity is a **compile-time** failure (can't runtime-test; guaranteed by the build). Note in test file comment |

Also, when `TestStringBindings.cpp` is eventually registered, keep its cases
green — they call the trampoline through `Function_Find` and are the only
end-to-end register→metadata→call coverage for String.

---

## 6. Per-Family Migration Patterns (before → after)

### 6.1 `FreeFunction` / `VoidFreeFunction` (556 sites)

```cpp
// BEFORE
FreeFunction(String, Object_GetName,
  "Return the name of 'object'",
  Object, object)
{
  return object->GetName();
} FunctionAlias(Object_GetName, GetName);

// AFTER
static Function const Object_GetName_Registration = Function_Bind(
  "Object_GetName",
  "Return the name of 'object'",
  [](Object const& object) -> String { return object->GetName(); },
  "object");
static int const Object_GetName_Alias =
  Function_Alias("Object_GetName", "GetName");
```

Void variant drops `-> void` (lambda defaults to void):
```cpp
static Function const Object_SetName_Registration = Function_Bind(
  "Object_SetName",
  "Set the name of 'object' to 'name'",
  [](Object const& object, String const& name) { object->SetName(name); },
  "object", "name");
static int const Object_SetName_Alias = Function_Alias("Object_SetName", "SetName");
```

`FreeFunctionNoParams` / `VoidFreeFunctionNoParams` (33 sites): lambda takes
no args; no trailing names.

**Rules:**
- Naming: use `static Function const <ScriptName>_Registration` and
  `static int const <ScriptName>_Alias`. Keeps names greppable and avoids
  colliding with the (now gone) `Name##_Metadata`/`Name##_Call` symbols.
- A `.cpp` file with many bindings may use one block at the bottom instead of
  interleaving; but **keep each alias textually after its source** (finding #5).
- Body changes are mechanical: `Name##_Impl(params)` → lambda `(params)`;
  `return x;` → `-> RT { return x; }`.

### 6.2 `DeclareFunction` + `DefineFunction` (~294/280 sites)

```cpp
// BEFORE — header (e.g. Zoned.h)
DeclareFunction(Object_GetZone, Zone, Object, object)
// BEFORE — cpp
DefineFunction(Object_GetZone) {
  return object->GetZone();
} FunctionAlias(Object_GetZone, GetZone);

// AFTER — header: plain C++ function decl (the impl IS the C++ API)
LT_API Zone Object_GetZone(Object const& object);
// AFTER — cpp
Zone Object_GetZone(Object const& object) {
  return object->GetZone();
}
static Function const Object_GetZone_Registration = Function_Bind(
  "Object_GetZone", "None", &Object_GetZone, "object");
static int const Object_GetZone_Alias =
  Function_Alias("Object_GetZone", "GetZone");
```

**Rules:**
- `DefineFunction(Name) { … args.N0 … }` becomes a normal function
  `RT Name(T0 const& N0, T1 const& N1, …)` with **direct param names** —
  `args.N0` → `N0`.
- Description must be `"None"` (preserves API-DB output byte-for-byte).
- The old generated convenience overloads (`Name(a,b)`, `Name()` for 0-arg)
  are subsumed by the real function; C++ call sites keep working unchanged
  (`Name(0, 1, …)` binds to the const& params).
- 0-arg: `LT_API RT Name();` + `Function_Bind("Name", "None", &Name)` (no names).

### 6.3 `DeclareFunctionArgBind` (58 sites, Glyphs + Items/Tasks/etc.)

The `_Args` struct stays as hand-written reflected C++ data; the script
binding unwraps it to the per-param signature.

```cpp
// BEFORE — Glyphs.h
DeclareFunctionArgBind(Glyph_Arc, Glyph,
  V2, position, float, radius, float, radiusS, Color, color,
  float, alpha, float, angle, float, angleS)

// AFTER — Glyphs.h: hand-written reflected struct + C++ API
struct Glyph_Arc_Args {
  V2 position;
  float radius;
  float radiusS;
  Color color;
  float alpha;
  float angle;
  float angleS;

  FIELDS {
    MAPFIELD(position) MAPFIELD(radius) MAPFIELD(radiusS) MAPFIELD(color)
    MAPFIELD(alpha) MAPFIELD(angle) MAPFIELD(angleS)
  }
};
LT_API Glyph Glyph_Arc(Glyph_Arc_Args const& args);
inline Glyph Glyph_Arc(V2 const& position, float radius, float radiusS,
  Color const& color, float alpha, float angle, float angleS) {
  return Glyph_Arc(Glyph_Arc_Args{position, radius, radiusS, color, alpha, angle, angleS});
}

// AFTER — Glyph/Arc.cpp: registration wraps to per-param (finding #6)
Glyph Glyph_Arc(Glyph_Arc_Args const& args) {
  return new Arc(args);
}
static Function const Glyph_Arc_Registration = Function_Bind(
  "Glyph_Arc", "Create an arc glyph", &Glyph_Arc,
  "position", "radius", "radiusS", "color", "alpha", "angle", "angleS");
```

**Rules:**
- `Glyph_Arc_Args` must keep `FIELDS`/`MAPFIELD` reflection (it is serialized
  data and a base class of the glyph).
- Where `AutoClassDerived(Arc, GlyphT, Glyph_Arc_Args, args)` was used, the
  derived struct keeps deriving from `Glyph_Arc_Args` (see the exact
  `AutoClassDerived` expansion in `AutoClass.h`/`AutoClass_Generated.h` during
  that migration — the field member names and `args` accessor must match).
- `using Meta_X = X_Args` metatype aliases (`Game/Items.h:55`) and any
  `X_Args` value uses (e.g. `Object/System.cpp:85`) must be preserved.
- The registered pointer `&Glyph_Arc` must resolve to the **per-param**
  overload; if ambiguous, bind a small lambda that forwards.

### 6.4 `MemberFunction` (4 sites, Vector.h)

```cpp
// BEFORE (inside template<class T> struct Vector)
VoidMemberFunction(Append, "Append 'element' to the back of the vector",
  T, element) { this->push(element); }
// …
METADATA { MEMBERFUNCTION(Append) MEMBERFUNCTION(Get) MEMBERFUNCTION(Size) MEMBERFUNCTION(Set) }

// AFTER
void Append_Impl(T const& element) { this->push(element); }
T Get_Impl(int index) const { return (*this)[index]; }
void Set_Impl(int index, T const& element) { (*this)[index] = element; }
int Size_Impl() const { return (int)this->size(); }

static Function Append_GetMetadata() {
  static Function fn;
  if (!fn)
    fn = Function_Bind_Member("Append",
      "Append 'element' to the back of the vector",
      &Vector::Append_Impl, "element");
  return fn;
}
// … Get/Size/Set analogous …

static void FillMetadata(Type const& type) {
  type->AddFunction(Append_GetMetadata());
  type->AddFunction(Get_GetMetadata());
  type->AddFunction(Size_GetMetadata());
  type->AddFunction(Set_GetMetadata());
}
```

**Rules (finding #7):** lazy (`static Function fn` inside the `GetMetadata`
static member, per specialization); `params[0]=(“object”, nullptr)`; raw
`new Parameter[]`. `const`/`virtual` member variants (unused today) map to the
same binder.

### 6.5 `DefineConversion` (53 sites) — new `Conversion_Bind<&Impl>()`

```cpp
// BEFORE
DefineConversion(int_to_color, int, Color) {
  dest = (Color)src;
}

// AFTER (function never called cross-TU → keep it file-static)
static void int_to_color_Impl(int const& src, Color& dest) { dest = (Color)src; }
static int const int_to_color_Registration = Conversion_Bind<&int_to_color_Impl>();
```

`Conversion_Bind` (NTTP on a named fn pointer; lambdas cannot be NTTP in
C++17):
```cpp
template <auto Fn>
struct ConversionTrampoline;

template <class Source, class Dest, void (*Fn)(Source const&, Dest&)>
struct ConversionTrampoline<Fn> {
  using SourceType = Source;
  using DestType = Dest;
  static void Call(TypeT*, void const* in, void* out) {
    Fn(*(Source const*)in, *(Dest*)out);
  }
};

template <auto Fn>
inline int Conversion_Bind() {
  using Tramp = ConversionTrampoline<Fn>;
  Type type = Type_Get<typename Tramp::SourceType>();
  ConversionType c;
  c.other = Type_Get<typename Tramp::DestType>();
  c.fn = &Tramp::Call;
  type->AddConversion(c);
  return 0;
}
```

### 6.6 `FunctionAlias` (523 sites) → `Function_Alias("src", "alias")`

Per §6.1–6.2 examples. Mechanical: `FunctionAlias(A, B)` → a following
`static int const … = Function_Alias("A", "B");`. **Alias must stay after its
source** (finding #5). `FunctionAlias` sites with no nearby binding (pure
aliases) keep a standalone `static int const` line.

### 6.7 `ObjectComponents.cpp` generative X-macro (exception, documented)

`Game/ScriptAPI/ObjectComponents.cpp` uses `#define X(x) … FreeFunction …`
over `COMPONENT_X` (44 components × 2 bindings) to generate
`Object_GetComponent##x` / `Object_HasComponent##x`. There is no generic
`Object::Get<T>()`, so a macro-free table would need 88 hand-written
tag-dispatch specializations.

**Decision:** keep this ONE generative-list X-macro (it is NOT arity-counting
and NOT fragile — it is the idiomatic loop-over-a-list pattern also used for
enums via `XEnum.h`). Migrate its body to the new `Function_Bind` form inside
the macro, keeping the `#define X(x) … COMPONENT_X` skeleton. Documented
exception; revisit later if desired.

### 6.8 Pre-pass: script-name collisions & overload groups (grep-verified, pre-migration)

Run **before** the mechanical conversions (§9 Step 3 onward) so overload
surprises surface now, not mid-migration. Results (fresh scan, current tree):

**836 distinct script names** are bound by the FF/VFF/Define/ArgBind macros.
Almost every "duplicate" is benign:

| Name | What is really happening | Migration consequence |
|------|--------------------------|-----------------------|
| `RNG_Int` | 3 `FreeFunction(int, RNG_Int, …)` in **one TU** (`LTE/ScriptAPI/RNG.cpp:78,89,99`) — 3 intentional script overloads (0/1/2-arg) | Keep all 3 `Function_Bind("RNG_Int", …)` lines; 3 bucket entries (API DB confirms 3 today). No ambiguity — distinct lambdas. |
| `Vec3_Distance` | **Same script name bound in TWO TUs**: `V2.cpp:22` (V2 a, V2 b) and `V3.cpp:112` (V3 a, V3 b). Today the two `template<int unused> Vec3_Distance_GetMetadata<0>()` definitions are **weak symbols that the linker merges into one**, so the shared `static Function fn` is created once — only ONE registration survives, and *which* TU wins is static-init-order-dependent (today: the V2 signature). See §11 (open question). | Migration produces **two** bucket entries (both overloads live). This **intentionally changes the API DB (+1 fn)** and fixes the nondeterminism. The §8 gate 3 byte-diff must whitelist exactly this one expected diff. |
| 57 `Define`+`ArgBind` pairs | Each `X_Args` C++ struct pair (`Glyph_Arc`, `Generator_Nebula`, `Item_*`, `Task_*`, `Renderable_*`, `Event_*`, `Object_Region`/`Object_System`…) — the header gets a real per-param function + the `_Args` overload after conversion | `&Name` becomes **overload-ambiguous** in `Function_Bind`. §6.3's "bind a small lambda that forwards" applies to **all 57**, not just Glyphs. Register the per-param wrapper explicitly. |
| `Event_Destroyed` | The **1 ArgBind with no `DefineFunction`** anywhere (`Game/Events.h:21`). It is declared but **never registered** — dead code today (0 entries in the API DB) | Convert the header to the `_Args` struct + overloads; add **no** `Function_Bind` (preserves 0-entry status). |
| Everything else flagged by a naive scan | `FunctionAlias(A, B)`/`Function_Alias("A","B")` whose source `A` is itself a binding, and repeat ReturnType tokens (`bool`, `float`, `String`, `V2`, …) mis-parsed as names | Benign; the alias source names are expected to equal binding names. |

Two things to carry into migration:
- **`Vec3_Distance` needs a deliberate decision before Step 3** (see §11).
- The committed `script/ltsl-lsp/api-database.json` (1843 fns / 444 types) is
  **STALE vs the working tree** — a fresh dump now yields **1849 / 445** (6 new
  fns, 1 type from uncommitted WIP: `SaveGame.cpp` bindings, `Int.cpp`/
  `StringList.cpp` edits). **The §8 baseline must be regenerated from the
  current tree, not copied from the committed file.** This also means the
  baseline gate's first check is "working-tree dump == working-tree dump".

---

## 7. Files That Change

### 7.1 Core (new/modified)

| File | Change |
|------|--------|
| `src/liblt/LTE/FunctionBind.h` | **NEW** — the variadic core (§5.2–5.3, 6.5) |
| `src/liblt/LTE/Function.h` | `call` → 3-arg; add `BindingBase* binding`; fwd-decl `BindingBase`; **keep `#include "Function_Generated.h"` until Step 10** (the §5.1a shim needs it); remove `DefineConversion`/`FunctionAlias` macros in Step 10 |
| `src/liblt/LTE/Function_Generated.h` / `DeclareFunction.h` | §5.1a **shim** in Step 1 (prepend unnamed `void*` to emitted `Name##_Call`); then **DELETE** in Step 10 |
| `script/check_binding_alias_order.py` | **NEW** in Step 1 — §8 gate 6 alias-order checker |
| `src/liblt/LTE/Function.cpp` | init `binding=0`; `~FunctionT` deletes `binding`; include `FunctionBind.h` |
| `src/liblt/LTE/Expression/FunctionCall.cpp:74` | pass `function->binding` |
| `src/liblt/LTE/Expression/Function.cpp:9` | `ScriptFunction_Call` stub → 3-arg |
| `src/liblt/LTE/Type/Array.cpp:180–202,262–323` | 6 handlers → 3-arg; `fn->binding = nullptr` |
| `src/liblt/LTE/Function_Generated.h` | **DELETE** |
| `src/liblt/LTE/DeclareFunction.h` | **DELETE** |
| `script/meta/DeclareFunction.py`, `script/meta/common.py` | **DELETE** (not build-wired) |
| `tests/TestStringBindings.cpp:126,131,144,162` | `fn->call(fn->binding, args, &result)` (3-arg) |
| `tests/TestFunctionBind.cpp` | **NEW** in Step 1 — binder unit tests (§5.4); add to `tests/CMakeLists.txt` `TEST_SRC` |

### 7.2 Call-site files (by subsystem)

Counts (grep-verified; `Decl` = `DeclareFunction*` in `.h`):
```
LTE        FF 250  VFF 58  NoP 21  MF 2   DefineFn 102  Decl 113  Alias 296  Conv 49
Game       FF  48  VFF 18  NoP  6          DefineFn 125  Decl 128  Alias  63  Conv  4
UI         FF  21  VFF 19  NoP  0          DefineFn  45  Decl  45  Alias  59  Conv  0
Component  FF  63  VFF 31  NoP  0          DefineFn   2  Decl   2  Alias  94  Conv  0
Module     FF   6  VFF  9  NoP  6          DefineFn   5  Decl   5  Alias  11  Conv  0
Audio/Volume/ThirdParty: 0 (no bindings)
```
Totals: FF 388, VFF 135, NoP 33, Member 4 (Vector.h), DefineFn 279, Decl 293
(incl. 58 ArgBind), Alias 523, Conversion 53.

ArgBind structs live in `UI/Glyphs.h` and `Game/{Items,Events,Tasks,Renderables,Objects}.h`,
`Game/Graphics/Generators.h`; define-files under `UI/Glyph/*.cpp`,
`Game/Item/*.cpp`, `Game/Object/*.cpp`, `Game/Task/*.cpp`, etc.

### 7.3 Delete-after checks

- `grep -rn "_GetMetadata\|_ExplicitCall\|_ArgRefs\|_ParamCount\|_Call\b" src/` —
  after migration nothing should reference these (they are macro-generated).
- `grep -rn "FreeFunction\|VoidFreeFunction\|DeclareFunction\|DefineFunction\|DefineConversion\|FunctionAlias" src/` — only the accepted
  `ObjectComponents.cpp` X-macro + the new API names should remain.
- The LSP (`script/ltsl-lsp/`), tree-sitter, extensions, and `docs/` do **not**
  reference any generated symbol (verified). `docs/VULKAN-AND-SPACE-PHENOMENA.md:685`
  mentions `Generator_Nebula_Args` — aspirational snippet, ignore.

---

## 8. Verification Gates (run after EVERY migration chunk)

1. **Build:** `python3 configure.py build` (parallel; `lt`/`launch` are
   `-Werror` — the binder must be warning-free).
2. **Unit tests:** `python3 configure.py test` (all `lte_tests` pass — incl. the
   new `TestFunctionBind.cpp` binder tests from §5.4).
3. **API DB byte-diff (critical regression net):**
   ```bash
   cmake --build ./build --target ltsl_api_dump -j
   LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltsl_api_dump build/api-baseline.json
   # regenerate AFTER each chunk and diff against the ORIGINAL pre-migration baseline
   diff <(python3 -m json.tool build/api-baseline.json) <(python3 -m json.tool build/api-after.json)
   ```
   The DB must be **byte-identical** except where a migration intentionally
   changes it — currently the expected diffs are all in the gate-3 whitelist
   (§11): `Vec3_Distance` gaining a second overload entry (the previously
   weak-merged V3 overload; appears twice — once under its own name, once as
   the `Distance` alias copy) and the `Dot(Vec4f)` entry restored by the
   `Vec4_Dot` fix. Actual Step-3 result: **+3 lines, 0 removed** (verified).
   (Save the baseline NOW, before any migration — regenerate from the
   **current working tree**: the committed `api-database.json` is stale,
   1843/444 vs 1849/445 fresh; §6.8.) The durable copy lives in
   `build/api-baseline.json` (gitignored, survives /tmp cleanup; also mirrored
   to `/tmp/api-baseline.json`).
4. **LSP regression:** `node script/ltsl-lsp/test-rpc.js` and
   `node script/ltsl-lsp/out/smoke.js $(find resource/script -name '*.lts' | sort)`
   must stay at exactly **6 diagnostics** (see AGENTS.md §6.2).
5. **App runs (trampoline coverage — the tests/LSP do NOT cover this):**
   ```bash
   python3 configure.py run war
   python3 configure.py run ltheory-main
   python3 configure.py run ui
   python3 configure.py run market
   python3 configure.py run map
   python3 configure.py run hud
   python3 configure.py run rails
   python3 configure.py run threads
   ```
   Watch for: crash on startup, "unknown function", wrong-value returns
   (NaN/zero), or type-mismatch assertions (`force.IsFinite()`-style). `rails`
   and `threads` are the cheapest automated-feel checks (deterministic sums /
   rails 3/3 clean runs per AGENTS A.7).
6. **Alias-ordering invariant (automated — new gate, ~60-line script):**
   `Function_AddAlias` copies the *current* source bucket (`Function.cpp:62`),
   so an alias before its source silently registers an empty bucket — §4's
   #5-ranked silent breaker, and 523 sites are hand-converted. Do not rely on
   manual audit. Commit as `script/check_binding_alias_order.py` in Step 1 and
   run it in every gate:
   ```bash
   python3 script/check_binding_alias_order.py \
     $(git ls-files 'src/liblt/**/*.cpp' 'src/liblt/**/*.h')
   ```
   The script understands **both** old (`FunctionAlias(A, B)`) and new
   (`Function_Alias("A", "B")`) syntax so it is usable in the mixed state
   through Steps 2–9. It fails if any alias's source name is not registered (or
   itself aliased) textually earlier in the same file. Alias chains
   (`A→B`, then `B→C`) are handled because aliases contribute *both* their
   names. It is intentionally lenient (permissive regexes) — its job is to catch
   ordering regressions, not to be a full parser.
   - **`KNOWN_EXCEPTIONS`:** two pre-existing broken aliases were found when the
     checker was first run and are whitelisted (reported, not failing) because
     fixing them changes the API DB (gate-3 whitelist, §11): `V2.cpp`'s
     `Vec2_Distance` (the §6.8 copy-paste bug) and **`V4.cpp`'s `Vec4_Dot`**
     (registers `Vec4f_Dot` but aliases `Vec4_Dot` → the V4F `Dot` overload is
     missing today; found by this checker, scheduled to be fixed during
     migration). Any *new* violation is a hard failure. Baseline run (committed
     with the script): `OK: 515 alias sites follow their source (2 known
     exceptions)`.

   ```python
   # script/check_binding_alias_order.py  (committed file is authoritative)
   # Alias-ordering invariant checker. Function_AddAlias copies the *current*
   # source bucket (Function.cpp:62); an alias before its source silently
   # registers an empty bucket. Fails any alias whose source is not registered
   # (or itself aliased) textually earlier in the same file. Understands both
   # old (FunctionAlias(A, B)) and new (Function_Alias("A", "B")) syntax.
   # KNOWN_EXCEPTIONS: pre-existing broken aliases whitelisted because fixing
   # them changes the API DB (gate-3 whitelist, SS11) - V2.cpp 'Vec2_Distance'
   # (SS6.8 copy-paste) and V4.cpp 'Vec4_Dot' (registers Vec4f_Dot; the V4F
   # 'Dot' overload is missing today). New violations are hard failures.
   import re, sys
   ALIAS_NEW = re.compile(r'Function_Alias\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)')
   ALIAS_OLD = re.compile(r'\bFunctionAlias\(([A-Za-z_]\w*)\s*,')
   NAMES = [
     re.compile(r'Function_(?:Bind|Bind_Member|Create)\(\s*"([^"]+)"'),
     re.compile(r'Function_Alias\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)'),
     re.compile(r'\bFunctionAlias\(([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)'),
     re.compile(r'\bDefineFunction\(([A-Za-z_]\w*)'),
     re.compile(r'\bDeclareFunction(?:ArgBind)?\(([A-Za-z_]\w*)'),
     re.compile(r'\bVoidFreeFunction(?:NoParams)?\(([A-Za-z_]\w*)'),
     re.compile(r'\bFreeFunction(?:NoParams)?\([^,]+,\s*([A-Za-z_]\w*)'),
   ]
   KNOWN_EXCEPTIONS = {
     ('src/liblt/LTE/ScriptAPI/V2.cpp', 'Vec2_Distance'),
     ('src/liblt/LTE/ScriptAPI/V4.cpp', 'Vec4_Dot'),
   }
   def names_on(line):
     out = set()
     for p in NAMES:
       for m in p.finditer(line):
         out.update(g for g in m.groups() if g)
     return out
   def main(paths):
     bad, known, checked = [], [], 0
     for path in paths:
       lines = open(path, encoding='utf-8').read().splitlines()
       seen = set()
       for i, line in enumerate(lines):
         if line.lstrip().startswith('#define'):
           continue
         m = ALIAS_NEW.search(line) or ALIAS_OLD.search(line)
         if m:
           checked += 1
           if m.group(1) not in seen:
             if (path, m.group(1)) in KNOWN_EXCEPTIONS:
               known.append((path, i + 1, m.group(1), line.strip()))
             else:
               bad.append((path, i + 1, m.group(1), line.strip()))
         seen |= names_on(line)
     for p, ln, src, line in known:
       print(f"KNOWN: {p}:{ln}: alias of {src!r} before its source (documented exception): {line}")
     if bad:
       for p, ln, src, line in bad:
         print(f"FAIL: {p}:{ln}: alias of {src!r} appears before its source: {line}")
       print(f"FAIL: {len(bad)} new violation(s) of {checked} alias sites ({len(known)} known exceptions)")
       sys.exit(1)
     print(f"OK: {checked} alias sites follow their source ({len(known)} known exceptions)")
   if __name__ == '__main__':
     main(sys.argv[1:])
   ```

---

## 9. Migration Plan (phase order)

Recommended order (canary-first, densest-to-least):

1. **Core + canary:** save the API-DB baseline from the **current tree**
   (regenerate, don't copy the stale committed file — §6.8); create
   `FunctionBind.h` (with the `remove_cvref_t` polyfill); change the
   `FunctionT` contract (§5.1); apply the §5.1a **compatibility shim** to
   `Function_Generated.h` + `DeclareFunction.h`; update    `FunctionCall.cpp`,
   `Function.cpp`, `Expression/Function.cpp` stub, `Type/Array.cpp` (6
   handlers), `tests/TestFunctionBind.cpp` binder tests (§5.4) added to
   `tests/CMakeLists.txt`;
   commit `script/check_binding_alias_order.py` (§8 gate 6). **Gate:** build +
   tests (incl. new binder tests), alias-order script, LSP smoke. `Function.h`
   keeps the generated-header include until Step 10.
   (`tests/TestStringBindings.cpp` is deferred — currently unregistered WIP,
   §4 baseline note; migrate its 4 sites when the String API lands.)
2. **Vector.h member functions** (4 sites) — validates laziness + AddFunction
   fixup + raw-array requirement in the hardest environment. **Gate.**
3. **LTE subsystem FreeFunction family** (`LTE/ScriptAPI/*.cpp` etc., ~250 FF +
   58 VFF + 21 NoP). **Gate** (API-DB diff + app runs).
4. **Component subsystem** (63 FF + 31 VFF + 94 Alias). **Gate.**
5. **Game subsystem incl. DeclareFunction/ArgBind** (Items/Events/Tasks/
   Renderables/Objects/Generators; 125 DefineFn). **Gate.**
6. **UI subsystem incl. Glyphs ArgBind** (45 DefineFn). **Gate.**
7. **Module subsystem** (small). **Gate.**
8. **DefineConversion** (53 sites) + `TypeAlias` (keep or convert — TypeAlias
   is already macro-free-friendly; low priority). **Gate.**
9. **ObjectComponents.cpp** X-macro body → new API (keep the list macro).
   **Gate.**
10. **Delete** `Function_Generated.h`, `DeclareFunction.h`,
    `script/meta/DeclareFunction.py`, `script/meta/common.py`. Update
    `AGENTS.md` (§2 repo map, §4 table row for Function/DeclareFunction, §6
    analyzer notes if any, §9.1 reflection notes) and this doc (§12). **Gate:**
    full suite + all 8 app runs.
11. **Optional cleanup:** add real descriptions to `DeclareFunction` sites
    (currently `"None"`) — deferred, would change the API DB intentionally.

Each chunk: one subsystem or sub-folder; commit at chunk boundaries; never
commit a red build.

---

## 10. Facts Log (things that took effort to verify — do not re-derive)

- The ONLY interpreter call site: `Expression/FunctionCall.cpp:74`.
- `_Impl` functions: never called externally — safe to absorb into lambdas.
- `DeclareFunction` sites have description `"None"` (must preserve).
- ArgBind script signatures are per-param (unwrapped), not the `_Args` struct.
- Vector member registration: lazy, `params[0]=(“object”, nullptr)`,
  `TypeT::AddFunction` fills the type (`Type.cpp:155`).
- ArrayCustom functions DO go through `Function_Create` (global registry) AND
  are attached to the `Array<…>` type's `GetFunctions()`.
- `Function_AddAlias` snapshots the source bucket at call time → alias must
  follow source.
- `Type_Get<T const&>()` resolves to `Type_Get<T>()` (via `Type_Ref` ADL);
  `Type_Get<void>()` is specialized.
- `ConversionFn` = `void(*)(TypeT*, void const*, void*)` — different from the
  function trampoline; do not unify.
- `Mutable()` (`Mutable.h`) casts away const on `params[i]`; house pattern for
  writing `Parameter const*` arrays.
- `FunctionT::GetAux()`/`FunctionImpl::extra` cast in `Function.cpp:47` stays
  valid when adding the `binding` field.
- Build flags: C++17, `-fno-exceptions`, `-pedantic -Wall -Wextra`, `-Werror`
  on `lt`/`launch`, RTTI ON, `-msse -msse2`. `-Wno-unused-parameter` is set at
  `CMakeLists.txt:205` (the §5.1a shim uses an unnamed param anyway, which can
  never warn).
- `remove_cvref_t`: **no polyfill exists anywhere in `src/`** (grep verified).
  `std::remove_cvref_t` is C++20 (P0550R2); this build is C++17
  (`CMakeLists.txt:195`). The local polyfill in `FunctionBind.h` is the only one.
- **§5.2's `FunctionTraits` name collides** with the unrelated pre-existing
  `LTE::FunctionTraits<ReturnT>` (`Generic.h:10`), which becomes visible at
  global scope via `Common.h:352`'s `using namespace LTE;` → `-Wtemplate-body`
  "ambiguous" errors in every TU that sees both. The binder's introspection
  trait (internal detail) was renamed to `BindingTraits` (Step 2, first build
  failure). Note in §5.2/§6.4 examples when reviewing.
- The committed `script/ltsl-lsp/api-database.json` is **stale**: 1843 fns / 444
  types vs **1849 / 445** from a fresh dump of the current tree (uncommitted WIP
  adds bindings — `SaveGame.cpp`, `Int.cpp`, `StringList.cpp`). Regenerate the
  baseline; never diff against the committed file.
- `Vec3_Distance` is bound in **two TUs** (`V2.cpp:22`, `V3.cpp:112`), yet the
  API DB shows **one** entry. Cause: `template<int unused> Name##_GetMetadata()`
  is a **weak symbol**; the linker merges the two TUs' definitions into one, and
  the shared `static Function fn` means only the first TU to static-init
  registers. The survivor is link/init-order-dependent (today: the V2
  signature). `nm` confirms both `Vec3_Distance_Metadata` statics present + one
  merged weak `GetMetadata`. Post-migration `Function_Bind` yields **two** bucket
  entries (both overloads live) — the one intentional API-DB change.
- `Common.h:352` puts `using namespace LTE;` at global scope — call sites
  (global scope) resolve `String`/`Function`/`Object` unqualified.
- **`#` is not a comment in the engine's LTSL lexer** — but it only bites some
  files. `war`/`map`/`rails` (no `#`) and `ltheory-main` (has a `#` header)
  all compiled and ran clean, while `threads.lts` (has a `#` header) failed
  with `unknown reference '#'` / `no native function named '#'` until its
  header was removed. The tokenizer treats `#` as a token (`Grammar.cpp:150`
  `#` handling is grammar-rule tags, not script comments); the ZED/extension
  `#`-comment rule is editor syntax only. So the trigger is not simply
  "a `#` line exists" — likely a specific token/context in the failing file.
  **Deferred — user: "ignore the '#' for now".** Open question: teach the
  engine lexer to strip `#` line comments (small C++ change).
- **`script/migrate_freefunction.py`** (untracked tool, Step 3) — mechanical
  transformer for the four FreeFunction-family macros → multiline
  `static Function const X_Registration = Function_Bind("X", "desc", [](…)…, "p", …);`
  + `static int const X_Alias = Function_Alias("X", "alias");`, plus
  `ensure_include` for `#include "LTE/FunctionBind.h"`. Not idempotent-safe to
  re-run (skips files with no macros). Validated against hand-migrated
  `Bool.cpp`/`Timer.cpp`.
- **`script/check_binding_alias_order.py`** (Step 3 update) — now understands the
  migration's multiline `Function_Bind(\n "Name",` layout (name captured from the
  line following the call-open). Current state: `OK: 509 alias sites follow
  their source (1 known exception)` — the lone exception is the deferred
  `V2.cpp 'Vec2_Distance'` copy-paste bug (§11).
- **`script/migrate_definefunction.py`** (committed Step 5) — mechanical
  transformer for the `DeclareFunction`/`DeclareFunctionNoParams`/
  `DeclareFunctionArgBind` families (the §6.2/§6.3 pattern, validated Step 4
  on Component's 2 sites and applied Step 5 to all of Game). Header:
  `DeclareFunction` → `LT_API RT Name(T0 const& N0, …);`;
  `DeclareFunctionNoParams` → `LT_API RT Name();`;
  `DeclareFunctionArgBind` → `AutoClass(Name_Args, …)` bundle + per-param
  inline overload + `LT_API RT Name(Name_Args const& args);`; drops the
  `LTE/DeclareFunction.h` include and adds `LTE/AutoClass.h` when a bundle is
  emitted. Cpp: `DefineFunction(Name)` bodies become real functions
  (`args.X`→`X` for plain; bundle kept for argbind), followed by
  `static Function const Name_Registration = Function_Bind("Name", "None", …)`
  (+ `Function_Alias` when a `FunctionAlias(Name, Alias)` tail follows the
  body). Plain bodies that forward the whole bundle (no `args.X` access) are
  auto-detected and migrated as argbind with a per-param lambda registration
   (`Object_Missile`, `Object_System`). Skips `#if 0` blocks. Not idempotent
   (skips files with no macros); re-run dry — `Parsed 0` means done.
   **Step 5b run notes:** headers whose `#include "DeclareFunction.h"` used the
   relative form (no `LTE/` prefix — all of LTE/Module/Strukt) were stripped
   manually via `sed -i '/^#include "DeclareFunction.h"$/d'` (the tool only
   drops the `LTE/`-prefixed form; `LTE.h` keeps its umbrella include until
   Step 10). **`FunctionAlias` lines NOT attached to a DefineFunction body brace
   are not migrated by the tool** — the SDF operator aliases
   (`SDF/Translate.cpp` `+`, `SDF/Scale.cpp` `*`, `SDF/Add.cpp` `+`,
   `SDF/Subtract.cpp` `-`) were hand-converted to standalone
   `static int const X_Alias = Function_Alias("X", "op");` (identical runtime
   semantics to the old macro — also `Function_AddAlias`). **Declare-in-cpp
   sites** are not handled — `Mesh_CylinderHUD` (`Meshes.cpp`, the only one in
   src) was hand-migrated to `static Mesh Mesh_CylinderHUD(float const&, float
   const&)` + `Function_Bind`. **Overloaded `&Name`** for a migrated function
   must go through a per-param lambda (Shader_Create, ShaderInstance_Create).
   Migrated decls may land at column 0 inside `namespace LTE { … }` — re-indent
   to match neighbors (done for `Script.h`, `Location.h`; `Mouse.h` was already
   col-0 house style). **Latent-bug find:** the old `DeclareFunctionNoParams`
   made `ArgRefs=int`, so `ParticleSystem_Pop(particles)` (Component/Interior.cpp)
   silently compiled via `Reference::operator bool`→int with the arg unused; the
   migrated 0-arg signature exposed it — fixed to `ParticleSystem_Pop()` (real
   fix, not a regression).
   **Step 6 run notes (two manual fixes the tool cannot do):**
   (1) **Shadowed locals:** `migrate_definefunction.py` warns (does not fix)
   when a stripped param name collides with a body local. In
   `WidgetRenderer.cpp` this was a real semantic break — `V4 color = V4(args.color,
   args.alpha)` became self-referential `V4 color = V4(color, alpha)` (reads
   the uninitialized local; `-Wshadow`+`-Werror` then forces a fix anyway).
   Locals renamed: `color`→`color4`, `size`→`sizePx` (DrawPanel),
   `color`→`color4` (DrawPanelRadial), `pos`→`posGL` (ClipRegion_Push; this
   one was semantically safe — the local's initializer never reads the param).
   (2) **Bundles need complete types:** `WidgetRenderer.h`'s
   `WidgetRenderer_DrawText{,_Glow}_Args` bundles hold `Font`/`String` values;
   the old DeclareFunction macros only needed the type *names*, but AutoClass
   needs them complete. Added explicit includes (Font/String/Texture2D/
   Transform/Renderable + UI/Glyph.h for Glyph/GlyphState/Color) to
   WidgetRenderer.h. Glyph bundles (V2/float/Color) needed none — Glyph.h
   already pulls Color.h/V3.h.
- **`DeclareFunction`/`DefineFunction` migration pattern (validated Step 4,
  Component — use for Steps 5/6):** header `DeclareFunction(Name, RT, T0, N0, …)`
  → `LT_API RT Name(T0 const& N0, …);` + drop `#include "LTE/DeclareFunction.h"`;
  cpp `DefineFunction(Name) { …args.N0… }` → plain `RT Name(T0 const& N0, …) { …N0… }`
  with an `args.`-prefix-strip rewrite (only the declared param names), then
  `static Function const Name_Registration = Function_Bind("Name", "None", &Name, "N0", …)`
  (+ `Function_Alias("Name", "Alias")` when a `FunctionAlias(Name, Alias)` line
  follows the body). Description must stay `"None"` (old `RegisterFunction`
  hardcodes it — needed for the API-DB byte-diff). Type visibility at the
  DeclareFunction point is guaranteed: the old macro already required the
  param/return type NAMES there (`typedef T0 Name##_ParamType0;` etc.), so the
  new declaration compiles wherever the old one did. C++ call sites are
  unchanged — the old inline convenience overload `RT Name(T0 const& N0, …)` has
  the exact signature of the new real function. Validated on `Object_AddHistory`
  (external C++ caller `Game/Action/Mine.cpp:40`) and `Object_GetZone` (script
  caller `Widget/HUD/Container.lts:18` via the `GetZone` alias).

---

## 11. Open Questions / Decisions To Revisit

- **`TypeAlias` (Type.h:99):** keep as-is (it is a trivial, non-fragile macro)
  or convert. Default: keep, out of scope for this pass.
- **`ObjectComponents.cpp` X-macro:** keep as documented exception (§6.7).
  Revisit only if a generic `Object::Get<T>()` is added later.
- **Descriptions on `DeclareFunction` sites:** currently `"None"`; adding real
  doc strings is a follow-up that intentionally changes the API DB.
- **`Const/Virtual` member variants:** unused today; the binder supports them
  via the same `FunctionTraits` member-fn paths (const already handled).
- **`Vec3_Distance` dual registration (§6.8):** `V2.cpp` and `V3.cpp` both bind
  the script name `Vec3_Distance`. Today the weak-symbol merge keeps only one
  (init-order-dependent). Migration registers both overloads — an intentional
  API-DB +1. **DECIDED (Step 3):** keep both; whitelist the +1 in the §8 gate 3
  diff. Behavior fix: the V2 overload becomes reliably available (today it wins
  only by init-order luck). The stray `FunctionAlias(Vec2_Distance, Distance)`
  in V2.cpp keeps its no-op form (`Function_Alias("Vec2_Distance", "Distance")`)
  so no extra DB entry appears.
  - *(deferred alternative)* Fix the apparent copy-paste in `V2.cpp:22` (it
    binds `Vec3_Distance` with V2 params and aliases the nonexistent
    `Vec2_Distance` → `Distance`; the intent was likely a `Vec2_Distance`
    binding). Would add a new name, drop a `Distance` alias entry. Deferred.
- **`Vec4_Dot` broken alias (found by gate-6 checker, `V4.cpp:102`):** the file
  registers `FreeFunction(float, Vec4f_Dot, ...)` but aliases
  `FunctionAlias(Vec4_Dot, Dot)` — `Vec4_Dot` is never registered, so the V4F
  `Dot` overload is missing today (the `Dot` bucket has 4 valid entries: Vec2,
  Vec3f, Vec3d, Vec4d). **FIXED (Step 3, vector chunk):** alias source changed
  to `Vec4f_Dot` (V4.cpp, `Vec4f_Dot_Alias`). This adds one `Dot` entry to the
  API DB — whitelisted in the gate-3 diff. Removed from the checker's
  `KNOWN_EXCEPTIONS`.
- **Commit discipline during review gates:** the user may run each revision
  past another AI. Keep the doc's design-review sections (§4, §8 rows, §6.8)
  as a running log so reviewer findings are never re-derived.

---

## 12. Progress Checklist

- [x] Save API-DB baseline (`build/api-baseline.json`, durable gitignored copy
      + `/tmp/api-baseline.json`) **regenerated from the current working tree**
      (committed `api-database.json` is stale — §6.8). Fresh dump = 1849 fns /
      445 types; diff vs committed 1843/444 is exactly +6 fns (`ClearAssets`,
      `LoadGame`, `Object_ClearAssets`, `SaveGame`, `SaveGame_Create`,
      `SaveGame_Load`) + 1 type (`SaveGameData`) — all WIP; zero existing
      entries changed.
- [x] Commit `script/check_binding_alias_order.py` (§8 gate 6). Baseline run:
      `OK: 515 alias sites follow their source (2 known exceptions)` — the
      exceptions are the pre-existing `V2.cpp` `Vec2_Distance` (deferred, §11)
      and `V4.cpp` `Vec4_Dot` (new finding, fix during migration). Also added
      `FreeFunctionNoParams`/`VoidFreeFunctionNoParams` capture + `#define` skip
      (fixes false positives); chain and failure paths verified.
- [x] Step 1 — core `FunctionBind.h` (+ `remove_cvref_t` polyfill) + `FunctionT`
      contract + §5.1a compatibility shim + island updates (incl. the 4
      `fn->call` sites in `TestStringBindings.cpp` — deferred until that file is
      registered, §4 baseline note).
- [x] Step 1 — new `tests/TestFunctionBind.cpp` (§5.4) registered in
      `tests/CMakeLists.txt`.
- [x] Step 1 gate — build, tests (incl. new binder tests), alias-order script,
      LSP smoke (6). Verified: 317 checks / 0 failures; API dump byte-identical
      to baseline; alias-order OK (515 sites, 2 known exceptions); smoke 6.
- [x] Decide `Vec3_Distance` outcome (§11) before Step 3. **Decided: keep both
      overloads; whitelist the +1 in the §8 gate 3 diff.**
- [x] Step 2 — Vector.h members. `Function_Bind_Member` lazy `GetMetadata`
      statics replace the 4 MemberFunction macros; §5.2's `FunctionTraits`
      renamed to `BindingTraits` (collision with `LTE::FunctionTraits`,
      `Generic.h:10` — see §10 facts log). Gate: build, 317 checks / 0
      failures, API dump byte-identical, alias-order OK, LSP smoke 6, all app
      runs OK (user-verified).
- [x] **Step 3 — LTE FreeFunction family (+ the FF/VFF/NoP families across ALL
      subsystems).** Bulk-migrated every `FreeFunction`/`VoidFreeFunction`/
      `FreeFunctionNoParams`/`VoidFreeFunctionNoParams` site outside
      `Function_Generated.h` (~61 `.cpp` files: LTE, Component, Game, UI,
      Module) via `script/migrate_freefunction.py` (tool, §10 below), plus
      hand-rewrote the 6 token-paste generator sites (ObjectComponents,
      Item, Object, Widget, Keyboard, ShaderInstance block). Compile fixes:
      `#include "LTE/FunctionBind.h"` in 5 files, `Motion.cpp`
      `Position hitPoint = {};`. Gate: build 100%, 317 checks / 0 failures,
      API-DB diff = **3 added / 0 removed** (the sanctioned whitelist:
      `Vec3_Distance(Vec3f)` under its own name + its `Distance` alias copy,
      and the `Dot(Vec4f)` restored by the `Vec4_Dot` fix), alias-order
      **OK: 509 sites / 1 known exception** (checker updated for the migration's
      multiline `Function_Bind(\n "Name",` format; `Vec4_Dot` exception
      removed), LSP smoke 6, app runs `war`/`map`/`rails`/`threads` clean
      (      exit 0). `threads.lts` comment header removed (engine does not lex `#`
      comments; see note in §10). The `#`-header apps (`ltheory-main`, `ui`,
      `market`, `hud`, …) were originally assumed blocked by the same `#` lexer
      quirk — but **`ltheory-main` ran clean for the user (verified)** despite
      its `#` header, so the `#` compile error seen in `threads.lts` was
      app-specific, not header-caused. `threads.lts` runs after header removal.
      Remaining `#`-header apps: not yet re-verified (see note in §10).
- [x] Step 4 — Component subsystem **FF/VFF family done** (same bulk run).
      **DefineFunction family done** — the 2 `DeclareFunction`/`DefineFunction`
      sites (`Object_AddHistory` in `History.h/cpp`, `Object_GetZone` in
      `Zoned.h/cpp`) migrated to the §6.2 pattern: header decl
      `LT_API RT Name(T0 const& N0, …);` (dropped the
      `#include "LTE/DeclareFunction.h"`), cpp body becomes a plain function
      with direct params (`args.N0` → `N0`), then
      `Function_Bind("Name", "None", &Name, "N0", …)` + `Function_Alias`.
      Gate: build 100%, 317 checks / 0 failures, API-DB **byte-identical**
      (1852/445), alias-order OK (509/1), LSP smoke 6, `war` run clean (exit 0,
      exercises the `GetZone` alias via `Widget/HUD/Container.lts`). This was the
      canary for the §6.2 pattern — see §10 facts log before doing Steps 5/6.
- [x] Step 7 — Module subsystem **FF/VFF family done** (same bulk run).
- [x] Step 9 — ObjectComponents.cpp X-macro body rewritten to the new API
      (kept the `#define X(x) … COMPONENT_X` list skeleton).
- [x] Step 5 — Game subsystem **FF/VFF done** (Step 3 run) + **DefineFunction/
      ArgBind done** (commit `6cecc22`). All 118 Game files migrated via
      `script/migrate_definefunction.py` (committed here — same pattern as Step 3;
      parses DeclareFunction/DeclareFunctionNoParams/DeclareFunctionArgBind,
      rewrites headers to `LT_API` per-param decls / `AutoClass X_Args` bundles +
      per-param overloads, rewrites cpp bodies with `args.X`→`X`, adds
      `Function_Bind` + `Function_Alias`; detects bundle-forwarding bodies and
      migrates them as argbind with per-param lambda registrations; skips `#if 0`
      blocks). **Gate:** build green (-Werror), 317 checks / 0 failures, API-DB
      functions **byte-identical + the 3 sanctioned additions** (0 doc diffs),
      alias-order OK 509/1, LSP smoke 6, app runs `war`/`ltheory-main`/`rails`/
      `threads` clean (`map` hit an intermittent amdgpu context loss at GPU
      render — environmental; `ui`/`market`/`hud` still on the deferred `#`
      lexer quirk).
      **Type-order observation (new, non-whitelist):** the API-DB `types` list
      **reordered** after Step 5 — same 445-type set, **0 content diffs** per
      type, but registration order shifted. Cause: the migration moved each
      TU's eager registrations to a bottom-of-file block, changing cross-TU
      static-init order of `Type_Get<T>()`. Functions section order was
      unaffected (only the +3 additions). Benign for the LSP (name-keyed). If
      the gate-3 byte-diff is used for Steps 6–8, compare the functions section
      + type *sets* (order-insensitive) or regenerate
      `build/api-baseline.json` from the current tree after this commit.
- [x] **Step 5b — LTE/Module/Strukt DefineFunction/DeclareFunction family done**
      (commit `e54f98b`). Same tool run over `src/liblt/LTE` (19 headers + 43
      cpps, 102 DefineFunction sites), `src/liblt/Module` (Settings, FrameTimer)
      and `src/liblt/Strukt` (CodeObject_Custom), plus the manual fixes recorded
      in the §10 facts log (relative-include strip, SDF `FunctionAlias` →
      `Function_Alias` standalone lines, `Mesh_CylinderHUD` hand-migration,
      `Shader_Create`/`ShaderInstance_Create` per-param lambdas, decl re-indent
      in `Script.h`/`Location.h`, and the `ParticleSystem_Pop` latent-bug fix in
      Component/Interior.cpp). **Gate:** build green (-Werror), 317 checks / 0
      failures, API-DB **byte-identical to the Step 5 dump** (1852/445 — still
      just the 3 sanctioned fn additions, 0 doc diffs; SDF operator aliases were
      already runtime-registered by the old macro so no DB change), alias-order
      OK 509/1 (now actually *covering* the 4 SDF aliases), LSP smoke 6, app
      runs `war`/`rails`/`threads` clean. UI (Step 6) is now the last
      DefineFunction/DeclareFunction holdout (9 headers still include
      `LTE/DeclareFunction.h`).
- [x] Step 6 — UI subsystem **DefineFunction/ArgBind done** (commit `28bec13`).
      All 9 UI headers (45 decls incl. the Glyph_* ArgBind family) + 25 cpps
      migrated with `migrate_definefunction.py`. **Gate:** build green
      (-Werror), 317 checks / 0 failures, API-DB **byte-identical to the Step 5b
      dump** (1852/445, 0 added/0 removed, 0 doc diffs — UI fns were already in
      the baseline; the alias-order checker counts both old `FunctionAlias` and
      new `Function_Alias` forms, so the 509 OK / 1 known count is unchanged
      after the old→new conversion), LSP smoke 6, app runs `war`/`rails`/
      `threads`/`ltheory-main`/`objectinfo`/`dogfight` clean (`ui`/`market`/
      `hud`/`launcher`/`platemesh`/`hnn`/`colony` still on the deferred `#`
      lexer quirk). See §10 facts log for two manual fixes the tool can't do
      (shadow locals + bundle complete-type includes). **UI is now the last
      DefineFunction/DeclareFunction holdout cleared — only `DefineConversion`
      (Step 8) and the shim deletion (Step 10) remain.**
- [ ] Step 8 — DefineConversion.
- [ ] Step 10 — delete generator + old headers; update AGENTS.md + this doc.
- [ ] Full verification: build, tests, API-DB diff, LSP smoke (6), 8 app runs.
